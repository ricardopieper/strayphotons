// Project Headers
#include "OpenVrSystem.hh"

#include "assets/AssetManager.hh"
#include "core/Common.hh"
#include "core/Logging.hh"
#include "core/Tracing.hh"
#include "ecs/EcsImpl.hh"
#include "game/Scene.hh"
#include "game/SceneManager.hh"
#include "graphics/core/GraphicsContext.hh"

#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <openvr.h>
#include <stdexcept>
#include <thread>

namespace sp::xr {
    vr::EVREye MapXrEyeToOpenVr(ecs::XrEye eye) {
        switch (eye) {
        case ecs::XrEye::Left:
            return vr::Eye_Left;
        case ecs::XrEye::Right:
            return vr::Eye_Right;
        default:
            Abortf("Unknown XrEye enum: %d", eye);
        }
    }

    OpenVrSystem::OpenVrSystem(GraphicsContext *context)
        : RegisteredThread("OpenVR", 120.0, true), context(context), eventHandler(*this) {
        StartThread();
    }

    OpenVrSystem::~OpenVrSystem() {
        StopThread();

        GetSceneManager().QueueActionAndBlock(SceneAction::RemoveScene, "vr");
        GetSceneManager().QueueActionAndBlock(SceneAction::RemoveScene, "vr_system");
        loaded.clear();
        vrSystem.reset();
    }

    bool OpenVrSystem::Initialized() {
        return loaded.test();
    }

    bool OpenVrSystem::ThreadInit() {
        ZoneScoped;
        if (!vr::VR_IsRuntimeInstalled() || !vr::VR_IsHmdPresent()) {
            Logf("No VR HMD is present.");
            return false;
        }

        vr::EVRInitError err = vr::VRInitError_None;
        auto vrSystemPtr = vr::VR_Init(&err, vr::VRApplication_Scene);

        if (err == vr::VRInitError_None) {
            Tracef("OpenVrSystem initialized");
            vrSystem = std::shared_ptr<vr::IVRSystem>(vrSystemPtr, [this](auto *ptr) {
                Logf("Shutting down OpenVR");
                context->WaitIdle();
                vr::VR_Shutdown();
            });
            loaded.test_and_set();
            loaded.notify_all();
        } else {
            Errorf("Failed to load OpenVR system: %s", VR_GetVRInitErrorAsSymbol(err));
            Errorf("Run 'reloadxrsystem' in the console to try again.");
            return false;
        }

        // Initialize SteamVR Input subsystem
        std::string actionManifestPath = std::filesystem::absolute("actions.json").string();
        inputBindings = std::make_shared<InputBindings>(*this, actionManifestPath);

        RegisterModels();

        GetSceneManager().QueueActionAndBlock(SceneAction::ApplySystemScene,
            "vr_system",
            [this](ecs::Lock<ecs::AddRemove> lock, std::shared_ptr<Scene> scene) {
                auto vrOrigin = scene->NewSystemEntity(lock, scene, vrOriginEntity.Name());
                vrOrigin.Set<ecs::TransformTree>(lock);

                static const std::array specialEntities = {vrHmdEntity,
                    vrControllerLeftEntity,
                    vrControllerRightEntity};
                for (auto &namedEntity : specialEntities) {
                    auto ent = scene->NewSystemEntity(lock, scene, namedEntity.Name());
                    ent.Set<ecs::TransformTree>(lock);
                    ent.Set<ecs::EventBindings>(lock);
                }

                for (size_t i = 0; i < reservedEntities.size(); i++) {
                    reservedEntities[i] = ecs::Name("vr", "device" + std::to_string(i));
                }

                uint32_t vrWidth, vrHeight;
                vrSystem->GetRecommendedRenderTargetSize(&vrWidth, &vrHeight);
                Logf("OpenVR Render Target Size: %u x %u", vrWidth, vrHeight);

                for (size_t i = 0; i < views.size(); i++) {
                    auto eye = (ecs::XrEye)i;

                    auto ent = scene->NewSystemEntity(lock, scene, views[eye].Name());
                    ent.Set<ecs::XRView>(lock, eye);

                    auto &transform = ent.Set<ecs::TransformTree>(lock);
                    transform.parent = vrOriginEntity;

                    auto &view = ent.Set<ecs::View>(lock);
                    view.extents = {vrWidth, vrHeight};
                    view.clip = {0.1, 256};
                    auto projMatrix = vrSystem->GetProjectionMatrix(MapXrEyeToOpenVr(eye), view.clip.x, view.clip.y);
                    view.SetProjMat(glm::transpose(glm::make_mat4((float *)projMatrix.m)));
                    view.visibilityMask = ecs::VisibilityMask::DirectEye;
                }
            });

        GetSceneManager().QueueActionAndBlock(SceneAction::ApplySystemScene, "vr");

        return true;
    }

