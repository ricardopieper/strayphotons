#include "CommandContext.hh"

#include "graphics/vulkan/core/DeviceContext.hh"

namespace sp::vulkan {
    CommandContext::CommandContext(DeviceContext &device, vk::UniqueCommandBuffer cmd, CommandContextType type) noexcept
        : device(device), cmd(std::move(cmd)), type(type) {
        SetDefaultOpaqueState();
    }

    CommandContext::~CommandContext() {
        Assert(!recording, "command context was never ended/submitted");
    }

    void CommandContext::SetDefaultOpaqueState() {
        SetDepthTest(true, true);
        SetDepthRange(0.0f, 1.0f);
        SetStencilTest(false);
        SetBlending(false);
        SetBlendFunc(vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha);
        SetCullMode(vk::CullModeFlagBits::eBack);
        SetFrontFaceWinding(vk::FrontFace::eCounterClockwise);
        SetPrimitiveTopology(vk::PrimitiveTopology::eTriangleList);
    }

    void CommandContext::Reset() {
        dirty = ~DirtyFlags();
        dirtyDescriptorSets = ~0u;
        currentPipeline = VK_NULL_HANDLE;
        pipelineInput.state.shaders = {};
        pipelineInput.renderPass = VK_NULL_HANDLE;
        framebuffer.reset();
        renderPass.reset();
    }

    void CommandContext::BeginRenderPass(const RenderPassInfo &info) {
        Reset();
        Assert(!framebuffer, "render pass already started");

        framebuffer = device.GetFramebuffer(info);
        pipelineInput.renderPass = framebuffer->GetRenderPass();
        renderPass = device.GetRenderPass(info);

        viewport = vk::Rect2D{{0, 0}, framebuffer->Extent()};
        scissor = viewport;

        vk::ClearValue clearValues[MAX_COLOR_ATTACHMENTS + 1];
        uint32 clearValueCount = info.state.colorAttachmentCount;

        for (uint32 i = 0; i < info.state.colorAttachmentCount; i++) {
            if (info.state.ShouldClear(i)) { clearValues[i].color = info.clearColors[i]; }
            if (info.colorAttachments[i]->IsSwapchain()) writesToSwapchain = true;
        }

        if (info.HasDepthStencil() && info.state.ShouldClear(RenderPassState::DEPTH_STENCIL_INDEX)) {
            clearValues[info.state.colorAttachmentCount].depthStencil = info.clearDepthStencil;
            clearValueCount = info.state.colorAttachmentCount + 1;
        }

        vk::RenderPassBeginInfo renderPassBeginInfo;
        renderPassBeginInfo.renderPass = *renderPass;
        renderPassBeginInfo.framebuffer = *framebuffer;
        renderPassBeginInfo.renderArea = scissor;
        renderPassBeginInfo.clearValueCount = clearValueCount;
        renderPassBeginInfo.pClearValues = clearValues;
        cmd->beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

        renderPass->RecordImplicitImageLayoutTransitions(info);
    }

    void CommandContext::EndRenderPass() {
        Assert(!!framebuffer, "render pass not started");

        cmd->endRenderPass();
        Reset();
    }

    void CommandContext::Begin() {
        Assert(!recording, "command buffer already recording");
        vk::CommandBufferBeginInfo beginInfo;
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        cmd->begin(beginInfo);
        recording = true;
    }

    void CommandContext::End() {
        Assert(recording, "command buffer not recording");
        cmd->end();
        recording = false;
    }

    void CommandContext::Abandon() {
        if (recording) {
            cmd->end();
            recording = false;
            abandoned = true;
        }
    }

