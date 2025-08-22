#pragma once
#include <stack>
#include <functional>
#include <vulkan/vulkan_core.h>
#include <vk_mem_alloc.h>

// TODO: sets, some things should be deleted while others
// shouldn't, should be possible to remove spesific sets.
namespace kaze {
  class DeletionStack {
  public:
    DeletionStack();
    void add(std::function<void()> t_deletionFunction);
    void deleteAll();
    ~DeletionStack();
  private:
    std::stack<std::function<void()>> mStack;
  };

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
