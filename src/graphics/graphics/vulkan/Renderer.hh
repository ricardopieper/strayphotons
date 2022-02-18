#pragma once

#include "assets/Async.hh"
#include "console/CFunc.hh"
#include "core/LockFreeMutex.hh"
#include "core/PreservingMap.hh"
#include "ecs/Ecs.hh"
#include "graphics/core/RenderTarget.hh"
#include "graphics/vulkan/GPUSceneContext.hh"
#include "graphics/vulkan/GPUTypes.hh"
#include "graphics/vulkan/core/Common.hh"
#include "graphics/vulkan/core/Memory.hh"
#include "graphics/vulkan/core/PerfTimer.hh"
#include "graphics/vulkan/core/RenderGraph.hh"

#include <atomic>
#include <functional>
#include <robin_hood.h>

namespace sp {
    class GuiManager;

#ifdef SP_XR_SUPPORT
    namespace xr {
        class XrSystem;
    }
#endif
} // namespace sp

namespace sp::vulkan {
    class Model;
    class GuiRenderer;
    class RenderGraph;
    class RenderGraphResources;

    struct LightingContext {
        int count = 0;
        glm::ivec2 renderTargetSize = {};
        ecs::View views[MAX_LIGHTS];

        int gelCount = 0;
        string gelNames[MAX_LIGHT_GELS];

        struct GPUData {
            GPULight lights[MAX_LIGHTS];
            int count;
            float padding[3];
        } gpuData;
    };

    struct LaserContext {
        struct GPULine {
            glm::vec3 color;
            float padding0[1];
            glm::vec3 start;
            float padding1[1];
            glm::vec3 end;
            float padding2[1];
        };
        static_assert(sizeof(GPULine) % sizeof(glm::vec4) == 0, "std430 alignment");

        vector<GPULine> gpuData;
    };

    class Renderer {
    public:
        using DrawLock = typename ecs::Lock<ecs::Read<ecs::Renderable, ecs::Light, ecs::TransformSnapshot>>;
        typedef std::function<void(DrawLock, Tecs::Entity &)> PreDrawFunc;

        Renderer(DeviceContext &context, PerfTimer &timer);
        ~Renderer();

        void RenderFrame();
        void EndFrame();

        struct DrawBufferIDs {
            RenderGraphResourceID drawCommandsBuffer; // first 4 bytes are the number of draws
            RenderGraphResourceID drawParamsBuffer;
        };

        DrawBufferIDs GenerateDrawsForView(ecs::Renderable::VisibilityMask viewMask);
        void DrawSceneIndirect(CommandContext &cmd,
            RenderGraphResources &resources,
            BufferPtr drawCommandsBuffer,
            BufferPtr drawParamsBuffer);

        void ForwardPass(CommandContext &cmd,
            ecs::Renderable::VisibilityMask viewMask,
            DrawLock lock,
            bool useMaterial = true,
            const PreDrawFunc &preDraw = {});

        void DrawEntity(CommandContext &cmd,
            ecs::Renderable::VisibilityMask viewMask,
            DrawLock lock,
            Tecs::Entity &ent,
            bool useMaterial = true,
            const PreDrawFunc &preDraw = {});

        float Exposure = 1.0f;

        void SetDebugGui(GuiManager &gui);

#ifdef SP_XR_SUPPORT
        void SetXRSystem(shared_ptr<xr::XrSystem> xr) {
            xrSystem = xr;
        }
#endif

        void QueueScreenshot(const string &path, const string &resource);

        ImageViewPtr GetBlankPixelImage();
        ImageViewPtr CreateSinglePixelImage(glm::vec4 value);

    private:
        DeviceContext &device;
        PerfTimer &timer;
        RenderGraph graph;

        void BuildFrameGraph();
        void AddFlatView(ecs::Lock<ecs::Read<ecs::TransformSnapshot, ecs::View>> lock);
        void AddWindowOutput();

#ifdef SP_XR_SUPPORT
        void AddXRView(ecs::Lock<ecs::Read<ecs::TransformSnapshot, ecs::View, ecs::XRView>> lock);
        void AddXRSubmit(ecs::Lock<ecs::Read<ecs::XRView>> lock);
#endif

        void AddScreenshots();
        RenderGraphResourceID VisualizeBuffer(RenderGraphResourceID sourceID, uint32 arrayLayer = ~0u);

        void AddSceneState(ecs::Lock<ecs::Read<ecs::Renderable, ecs::TransformSnapshot>> lock);
        void AddGeometryWarp();
        void AddLaserState(ecs::Lock<ecs::Read<ecs::LaserLine, ecs::TransformSnapshot>> lock);
        void AddLightState(ecs::Lock<ecs::Read<ecs::Light, ecs::TransformSnapshot>> lock);
        void AddShadowMaps(DrawLock lock);
        void AddGuis(ecs::Lock<ecs::Read<ecs::Gui>> lock);
        void AddDeferredPasses(ecs::Lock<ecs::Read<ecs::TransformSnapshot, ecs::Screen>> lock);
        void AddLighting();
        void AddEmissive(ecs::Lock<ecs::Read<ecs::Screen, ecs::TransformSnapshot>> lock);
        void AddTonemap();
        void AddMenuOverlay();

        RenderGraphResourceID AddBloom();
        RenderGraphResourceID AddGaussianBlur(RenderGraphResourceID sourceID,
            glm::ivec2 direction,
            uint32 downsample = 1,
            float scale = 1.0f,
            float clip = FLT_MAX);

        CFuncCollection funcs;

        LightingContext lights;
        LaserContext lasers;
        GPUSceneContext scene;
        PreservingMap<string, Model> activeModels;
        vector<std::shared_ptr<const sp::Model>> modelsToLoad;

        struct RenderableGui {
            Tecs::Entity entity;
            shared_ptr<GuiRenderer> renderer;
            RenderGraphResourceID renderGraphID = ~0u;
        };
        vector<RenderableGui> guis;
        shared_ptr<GuiRenderer> debugGuiRenderer;

        ecs::ComponentObserver<ecs::Gui> guiObserver;

        LockFreeMutex screenshotMutex;
        vector<std::pair<string, string>> pendingScreenshots;
        bool listRenderTargets = false;

        struct EmptyImageKey {
            vk::Format format;
        };

        ImageViewPtr blankPixelImage;

        PreservingMap<ImageView *, ImageView> debugViews;

        std::atomic_flag sceneReady, pendingTransaction;

#ifdef SP_XR_SUPPORT
        shared_ptr<xr::XrSystem> xrSystem;
        std::vector<glm::mat4> xrRenderPoses;
        std::array<BufferPtr, 2> hiddenAreaMesh;
        std::array<uint32, 2> hiddenAreaTriangleCount;
#endif
    };
} // namespace sp::vulkan
