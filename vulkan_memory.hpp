#pragma once
#include <utility>
#include <vulkan/vulkan_core.h>
#include <vk_mem_alloc.h>


namespace kaze {
  // staged, static. lazy way to upload
  struct GpuMemoryBuffer {
    VkBuffer buffer;
    VmaAllocation bufferAllocation;
    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;
  };

  GpuMemoryBuffer
  uploadStaticData(VmaAllocator allocator, void *data,
		   std::size_t arrSize, VkCommandPool cmdPool,
		   VkDevice device, VkQueue graphicsQueue,
		   VkBufferUsageFlagBits usage);
}