    bool OpenVrSystem::GetPredictedViewPose(ecs::XrEye eye, glm::mat4 &invViewMat) {
        if (!loaded.test() || !vrSystem) return false;

        float frameTimeRemaining = vr::VRCompositor()->GetFrameTimeRemaining();
        float vSyncToPhotons = vrSystem->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd,
            vr::Prop_SecondsFromVsyncToPhotons_Float);

        vr::TrackedDevicePose_t trackedDevicePoses[vr::k_unTrackedDeviceIndex_Hmd + 1];
        vrSystem->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseOrigin::TrackingUniverseStanding,
            frameTimeRemaining + vSyncToPhotons,
            trackedDevicePoses,
            vr::k_unTrackedDeviceIndex_Hmd + 1);

        if (trackedDevicePoses[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid) {
            glm::mat4 hmdPose = glm::mat4(glm::make_mat3x4(
                (float *)trackedDevicePoses[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking.m));

            vr::HmdMatrix34_t eyePosOvr = vrSystem->GetEyeToHeadTransform(MapXrEyeToOpenVr(eye));
            glm::mat4 eyeToHmdPose = glm::mat4(glm::make_mat3x4((float *)eyePosOvr.m));

            invViewMat = glm::transpose(eyeToHmdPose * hmdPose);

            Tracef("GetPredictedViewPose");
            return true;
        }

        return false;
    }

    void OpenVrSystem::WaitFrame() {
        ZoneScoped;
        if (!loaded.test()) return;

        vr::EVRCompositorError error = vr::VRCompositor()->WaitGetPoses(nullptr, 0, nullptr, 0);
        Assert(error == vr::EVRCompositorError::VRCompositorError_None,
            "WaitGetPoses failed: " + std::to_string((int)error));
    }

    ecs::Entity OpenVrSystem::GetEntityForDeviceIndex(size_t index) {
        if (index >= trackedDevices.size() || trackedDevices[index] == nullptr) return ecs::Entity();

        return trackedDevices[index]->GetLive();
    }

    void OpenVrSystem::Frame() {
        if (!vrSystem) return;

        ZoneScoped;
        eventHandler.Frame();

        vr::TrackedDevicePose_t trackedDevicePoses[vr::k_unMaxTrackedDeviceCount];
        vr::EVRCompositorError error = vr::VRCompositor()->GetLastPoses(trackedDevicePoses,
            vr::k_unMaxTrackedDeviceCount,
            NULL,
            0);
        if (error != vr::VRCompositorError_None) return;

        for (vr::TrackedDeviceIndex_t i = 0; i < vr::k_unMaxTrackedDeviceCount; i++) {
            if (vrSystem->IsTrackedDeviceConnected(i)) {
                auto deviceClass = vrSystem->GetTrackedDeviceClass(i);
                if (deviceClass == vr::TrackedDeviceClass_HMD && i == vr::k_unTrackedDeviceIndex_Hmd) {
                    trackedDevices[i] = &vrHmdEntity;
                } else if (deviceClass == vr::TrackedDeviceClass_Controller) {
                    auto role = vrSystem->GetControllerRoleForTrackedDeviceIndex(i);
                    if (role == vr::TrackedControllerRole_LeftHand) {
                        trackedDevices[i] = &vrControllerLeftEntity;
                    } else if (role == vr::TrackedControllerRole_RightHand) {
                        trackedDevices[i] = &vrControllerRightEntity;
                    } else {
                        trackedDevices[i] = &reservedEntities[i];
                    }
                } else if (deviceClass == vr::TrackedDeviceClass_GenericTracker) {
                    trackedDevices[i] = &reservedEntities[i];
                } else {
                    // Ignore tracking references and other devices
                    trackedDevices[i] = nullptr;
                }
            } else {
                trackedDevices[i] = nullptr;
            }
        }
        bool missingEntities = false;
        {
            ZoneScopedN("OpenVRSystem Sync to ECS");
            auto lock = ecs::StartTransaction<ecs::Read<ecs::Name>, ecs::Write<ecs::TransformTree>>();

            for (auto entityRef : trackedDevices) {
                if (entityRef && !entityRef->Get(lock).Exists(lock)) {
                    missingEntities = true;
                    break;
                }
            }

            for (vr::TrackedDeviceIndex_t i = 0; i < vr::k_unMaxTrackedDeviceCount; i++) {
                if (trackedDevices[i]) {
                    ecs::Entity ent = trackedDevices[i]->Get(lock);
                    if (ent.Has<ecs::TransformTree>(lock) && trackedDevicePoses[i].bPoseIsValid) {
                        auto &transform = ent.Get<ecs::TransformTree>(lock);
                        auto &pose = trackedDevicePoses[i].mDeviceToAbsoluteTracking.m;
                        transform.pose = glm::mat4(glm::transpose(glm::make_mat3x4((float *)pose)));
                        transform.parent = vrOriginEntity;
                    }
                }
            }
        }
        if (missingEntities) {
            ZoneScopedN("OpenVrSystem::AddMissingEntities");
            GetSceneManager().QueueActionAndBlock(SceneAction::ApplySystemScene,
                "vr_system",
                [this](ecs::Lock<ecs::AddRemove> lock, std::shared_ptr<Scene> scene) {
                    for (auto *entityRef : trackedDevices) {
                        if (entityRef && !scene->GetStagingEntity(entityRef->Name())) {
                            auto ent = scene->NewSystemEntity(lock, scene, entityRef->Name());
                            ent.Set<ecs::TransformTree>(lock);
                            ent.Set<ecs::EventBindings>(lock);
                        }
                    }
                });
        }
        if (inputBindings) inputBindings->Frame();
    }

    HiddenAreaMesh OpenVrSystem::GetHiddenAreaMesh(ecs::XrEye eye) {
        if (!vrSystem || !loaded.test()) return {};

        static_assert(sizeof(*vr::HiddenAreaMesh_t::pVertexData) == sizeof(*HiddenAreaMesh::vertices));
        auto mesh = vrSystem->GetHiddenAreaMesh(MapXrEyeToOpenVr(eye));
        return {(const glm::vec2 *)mesh.pVertexData, mesh.unTriangleCount};
    }

    void OpenVrSystem::RegisterModels() {
        auto modelPathLen = vr::VRResources()->GetResourceFullPath("vr_glove_left_model.glb",
            "rendermodels/vr_glove/",
            NULL,
            0);
        std::vector<char> modelPathStr(modelPathLen);
        vr::VRResources()->GetResourceFullPath("vr_glove_left_model.glb",
            "rendermodels/vr_glove/",
            modelPathStr.data(),
            modelPathStr.size());
        Assets().RegisterExternalGltf("vr_glove_left", modelPathStr.data());

        modelPathLen = vr::VRResources()->GetResourceFullPath("vr_glove_right_model.glb",
            "rendermodels/vr_glove/",
            NULL,
            0);
        modelPathStr.resize(modelPathLen);
        vr::VRResources()->GetResourceFullPath("vr_glove_right_model.glb",
            "rendermodels/vr_glove/",
            modelPathStr.data(),
            modelPathStr.size());
        Assets().RegisterExternalGltf("vr_glove_right", modelPathStr.data());
    }
} // namespace sp::xr
