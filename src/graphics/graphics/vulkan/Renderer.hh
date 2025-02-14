#pragma once

#include "assets/Async.hh"
#include "console/CFunc.hh"
#include "ecs/Ecs.hh"
#include "graphics/vulkan/core/Memory.hh"
#include "graphics/vulkan/core/VkCommon.hh"
#include "graphics/vulkan/render_graph/RenderGraph.hh"
#include "graphics/vulkan/render_passes/Emissive.hh"
#include "graphics/vulkan/render_passes/Lighting.hh"
#include "graphics/vulkan/render_passes/SMAA.hh"
#include "graphics/vulkan/render_passes/Screenshots.hh"
#include "graphics/vulkan/render_passes/Transparency.hh"
#include "graphics/vulkan/render_passes/Voxels.hh"
#include "graphics/vulkan/scene/GPUScene.hh"

#include <atomic>
#include <functional>
#include <robin_hood.h>

namespace sp {
    class GuiContext;

#ifdef SP_XR_SUPPORT
    namespace xr {
        class XrSystem;
    }
#endif
} // namespace sp

namespace sp::vulkan {
    class Mesh;
    class GuiRenderer;

    extern CVar<string> CVarWindowViewTarget;

    class Renderer {
    public:
        Renderer(DeviceContext &context);
        ~Renderer();

        void RenderFrame(chrono_clock::duration elapsedTime);
        void EndFrame();

        void SetDebugGui(GuiContext *gui);
        void SetMenuGui(GuiContext *gui);

#ifdef SP_XR_SUPPORT
        void SetXRSystem(shared_ptr<xr::XrSystem> xr) {
            xrSystem = xr;
        }
#endif

    private:
        DeviceContext &device;
        rg::RenderGraph graph;

        void BuildFrameGraph(chrono_clock::duration elapsedTime);
        ecs::View AddFlatView(ecs::Lock<ecs::Read<ecs::TransformSnapshot, ecs::View>> lock);
        void AddWindowOutput();

#ifdef SP_XR_SUPPORT
        ecs::View AddXRView(ecs::Lock<ecs::Read<ecs::TransformSnapshot, ecs::View, ecs::XRView>> lock);
        void AddXRSubmit(ecs::Lock<ecs::Read<ecs::XRView>> lock);
#endif

        void AddGui(ecs::Entity ent, const ecs::Gui &gui);
        void AddWorldGuis(ecs::Lock<ecs::Read<ecs::TransformSnapshot, ecs::Gui, ecs::Screen, ecs::Name>> lock);
        void AddMenuGui(ecs::Lock<ecs::Read<ecs::View>> lock);
        void AddDeferredPasses(ecs::Lock<ecs::Read<ecs::TransformSnapshot, ecs::Screen, ecs::Gui, ecs::LaserLine>> lock,
            const ecs::View &view,
            chrono_clock::duration elapsedTime);
        void AddMenuOverlay();

        CFuncCollection funcs;
        vk::Format depthStencilFormat;

        GPUScene scene;

        renderer::Voxels voxels;
        renderer::Lighting lighting;
        renderer::Transparency transparency;
        renderer::Emissive emissive;
        renderer::SMAA smaa;
        renderer::Screenshots screenshots;

        unique_ptr<GuiRenderer> guiRenderer;
        struct RenderableGui {
            ecs::Entity entity;
            GuiContext *context;
            shared_ptr<GuiContext> contextShared;
            rg::ResourceID renderGraphID = rg::InvalidResource;
        };
        vector<RenderableGui> guis;
        GuiContext *debugGui = nullptr, *menuGui = nullptr;

        ecs::ComponentObserver<ecs::Gui> guiObserver;

        bool listImages = false;

#ifdef SP_XR_SUPPORT
        shared_ptr<xr::XrSystem> xrSystem;
        std::vector<glm::mat4> xrRenderPoses;
        std::array<BufferPtr, 2> hiddenAreaMesh;
        std::array<uint32, 2> hiddenAreaTriangleCount;
#endif
    };
} // namespace sp::vulkan
