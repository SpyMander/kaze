#pragma once

#include<optional>
#include<tuple>
#include<vector>
#include<cstdint>
#include<vulkan/vulkan_core.h>
//#include<SDL3/SDL_video.h>
// forward decleration
class SDL_Window;

namespace kaze {

  const std::vector<const char*> gDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
  };

  struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily; //has_value();?
    std::optional<uint32_t> presentFamily;
    bool isComplete();
  };

  struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
  };

  struct PipelineCreationInfo {
    VkDevice device;
    int stageCount;
    VkPipelineShaderStageCreateInfo* shaderStageInfo;
    VkExtent2D swapchainExtent;
    VkRenderPass renderPass;
    VkPipelineColorBlendStateCreateInfo colorBlendStateInfo;
    VkPipelineRasterizationStateCreateInfo rasterizerInfo;
  };

  struct ColorBlendInfo {
    VkPipelineColorBlendAttachmentState attachmentState;
    VkPipelineColorBlendStateCreateInfo createInfo;
  };
  

  std::pair<bool, QueueFamilyIndices>
  isDeviceSuitable(const VkPhysicalDevice physicalDevice,
		   const VkSurfaceKHR surface);

  std::pair<VkPhysicalDevice, QueueFamilyIndices>
  getPhysicalDevice(const VkInstance vkInstance,
		    const VkSurfaceKHR surface);

  QueueFamilyIndices
  findQueueFamilies(const VkPhysicalDevice physicalDevice,
		    const VkSurfaceKHR surface);

  VkDevice createVkDevice(const VkPhysicalDevice physicalDevice,
			  const QueueFamilyIndices indices);

  VkInstance createVkInstance(const bool enableValidationLayers);

  // --- swapchain stuff, should this be internal?

  SwapchainSupportDetails
  querySwapchainSupportDetails(const VkPhysicalDevice device,
			       const VkSurfaceKHR surface);

  VkSurfaceFormatKHR
  chooseSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>&
			  availableFormats);


  VkPresentModeKHR
  chooseSwapchainPresentMode(const std::vector<VkPresentModeKHR>&
			     availablePresentModes, VkPresentModeKHR presentMode);

  VkExtent2D
  chooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities,
			 SDL_Window* window);

  VkSurfaceFormatKHR
  chooseSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>&
			       availableFormats);

  std::tuple<VkSwapchainKHR, std::vector<VkImage>, VkFormat, VkExtent2D>
  createSwapchain(const VkPhysicalDevice physicalDevice, const VkDevice device,
                  const VkSurfaceKHR surface, SDL_Window *window,
                  const QueueFamilyIndices indices,
                  const VkPresentModeKHR desiredPresentMode);
  

  std::vector<VkImageView>
  createSwapchainImageViews(const std::vector<VkImage>& swapchainImages,
			    const VkFormat format,
			    const VkDevice device);
  // ---

  kaze::ColorBlendInfo
  createColorBlendInfo();
 
  VkPipelineRasterizationStateCreateInfo
  createRasterizationInfo(bool wireframe);

  VkRenderPass createRenderPass(VkFormat swapchainImageFormat,
				VkDevice device);

  std::pair<VkPipeline, VkPipelineLayout>
  createPipelineLayout(const PipelineCreationInfo info);

  std::vector<VkFramebuffer>
  createSwapchainFramebuffers
  (const std::vector<VkImageView>& swapchainImageViews,
   VkExtent2D swapchainExtent, VkRenderPass renderpass, VkDevice device);

  VkCommandPool
  createCommandPool(QueueFamilyIndices indicies, VkDevice device);

  VkCommandBuffer
  createCommandBuffer(VkCommandPool commandPool, VkDevice device);

  void
  freeCommandBuffer(VkCommandPool commandPool,
		  VkCommandBuffer* buffer, VkDevice device);
 
  void startCommandBuffer(VkCommandBuffer cmdBuffer, VkFramebuffer frame,
			  VkRenderPass renderPass, VkExtent2D extent);
  
  void endCommandBuffer(VkCommandBuffer cmdBuffer);

  // this just makes it easier to create it.
  VkSemaphore
  createSemaphore(VkDevice device);

  VkFence
  createFence(VkDevice device, bool createSignaled);

  // ! THIS MAN WILL BE REMOVED
  void testBG(VkCommandBuffer cmdBuffer, VkPipeline graphicsPipeline,
	      VkExtent2D extent);
}
