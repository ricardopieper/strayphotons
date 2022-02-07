#pragma once

#include "core/Common.hh"
#include "core/Tracing.hh"
#include "graphics/vulkan/core/Common.hh"

#ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-variable"
    #pragma clang diagnostic ignored "-Wreorder"
    #pragma clang diagnostic ignored "-Wnullability-completeness"
#endif

#ifdef __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-variable"
    #pragma GCC diagnostic ignored "-Wreorder"
#endif

#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable : 4127 4189 4324)
#endif

#include <vk_mem_alloc.h>

#ifdef __GNUC__
    #pragma GCC diagnostic pop
#endif

#ifdef __clang__
    #pragma clang diagnostic pop
#endif

#ifdef _MSC_VER
    #pragma warning(pop)
#endif

namespace sp::vulkan {
    enum BufferType {
        BUFFER_TYPE_UNIFORM,
        BUFFER_TYPE_INDIRECT,
        BUFFER_TYPE_STORAGE_TRANSFER,
        BUFFER_TYPE_STORAGE_LOCAL,
        BUFFER_TYPE_STORAGE_LOCAL_INDIRECT,
        BUFFER_TYPE_STORAGE_LOCAL_VERTEX,
        BUFFER_TYPES_COUNT
    };

    struct BufferDesc {
        size_t size;
        BufferType type;
    };

    class UniqueMemory : public NonCopyable, public HasUniqueID {
    public:
        UniqueMemory() = delete;
        UniqueMemory(VmaAllocator allocator) : allocator(allocator), allocation(nullptr) {}
        virtual ~UniqueMemory() = default;

        void *Mapped() {
            void *data;
            AssertVKSuccess(MapPersistent(&data), "MapPersistent failed");
            return data;
        }

        vk::Result MapPersistent(void **data = nullptr);
        vk::Result Map(void **data);
        void Unmap();
        void UnmapPersistent();
        vk::DeviceSize ByteSize() const;

        template<typename T>
        void CopyFrom(const T *srcData, size_t srcCount = 1, size_t dstOffset = 0) {
            ZoneScoped;
            Assert(sizeof(T) * (dstOffset + srcCount) <= ByteSize(), "UniqueMemory overflow");
            T *dstData;
            Map((void **)&dstData);
            {
                ZoneScopedN("memcpy");
                std::copy(srcData, srcData + srcCount, dstData + dstOffset);
            }
            Unmap();
            Flush();
        }

        void Flush();

    protected:
        VmaAllocator allocator;
        VmaAllocation allocation;
        void *persistentMap = nullptr;
    };

    class SubBuffer : public NonCopyable, public HasUniqueID {
    public:
        SubBuffer(Buffer *buffer,
            VmaVirtualBlock subAllocationBlock,
            vk::DeviceSize offsetBytes,
            vk::DeviceSize size,
            vk::DeviceSize bytesPerElement = 1)
            : parentBuffer(buffer), subAllocationBlock(subAllocationBlock), offsetBytes(offsetBytes), size(size),
              bytesPerElement(bytesPerElement) {}

        ~SubBuffer();

        void *Mapped();

        vk::DeviceSize ArrayOffset() const {
            return offsetBytes / bytesPerElement;
        }

        vk::DeviceSize Size() const {
            return size;
        }

        vk::DeviceSize ByteOffset() const {
            return offsetBytes;
        }

    private:
        Buffer *parentBuffer;
        VmaVirtualBlock subAllocationBlock;
        vk::DeviceSize offsetBytes, size, bytesPerElement;
    };

    class Buffer : public UniqueMemory {
    public:
        Buffer();
        Buffer(vk::BufferCreateInfo bufferInfo, VmaAllocationCreateInfo allocInfo, VmaAllocator allocator);
        ~Buffer();

        vk::Buffer operator*() const {
            return buffer;
        }

        operator vk::Buffer() const {
            return buffer;
        }

        vk::DeviceSize Size() const {
            return bufferInfo.size;
        }

        // Treats the buffer as an array with all elements having `bytesPerElement` size.
        // Allocates `elementCount` elements from this array.
        // SubAllocate cannot be called after calling ArrayAllocate.
        SubBufferPtr ArrayAllocate(size_t elementCount, vk::DeviceSize bytesPerElement);

        // Treats the buffer as a heap containing arbitrarily sized allocations.
        // ArrayAllocate cannot be called after calling SubAllocate.
        SubBufferPtr SubAllocate(vk::DeviceSize size, vk::DeviceSize alignment = 4);

    private:
        vk::DeviceSize SubAllocateRaw(vk::DeviceSize size, vk::DeviceSize alignment);

        vk::BufferCreateInfo bufferInfo;
        vk::Buffer buffer;

        vk::DeviceSize subBufferBytesPerElement = 0;
        VmaVirtualBlock subAllocationBlock = VK_NULL_HANDLE;
    };
} // namespace sp::vulkan