    void CommandContext::Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ) {
        FlushComputeState();
        cmd->dispatch(groupCountX, groupCountY, groupCountZ);
    }

    void CommandContext::Draw(uint32 vertexes, uint32 instances, int32 firstVertex, uint32 firstInstance) {
        FlushGraphicsState();
        cmd->draw(vertexes, instances, firstVertex, firstInstance);
    }

    void CommandContext::DrawIndexed(uint32 indexes,
        uint32 instances,
        uint32 firstIndex,
        int32 vertexOffset,
        uint32 firstInstance) {
        FlushGraphicsState();
        cmd->drawIndexed(indexes, instances, firstIndex, vertexOffset, firstInstance);
    }

    void CommandContext::DrawIndirect(BufferPtr drawCommands, vk::DeviceSize offset, uint32 drawCount, uint32 stride) {
        FlushGraphicsState();
        cmd->drawIndirect(*drawCommands, offset, drawCount, stride);
    }

    void CommandContext::DrawIndirectCount(BufferPtr drawCommands,
        vk::DeviceSize offset,
        BufferPtr countBuffer,
        vk::DeviceSize countOffset,
        uint32 maxDrawCount,
        uint32 stride) {
        FlushGraphicsState();
        cmd->drawIndirectCount(*drawCommands, offset, *countBuffer, countOffset, maxDrawCount, stride);
    }

    void CommandContext::DrawIndexedIndirect(BufferPtr drawCommands,
        vk::DeviceSize offset,
        uint32 drawCount,
        uint32 stride) {
        FlushGraphicsState();
        cmd->drawIndexedIndirect(*drawCommands, offset, drawCount, stride);
    }

    void CommandContext::DrawIndexedIndirectCount(BufferPtr drawCommands,
        vk::DeviceSize offset,
        BufferPtr countBuffer,
        vk::DeviceSize countOffset,
        uint32 maxDrawCount,
        uint32 stride) {
        FlushGraphicsState();
        cmd->drawIndexedIndirectCount(*drawCommands, offset, *countBuffer, countOffset, maxDrawCount, stride);
    }

    void CommandContext::DrawScreenCover(const ImageViewPtr &view) {
        SetShaders("screen_cover.vert", "screen_cover.frag");
        if (view) {
            SetTexture(0, 0, view);
            if (view->ViewType() == vk::ImageViewType::e2DArray) {
                SetSingleShader(ShaderStage::Fragment, "screen_cover_array.frag");
            }
        }
        Draw(3); // vertices are defined as constants in the vertex shader
    }

    void CommandContext::ImageBarrier(const ImagePtr &image,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        vk::PipelineStageFlags srcStages,
        vk::AccessFlags srcAccess,
        vk::PipelineStageFlags dstStages,
        vk::AccessFlags dstAccess,
        const ImageBarrierInfo &options) {
        vk::ImageMemoryBarrier barrier;
        barrier.image = *image;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcAccessMask = srcAccess;
        barrier.dstAccessMask = dstAccess;
        barrier.subresourceRange.aspectMask = FormatToAspectFlags(image->Format());
        barrier.subresourceRange.baseMipLevel = options.baseMipLevel;
        barrier.subresourceRange.levelCount = options.mipLevelCount ? options.mipLevelCount : image->MipLevels();
        barrier.subresourceRange.baseArrayLayer = options.baseArrayLayer;
        barrier.subresourceRange.layerCount = options.arrayLayerCount ? options.arrayLayerCount : image->ArrayLayers();
        barrier.srcQueueFamilyIndex = options.srcQueueFamilyIndex;
        barrier.dstQueueFamilyIndex = options.dstQueueFamilyIndex;
        cmd->pipelineBarrier(srcStages, dstStages, {}, {}, {}, {barrier});

        if (options.trackImageLayout) {
            Assert(
                !options.baseMipLevel && !options.mipLevelCount && !options.baseArrayLayer && !options.arrayLayerCount,
                "can't track image layout when specifying a subresource range");
            image->SetLayout(oldLayout, newLayout);
        }
    }

    void CommandContext::SetShaders(std::initializer_list<std::pair<ShaderStage, string_view>> shaders) {
        pipelineInput.state.shaders = {};
        for (auto &s : shaders) {
            SetSingleShader(s.first, s.second);
        }
    }

    void CommandContext::SetShaders(string_view vertName, string_view fragName) {
        pipelineInput.state.shaders = {};
        SetSingleShader(ShaderStage::Vertex, vertName);
        SetSingleShader(ShaderStage::Fragment, fragName);
    }

    void CommandContext::SetComputeShader(string_view name) {
        pipelineInput.state.shaders = {};
        SetSingleShader(ShaderStage::Compute, name);
    }

    void CommandContext::SetSingleShader(ShaderStage stage, ShaderHandle handle) {
        auto &slot = pipelineInput.state.shaders[(size_t)stage];
        if (slot == handle) return;
        slot = handle;

        auto &spec = pipelineInput.state.specializations[(size_t)stage];
        std::fill(spec.values.begin(), spec.values.end(), 0);
        spec.set.reset();
        SetDirty(DirtyBits::Pipeline);
    }

    void CommandContext::SetSingleShader(ShaderStage stage, string_view name) {
        SetSingleShader(stage, device.LoadShader(name));
    }

    void CommandContext::SetShaderConstant(ShaderStage stage, uint32 index, uint32 data) {
        Assert(pipelineInput.state.shaders[(size_t)stage], "no shader bound to set constant");
        auto &spec = pipelineInput.state.specializations[(size_t)stage];
        spec.values[index] = data;
        spec.set.set(index, true);
        SetDirty(DirtyBits::Pipeline);
    }

    void CommandContext::PushConstants(const void *data, VkDeviceSize offset, VkDeviceSize range) {
        Assert(offset + range <= sizeof(shaderData.pushConstants), "CommandContext::PushConstants overflow");
        memcpy(shaderData.pushConstants + offset, data, range);
        SetDirty(DirtyBits::PushConstants);
    }

    void CommandContext::SetSampler(uint32 set, uint32 binding, const vk::Sampler &sampler) {
        Assert(set < MAX_BOUND_DESCRIPTOR_SETS, "descriptor set index too high");
        Assert(binding < MAX_BINDINGS_PER_DESCRIPTOR_SET, "binding index too high");
        auto &image = shaderData.sets[set].bindings[binding].image;
        image.sampler = sampler;
        SetDescriptorDirty(set);
    }

    void CommandContext::SetTexture(uint32 set, uint32 binding, const ImageViewPtr &view) {
        SetTexture(set, binding, view.get());
    }

    void CommandContext::SetTexture(uint32 set, uint32 binding, const ImageView *view) {
        Assert(set < MAX_BOUND_DESCRIPTOR_SETS, "descriptor set index too high");
        Assert(binding < MAX_BINDINGS_PER_DESCRIPTOR_SET, "binding index too high");
        auto &bindingDesc = shaderData.sets[set].bindings[binding];
        bindingDesc.uniqueID = view->GetUniqueID();

        auto &image = bindingDesc.image;
        image.imageView = **view;
        image.imageLayout = (VkImageLayout)view->Image()->LastLayout();
        SetDescriptorDirty(set);

        auto defaultSampler = view->DefaultSampler();
        if (defaultSampler) SetSampler(set, binding, defaultSampler);
    }

    void CommandContext::SetUniformBuffer(uint32 set, uint32 binding, const BufferPtr &buffer) {
        Assert(set < MAX_BOUND_DESCRIPTOR_SETS, "descriptor set index too high");
        Assert(binding < MAX_BINDINGS_PER_DESCRIPTOR_SET, "binding index too high");
        auto &bindingDesc = shaderData.sets[set].bindings[binding];
        bindingDesc.uniqueID = buffer->GetUniqueID();

        auto &bufferBinding = bindingDesc.buffer;
        bufferBinding.buffer = **buffer;
        bufferBinding.offset = 0;
        bufferBinding.range = buffer->Size();
        SetDescriptorDirty(set);
    }

    void CommandContext::SetStorageBuffer(uint32 set, uint32 binding, const BufferPtr &buffer) {
        Assert(set < MAX_BOUND_DESCRIPTOR_SETS, "descriptor set index too high");
        Assert(binding < MAX_BINDINGS_PER_DESCRIPTOR_SET, "binding index too high");
        auto &bindingDesc = shaderData.sets[set].bindings[binding];
        bindingDesc.uniqueID = buffer->GetUniqueID();

        auto &bufferBinding = bindingDesc.buffer;
        bufferBinding.buffer = **buffer;
        bufferBinding.offset = 0;
        bufferBinding.range = buffer->Size();
        SetDescriptorDirty(set);
    }

    BufferPtr CommandContext::AllocUniformBuffer(uint32 set, uint32 binding, vk::DeviceSize size) {
        auto buffer = device.GetFramePooledBuffer(BUFFER_TYPE_UNIFORM, size);
        SetUniformBuffer(set, binding, buffer);
        return buffer;
    }

    void CommandContext::SetBindlessDescriptors(uint32 set, vk::DescriptorSet descriptorSet) {
        Assert(set < MAX_BOUND_DESCRIPTOR_SETS, "descriptor set index too high");
        bindlessSets[set] = descriptorSet;
        SetDescriptorDirty(set);
    }

    void CommandContext::FlushDescriptorSets(vk::PipelineBindPoint bindPoint) {
        auto layout = currentPipeline->GetLayout();

        for (uint32 set = 0; set < MAX_BOUND_DESCRIPTOR_SETS; set++) {
            if (!ResetDescriptorDirty(set)) continue;
            if (!layout->HasDescriptorSet(set)) continue;

            vk::DescriptorSet descriptorSet;
            if (layout->IsBindlessSet(set)) {
                descriptorSet = bindlessSets[set];
            } else {
                descriptorSet = layout->GetFilledDescriptorSet(set, shaderData.sets[set]);
            }

            cmd->bindDescriptorSets(bindPoint, *layout, set, {descriptorSet}, nullptr);
        }
    }

    void CommandContext::FlushPushConstants() {
        auto pipelineLayout = currentPipeline->GetLayout();
        auto &layoutInfo = pipelineLayout->Info();

        if (ResetDirty(DirtyBits::PushConstants)) {
            auto &range = layoutInfo.pushConstantRange;
            if (range.stageFlags) {
                Assert(range.offset == 0, "push constant range must start at 0");
                cmd->pushConstants(*pipelineLayout, range.stageFlags, 0, range.size, shaderData.pushConstants);
            }
        }
    }

    void CommandContext::FlushComputeState() {
        if (ResetDirty(DirtyBits::Pipeline)) {
            auto pipeline = device.GetPipeline(pipelineInput);
            if (pipeline != currentPipeline) { cmd->bindPipeline(vk::PipelineBindPoint::eCompute, *pipeline); }
            currentPipeline = pipeline;
        }

        FlushPushConstants();
        FlushDescriptorSets(vk::PipelineBindPoint::eCompute);
    }

    void CommandContext::FlushGraphicsState() {
        if (ResetDirty(DirtyBits::Pipeline)) {
            auto pipeline = device.GetPipeline(pipelineInput);
            if (pipeline != currentPipeline) { cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline); }
            currentPipeline = pipeline;
        }

        if (ResetDirty(DirtyBits::Viewport)) {
            vk::Viewport vp = {(float)viewport.offset.x,
                (float)viewport.offset.y,
                (float)viewport.extent.width,
                (float)viewport.extent.height,
                minDepth,
                maxDepth};

            if (viewportYDirection == YDirection::Up) {
                // Negative height sets viewport coordinates to OpenGL style (Y up)
                vp.y = framebuffer->Extent().height - vp.y;
                vp.height = -vp.height;
            }
            cmd->setViewport(0, 1, &vp);
        }

        if (ResetDirty(DirtyBits::Scissor)) {
            vk::Rect2D sc = scissor;
            sc.offset.y = framebuffer->Extent().height - sc.offset.y - sc.extent.height;
            cmd->setScissor(0, 1, &sc);
        }

        if (pipelineInput.state.stencilTest && ResetDirty(DirtyBits::Stencil)) {
            const auto &front = stencilState[0];
            const auto &back = stencilState[1];
            if (front.writeMask == back.writeMask) {
                cmd->setStencilWriteMask(vk::StencilFaceFlagBits::eFrontAndBack, front.writeMask);
            } else {
                cmd->setStencilWriteMask(vk::StencilFaceFlagBits::eFront, front.writeMask);
                cmd->setStencilWriteMask(vk::StencilFaceFlagBits::eBack, back.writeMask);
            }
            if (front.compareMask == back.compareMask) {
                cmd->setStencilCompareMask(vk::StencilFaceFlagBits::eFrontAndBack, front.compareMask);
            } else {
                cmd->setStencilCompareMask(vk::StencilFaceFlagBits::eFront, front.compareMask);
                cmd->setStencilCompareMask(vk::StencilFaceFlagBits::eBack, back.compareMask);
            }
            if (front.reference == back.reference) {
                cmd->setStencilReference(vk::StencilFaceFlagBits::eFrontAndBack, front.reference);
            } else {
                cmd->setStencilReference(vk::StencilFaceFlagBits::eFront, front.reference);
                cmd->setStencilReference(vk::StencilFaceFlagBits::eBack, back.reference);
            }
        }

        FlushPushConstants();
        FlushDescriptorSets(vk::PipelineBindPoint::eGraphics);
    }
} // namespace sp::vulkan
