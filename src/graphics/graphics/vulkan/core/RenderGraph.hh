#pragma once

#include "core/Hashing.hh"
#include "core/InlineVector.hh"
#include "graphics/vulkan/core/Common.hh"
#include "graphics/vulkan/core/RenderPass.hh"
#include "graphics/vulkan/core/RenderTarget.hh"

#include <robin_hood.h>
#include <variant>

namespace sp::vulkan {
    const uint32 MAX_RESOURCE_SCOPES = sizeof(uint8);
    const uint32 MAX_RESOURCE_SCOPE_DEPTH = 4;

    struct RenderGraphResource {
        enum class Type {
            Undefined,
            RenderTarget,
        };

        RenderGraphResource() {}
        RenderGraphResource(RenderTargetDesc desc) : type(Type::RenderTarget), renderTargetDesc(desc) {}

        operator bool() const {
            return type != Type::Undefined;
        }

        RenderGraphResourceID id = ~0u;
        Type type = Type::Undefined;
        union {
            RenderTargetDesc renderTargetDesc;
        };
    };

    struct RenderGraphResourceAccess {
        vk::PipelineStageFlags stages = {};
        vk::AccessFlags access = {};
        vk::ImageLayout layout = vk::ImageLayout::eUndefined;
    };

    struct AttachmentInfo {
        AttachmentInfo() {}
        AttachmentInfo(LoadOp loadOp, StoreOp storeOp) : loadOp(loadOp), storeOp(storeOp) {}
        LoadOp loadOp = LoadOp::DontCare;
        StoreOp storeOp = StoreOp::DontCare;
        vk::ClearColorValue clearColor = {};
        vk::ClearDepthStencilValue clearDepthStencil = {1.0f, 0};

        void SetClearColor(glm::vec4 clear) {
            std::array<float, 4> clearValues = {clear.r, clear.g, clear.b, clear.a};
            clearColor = {clearValues};
        }

    private:
        friend class RenderGraph;
        friend class RenderGraphPassBuilder;
        RenderGraphResourceID resourceID = ~0u;
    };

    class RenderGraphResources {
    public:
        RenderGraphResources(DeviceContext &device) : device(device) {
            nameScopes.emplace_back();
            scopeStack.push_back(0);
        }

        RenderTargetPtr GetRenderTarget(RenderGraphResourceID id);
        RenderTargetPtr GetRenderTarget(string_view name);

        const RenderGraphResource &GetResourceByName(string_view name) const;
        const RenderGraphResource &GetResourceByID(RenderGraphResourceID id) const;
        RenderGraphResourceID GetID(string_view name) const;

    private:
        friend class RenderGraph;
        friend class RenderGraphPassBuilder;

        void ResizeBeforeExecute();
        uint32 RefCount(RenderGraphResourceID id);
        void IncrementRef(RenderGraphResourceID id);
        void DecrementRef(RenderGraphResourceID id);

        void Register(string_view name, RenderGraphResource &resource);

        RenderGraphResource &GetResourceRef(RenderGraphResourceID id) {
            Assert(id < resources.size(), "resource ID " + std::to_string(id) + " is invalid");
            return resources[id];
        }

        DeviceContext &device;

        struct Scope {
            string name;
            robin_hood::unordered_flat_map<string, RenderGraphResourceID, StringHash, StringEqual> resourceNames;
            static const RenderGraphResourceID npos = ~0u;

            RenderGraphResourceID GetID(string_view name) const;
            void SetID(string_view name, RenderGraphResourceID id);
        };
        vector<Scope> nameScopes;
        InlineVector<uint8, MAX_RESOURCE_SCOPE_DEPTH> scopeStack; // refers to indexes in nameScopes

        vector<RenderGraphResource> resources;

        // Built during execution
        vector<uint32> refCounts;
        vector<RenderTargetPtr> renderTargets;
    };

    struct RenderGraphResourceDependency {
        RenderGraphResourceAccess access;
        RenderGraphResourceID id;
    };

    class RenderGraphPass {
    public:
        RenderGraphPass(string_view name) : name(name) {}

        void AddDependency(const RenderGraphResourceAccess &access, const RenderGraphResource &res) {
            dependencies.push_back({access, res.id});
        }

        void AddOutput(const RenderGraphResource &res) {
            outputs.push_back(res.id);
        }

        bool HasExecute() const {
            return executeFunc.index() > 0;
        }
        bool ExecutesWithCommandContext() const {
            return executeFunc.index() == 1;
        }
        bool ExecutesWithDeviceContext() const {
            return executeFunc.index() == 2;
        }
        void Execute(RenderGraphResources &resources, CommandContext &cmd) const {
            std::get<1>(executeFunc)(resources, cmd);
        }
        void Execute(RenderGraphResources &resources, DeviceContext &device) const {
            std::get<2>(executeFunc)(resources, device);
        }

    private:
        friend class RenderGraph;
        friend class RenderGraphPassBuilder;
        string_view name;
        InlineVector<RenderGraphResourceDependency, 16> dependencies;
        InlineVector<RenderGraphResourceID, 16> outputs;
        std::array<AttachmentInfo, MAX_COLOR_ATTACHMENTS + 1> attachments;
        bool active = false, required = false;

        std::variant<std::monostate,
            std::function<void(RenderGraphResources &, CommandContext &)>,
            std::function<void(RenderGraphResources &, DeviceContext &)>>
            executeFunc;

