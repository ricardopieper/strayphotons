#pragma once

#include "core/CFunc.hh"
#include "ecs/Ecs.hh"
#include "graphics/core/RenderTarget.hh"

#include <functional>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

namespace sp {
    class Game;
    class Model;
    class GraphicsContext;
    class VulkanGraphicsContext;

    class VulkanRenderer {
    public:
        using DrawLock = typename ecs::Lock<ecs::Read<ecs::Renderable, ecs::View, ecs::Transform>>;
        typedef std::function<void(DrawLock, Tecs::Entity &)> PreDrawFunc;

        VulkanRenderer(ecs::Lock<ecs::AddRemove> lock, VulkanGraphicsContext &context);
        ~VulkanRenderer();

        void Prepare(){};
        void BeginFrame(ecs::Lock<ecs::Read<ecs::Transform>> lock){};
        void RenderPass(const ecs::View &view, DrawLock lock, RenderTarget *finalOutput = nullptr);
        void EndFrame();

        void ForwardPass(vk::CommandBuffer &commands, ecs::View &view, DrawLock lock, const PreDrawFunc &preDraw = {});
        void DrawEntity(vk::CommandBuffer &commands,
                        const ecs::View &view,
                        DrawLock lock,
                        Tecs::Entity &ent,
                        const PreDrawFunc &preDraw = {});

        void CreatePipeline(vk::Extent2D extent);
        void CleanupPipeline();

        float Exposure = 1.0f;

        VulkanGraphicsContext &context;
        vk::Device &device;

    private:
        CFuncCollection funcs;

        vk::UniqueCommandPool commandPool;
        vector<vk::UniqueCommandBuffer> commandBuffers;

        // test pipeline
        uint32_t pipelineSwapchainVersion = 0;
        vk::UniqueRenderPass renderPass;
        vk::UniquePipelineLayout pipelineLayout;
        vk::UniquePipeline graphicsPipeline;
        vector<vk::UniqueFramebuffer> swapchainFramebuffers;
    };
} // namespace sp
