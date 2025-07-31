#include "vulkan_init.hpp"
#include "error_handling.hpp"
#include <vulkan/vulkan_core.h>
#define VMA_IMPLEMENTATION
#include "vulkan_memory.hpp"

namespace kz = kaze;

// quick and dirty vertex uploader.
// it's static, also it memcpy's, which might be bad.
kz::GpuMemoryBuffer
kz::uploadStaticVertecies(VmaAllocator allocator,
			  void *vertexData,
			  std::size_t arrSize,
			  VkCommandPool cmdPool, VkDevice device,
			  VkQueue graphicsQueue) {
  // vertex buffer
  VkBufferCreateInfo bufferInfo {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = arrSize;
  bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
    VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo allocationInfo {};
  allocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  VkBuffer vertexBuffer;
  VmaAllocation vertexBufferAllocation;

  auto bufferCreation =
    vmaCreateBuffer(allocator, &bufferInfo, &allocationInfo, &vertexBuffer, &vertexBufferAllocation, nullptr);
  if (bufferCreation != VK_SUCCESS) {
    kz::errorExit("coudln't create vertex buffer");
  }

  // stage + transfer
  VkBufferCreateInfo stagingBufferInfo = bufferInfo;
  stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

  VmaAllocationCreateInfo stagingAllocInfo{};
  stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
  stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

  VkBuffer stagingBuffer;
  VmaAllocation stagingBufferAllocation;
  VmaAllocationInfo stagingAllocInfo_out;

  auto stagingBufferCreation =
    vmaCreateBuffer(allocator, &stagingBufferInfo, &stagingAllocInfo,
		  &stagingBuffer, &stagingBufferAllocation, &stagingAllocInfo_out);

  if (stagingBufferCreation != VK_SUCCESS) {
    kz::errorExit("couldn't create staging buffer, vertex");
  }

  std::memcpy(stagingAllocInfo_out.pMappedData, vertexData, bufferInfo.size);

  VkCommandBufferBeginInfo beginInfo {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  auto cmdBuffer = kz::createCommandBuffer(cmdPool, device);

  vkBeginCommandBuffer(cmdBuffer, &beginInfo);

  VkBufferCopy copyRegion = {};
  copyRegion.size = bufferInfo.size;
  vkCmdCopyBuffer(cmdBuffer, stagingBuffer, vertexBuffer, 1, &copyRegion);

  vkEndCommandBuffer(cmdBuffer);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &cmdBuffer;


  vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);


  // i know some snobby ass, no life, bucktooth, lanky, stanky, neckbeard,
  // wanna be developer, will say that this is bad since it blocks the cpu.
  // well, if your so good at programming, why don't you fix it yourself? pls :D
  vkQueueWaitIdle(graphicsQueue);
  return {
    vertexBuffer, vertexBufferAllocation,
    stagingBuffer, stagingBufferAllocation};
}