        InlineVector<uint8, MAX_RESOURCE_SCOPE_DEPTH> scopes;
    };

    class RenderGraphPassBuilder {
    public:
        RenderGraphPassBuilder(RenderGraphResources &resources, RenderGraphPass &pass)
            : resources(resources), pass(pass) {}

        const RenderGraphResource &ShaderRead(string_view name) {
            return ShaderRead(resources.GetID(name));
        }
        const RenderGraphResource &ShaderRead(RenderGraphResourceID id) {
            auto &resource = resources.GetResourceRef(id);
            resource.renderTargetDesc.usage |= vk::ImageUsageFlagBits::eSampled;
            RenderGraphResourceAccess access = {
                vk::PipelineStageFlagBits::eFragmentShader,
                vk::AccessFlagBits::eShaderRead,
                vk::ImageLayout::eShaderReadOnlyOptimal,
            };
            pass.AddDependency(access, resource);
            return resource;
        }

        const RenderGraphResource &TransferRead(string_view name) {
            return TransferRead(resources.GetID(name));
        }
        const RenderGraphResource &TransferRead(RenderGraphResourceID id) {
            auto &resource = resources.GetResourceRef(id);
            resource.renderTargetDesc.usage |= vk::ImageUsageFlagBits::eTransferSrc;
            RenderGraphResourceAccess access = {
                vk::PipelineStageFlagBits::eTransfer,
                vk::AccessFlagBits::eTransferRead,
                vk::ImageLayout::eTransferSrcOptimal,
            };
            pass.AddDependency(access, resource);
            return resource;
        }

        RenderGraphResource GetResourceByName(string_view name) {
            return resources.GetResourceByName(name);
        }

        RenderGraphResource OutputRenderTarget(string_view name, const RenderTargetDesc &desc) {
            auto resource = RenderGraphResource(desc);
            resources.Register(name, resource);
            pass.AddOutput(resource);
            return resource;
        }

        RenderGraphResource OutputColorAttachment(uint32 index,
            string_view name,
            RenderTargetDesc desc,
            const AttachmentInfo &info) {
            desc.usage |= vk::ImageUsageFlagBits::eColorAttachment;
            return OutputAttachment(index, name, desc, info);
        }

        RenderGraphResource OutputDepthAttachment(string_view name, RenderTargetDesc desc, const AttachmentInfo &info) {
            desc.usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
            return OutputAttachment(MAX_COLOR_ATTACHMENTS, name, desc, info);
        }

        void RequirePass() {
            pass.required = true;
        }

    private:
        RenderGraphResource OutputAttachment(uint32 index,
            string_view name,
            const RenderTargetDesc &desc,
            const AttachmentInfo &info) {
            auto resource = OutputRenderTarget(name, desc);
            auto &attachment = pass.attachments[index];
            attachment = info;
            attachment.resourceID = resource.id;
            return resource;
        }

        RenderGraphResources &resources;
        RenderGraphPass &pass;
    };

    class RenderGraph {
    public:
        RenderGraph(DeviceContext &device) : device(device), resources(device) {}

        class InitialPassState {
        public:
            InitialPassState(RenderGraph &graph, string_view name) : graph(graph), name(name) {}

            template<typename SetupFunc>
            InitialPassState &Build(SetupFunc setupFunc) {
                Assert(passIndex == ~0u, "multiple Build calls for the same pass");
                RenderGraphPass pass(name);
                pass.scopes = graph.resources.scopeStack;

                RenderGraphPassBuilder builder(graph.resources, pass);
                setupFunc(builder);
                passIndex = graph.passes.size();
                graph.passes.push_back(pass);
                return *this;
            }

            InitialPassState &Execute(std::function<void(RenderGraphResources &, CommandContext &)> executeFunc) {
                Assert(passIndex != ~0u, "Build must be called before Execute");
                Assert(executeFunc, "Execute function must be defined");
                auto &pass = graph.passes[passIndex];
                Assert(!pass.HasExecute(), "multiple Execute functions for the same pass");
                pass.executeFunc = executeFunc;
                return *this;
            }

            InitialPassState &Execute(std::function<void(RenderGraphResources &, DeviceContext &)> executeFunc) {
                Assert(passIndex != ~0u, "Build must be called before Execute");
                Assert(executeFunc, "Execute function must be defined");
                auto &pass = graph.passes[passIndex];
                Assert(!pass.HasExecute(), "multiple Execute functions for the same pass");
                pass.executeFunc = executeFunc;
                return *this;
            }

        private:
            RenderGraph &graph;
            string_view name;
            uint32 passIndex = ~0u;
        };

        InitialPassState Pass(string_view name) {
            return {*this, name};
        }

        void BeginScope(string_view name);
        void EndScope();

        void SetTargetImageView(string_view name, ImageViewPtr view);

        void RequireResource(string_view name) {
            RequireResource(resources.GetResourceByName(name).id);
        }

        void RequireResource(RenderGraphResourceID id) {
            resources.IncrementRef(id);
        }

        void Execute();

        struct RenderTargetInfo {
            string name;
            RenderTargetDesc desc;
        };
        vector<RenderTargetInfo> AllRenderTargets();

    private:
        friend class InitialPassState;
        void AddPassBarriers(CommandContext &cmd, RenderGraphPass &pass);

        DeviceContext &device;
        RenderGraphResources resources;
        vector<RenderGraphPass> passes;
    };
} // namespace sp::vulkan
