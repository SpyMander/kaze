#include <SDL3/SDL_events.h>
#include<SDL3/SDL_filesystem.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_oldnames.h>
#include<SDL3/SDL_timer.h>
#include<cstdint>
#include<iostream>
#include<vector>

#include"error_handling.hpp"
#include"vulkan_init.hpp"
#include"vulkan_validation_layers.hpp"
#include"shaders.hpp"

#include<vulkan/vulkan.h>
#include<SDL3/SDL.h>
#include<SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#define INITIAL_WIDTH 800
#define INITIAL_HEIGHT 600
#define FRAMES_IN_FLIGHT 3
#define KAZE_VALIDATION_LAYERS

int main() {
  #ifdef KAZE_VALIDATION_LAYERS
  std::cout << "using validation layers" << std::endl;
  const bool enableValidationLayers = true;
  #else
  const bool enableValidationLayers = false;
  
  #endif

  if (enableValidationLayers) {
    if (!kaze::checkValidationLayerSupport()) {
      kaze::errorExit("the requested validation layers weren't found");
    }
  }

  //SDL_Init(SDL_INIT_VIDEO);
  SDL_Init(SDL_INIT_EVENTS);



  SDL_Window* window;
  window = SDL_CreateWindow("window :*", INITIAL_WIDTH, INITIAL_HEIGHT,
			    SDL_WINDOW_VULKAN | SDL_WINDOW_BORDERLESS);
  //| SDL_WINDOW_RESIZABLE

  if (!window) {
    kaze::errorExit("couldn't create window");
  }

  VkInstance vkInstance = kaze::createVkInstance(enableValidationLayers);
  kaze::QueueFamilyIndices queueFamilyIndices;
  VkPhysicalDevice physicalDevice;
  VkDevice device;
  VkQueue graphicsQueue;
  VkSurfaceKHR windowSurface;
  VkQueue presentQueue;


  //create window surface
  if (!SDL_Vulkan_CreateSurface(window, vkInstance, nullptr,
			       &windowSurface)) {
    kaze::errorExit("couldn't create window surface (sdl)");
  }
 

  //get device
  std::tie(physicalDevice, queueFamilyIndices) =
    kaze::getPhysicalDevice(vkInstance, windowSurface);

  if (!queueFamilyIndices.isComplete()) {
    kaze::errorExit("queue family isn't complete");
  }
  // DEVICE creation
  device = kaze::createVkDevice(physicalDevice, queueFamilyIndices);

  //that 0 means 0th queue index
  // TODO: put this in a kaze:: function
  vkGetDeviceQueue(device, queueFamilyIndices.graphicsFamily.value(),
  		   0, &graphicsQueue);

  vkGetDeviceQueue(device, queueFamilyIndices.presentFamily.value(), 0,
  		   &presentQueue);

  VkSwapchainKHR swapchain;
  std::vector<VkImage> swapchainImages;
  VkFormat swapchainImageFormat;
  VkExtent2D swapchainExtent;

  // crazy
  std::tie(swapchain, swapchainImages,
	   swapchainImageFormat, swapchainExtent) =
    kaze::createSwapchain(physicalDevice, device, windowSurface, window,
			  queueFamilyIndices);

  std::vector<VkImageView> swapchainImageViews;
  swapchainImageViews =
    kaze::createSwapchainImageViews(swapchainImages, swapchainImageFormat,
				    device);
  // pipelinez
  // these dudes need to be freed
  VkRenderPass renderPass;
  VkPipeline graphicsPipeline;
  VkPipelineLayout pipelineLayout;
  std::vector<VkFramebuffer> swapchainFramebuffers;

  // --- shader stuff
  // these guys will free themselfs when out of scope, which isn't now
  std::string basePath(SDL_GetBasePath());

  std::unique_ptr<kaze::Shader> vertShader =
    std::make_unique<kaze::Shader>(basePath + "basic.vert.spv", device,
				   kaze::vertexShaderStage);

  std::unique_ptr<kaze::Shader> fragShader =
    std::make_unique<kaze::Shader>(basePath + "basic.frag.spv", device,
				   kaze::fragmentShaderStage);

  VkPipelineShaderStageCreateInfo pipelineShaderStageInfos[2] = {
    fragShader->getShaderStageInfo(),
    vertShader->getShaderStageInfo(),
  };
  // ---

  kaze::ColorBlendInfo colorBlendInfo =
    kaze::createColorBlendInfo();
    
  VkPipelineRasterizationStateCreateInfo rasterizerInfo;
  rasterizerInfo = kaze::createRasterizationInfo(false);

  // need to destroy
  renderPass =
    kaze::createRenderPass(swapchainImageFormat, device);
    
  kaze::PipelineCreationInfo pipelineInfo {};
  pipelineInfo.device = device;
  pipelineInfo.stageCount = 2;
  pipelineInfo.shaderStageInfo = pipelineShaderStageInfos;
  pipelineInfo.swapchainExtent = swapchainExtent;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.colorBlendStateInfo = colorBlendInfo.createInfo;
  pipelineInfo.rasterizerInfo = rasterizerInfo;


  std::tie(graphicsPipeline, pipelineLayout) =
    kaze::createPipelineLayout(pipelineInfo);

  swapchainFramebuffers =
    kaze::createSwapchainFramebuffers(swapchainImageViews,
				      swapchainExtent, renderPass,
				      device);

  // one of this dude
  VkCommandPool cmdPool =
    kaze::createCommandPool(queueFamilyIndices, device);

  std::vector<VkCommandBuffer> cmdBuffers(FRAMES_IN_FLIGHT);
  std::vector<VkSemaphore> imageAvailableSemaphores(FRAMES_IN_FLIGHT);
  std::vector<VkSemaphore> renderFinishedSemaphores(FRAMES_IN_FLIGHT);
  std::vector<VkFence> inFlightFences(FRAMES_IN_FLIGHT);
  for (int i{}; i < FRAMES_IN_FLIGHT; i++) {
    // each for swapchain image.
    cmdBuffers[i] =
      kaze::createCommandBuffer(cmdPool, device);

    imageAvailableSemaphores[i] = kaze::createSemaphore(device);
    renderFinishedSemaphores[i] = kaze::createSemaphore(device);

    // signals when a frame is finished.
    inFlightFences[i] = kaze::createFence(device, true);

  }
  std::cout << "get ready for the loop." << std::endl;
  uint32_t currentFrame = 0;
  // --- drawing shit.
  SDL_Event event;
  bool running = true;
  while (running) {

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
      	std::cout << "close requestd" << std::endl;
	running = false;
      }
    }

    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1,  &inFlightFences[currentFrame]);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    //std::cout << "current swapchain image index: " << imageIndex << std::endl;

    vkResetCommandBuffer(cmdBuffers[currentFrame], 0);
    kaze::startCommandBuffer(cmdBuffers[currentFrame], swapchainFramebuffers[imageIndex], renderPass, swapchainExtent);
    kaze::testBG(cmdBuffers[currentFrame], graphicsPipeline, swapchainExtent);
    kaze::endCommandBuffer(cmdBuffers[currentFrame]);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffers[currentFrame];

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
      kaze::errorExit("couldn't submit command buffer");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(presentQueue, &presentInfo);
    currentFrame = (currentFrame + 1) % FRAMES_IN_FLIGHT;

    SDL_Delay(1000/90);
  }

  

  // std::cin.get();


  // cleanup
  // free shaders before anything else
  vertShader.reset();
  fragShader.reset();

  for (auto semaphore : imageAvailableSemaphores)
    vkDestroySemaphore(device, semaphore, nullptr);

  for (auto semaphore : renderFinishedSemaphores)
    vkDestroySemaphore(device, semaphore, nullptr);

  for (auto fence : inFlightFences)
    vkDestroyFence(device, fence, nullptr);

  // vkDestroySurfaceKHR(vkInstance, windowSurface, nullptr);??
  for (auto imageView : swapchainImageViews) {
    vkDestroyImageView(device, imageView, nullptr);
  }
  for (auto framebuffer : swapchainFramebuffers) {
    vkDestroyFramebuffer(device, framebuffer, nullptr);
  }

  // --- TODO: make these guys free themselfs
  vkDestroyPipeline(device, graphicsPipeline, nullptr);
  vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
  vkDestroyRenderPass(device, renderPass, nullptr);
  // ---
  
  vkDestroyCommandPool(device, cmdPool, nullptr);

  //vkDestroySurfaceKHR(vkInstance, windowSurface, nullptr);
  vkDestroySwapchainKHR(device, swapchain, nullptr);
  SDL_Vulkan_DestroySurface(vkInstance, windowSurface, nullptr);
  vkDestroyDevice(device, nullptr);
  vkDestroyInstance(vkInstance, nullptr);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
