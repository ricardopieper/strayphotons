#pragma once

#include "assets/Async.hh"
#include "assets/Gltf.hh"
#include "core/DispatchQueue.hh"
#include "graphics/vulkan/core/Common.hh"
#include "graphics/vulkan/core/Image.hh"
#include "graphics/vulkan/core/Memory.hh"

namespace sp::vulkan {
    typedef uint16 TextureIndex;

    struct TextureHandle {
        TextureIndex index = 0;
        AsyncPtr<void> ref = nullptr;

        bool Ready() const {
            return !ref || ref->Ready();
        }
    };

    class TextureSet {
    public:
        TextureSet(DeviceContext &device, DispatchQueue &workQueue);

        TextureHandle Add(const ImageCreateInfo &imageInfo,
            const ImageViewCreateInfo &viewInfo,
            const InitialData &data);

        TextureHandle Add(const ImageViewPtr &ptr);

        ImageViewPtr Get(TextureIndex i) {
            if (i == 0) return GetBlankPixel();
            return textures[i];
        }

        ImageViewPtr GetBlankPixel();
        ImageViewPtr CreateSinglePixel(glm::vec4 value);

        vk::DescriptorSet GetDescriptorSet() const {
            return textureDescriptorSet;
        }

        TextureIndex Count() const {
            return static_cast<TextureIndex>(textures.size());
        }

        TextureHandle LoadGltfMaterial(const shared_ptr<const Gltf> &source, int materialIndex, TextureType type);

        void Flush();

    private:
        void ReleaseTexture(TextureIndex i);
        TextureIndex AllocateTextureIndex();

        vector<ImageViewPtr> textures;

        vector<TextureIndex> freeTextureIndexes;
        vector<TextureIndex> texturesToFlush;
        vk::DescriptorSet textureDescriptorSet;

        robin_hood::unordered_map<string, TextureHandle> textureCache;

        DeviceContext &device;
        DispatchQueue &workQueue;
    };
} // namespace sp::vulkan
