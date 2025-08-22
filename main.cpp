
/*
honestly this is a demo file for now. to check if everything is working
*/

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_timer.h>
#include <array>
#include <cstdint>
#include <vector>

#include "error_handling.hpp"
#include "vulkan_init.hpp"
#include "vulkan_validation_layers.hpp"
#include "shaders.hpp"
#include "logging.hpp"
#include "vulkan_memory.hpp"

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_video.hpp>
#include <glm/common.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#define INITIAL_WIDTH 800
#define INITIAL_HEIGHT 600
#define FRAMES_IN_FLIGHT 2
#define KAZE_VALIDATION_LAYERS

// no fps cap
//#define DEFAULT_PRESENT_MODE VK_PRESENT_MODE_IMMEDIATE_KHR

#define DEFAULT_PRESENT_MODE VK_PRESENT_MODE_FIFO_KHR

namespace kz = kaze;


struct Vertex {
  glm::vec3 pos;
  glm::vec2 uv; 
};

struct Ubo {
  float colorOffset;
};

int main() {
  //kz::setLoggingMode(kz::LogFlagBits::Info | kz::LogFlagBits::Warn |
  //                   kz::LogFlagBits::Fatal);

  kz::setLoggingMode(kz::LogFlagBits::All);
  kz::setLogFile("./logz.txt");

  kz::logInfo("starting");
  #ifdef KAZE_VALIDATION_LAYERS
  kz::logInfo("using validation layers");
  const bool enableValidationLayers = true;
  #else
  const bool enableValidationLayers = false;
  
  #endif

  if (enableValidationLayers) {
    if (!kz::checkValidationLayerSupport()) {
      kz::errorExit("the requested validation layers weren't found");
    }
  }

  //SDL_Init(SDL_INIT_VIDEO);
  SDL_Init(SDL_INIT_EVENTS);



  SDL_Window* window;
  window = SDL_CreateWindow("window :*", INITIAL_WIDTH, INITIAL_HEIGHT,
			    SDL_WINDOW_VULKAN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE);
  //| SDL_WINDOW_RESIZABLE

  if (!window) {
    kz::errorExit("couldn't create window");
  }

  VkInstance vkInstance = kz::createVkInstance(enableValidationLayers);
  kz::QueueFamilyIndices queueFamilyIndices;
  VkPhysicalDevice physicalDevice;
  VkDevice device;
  VkQueue graphicsQueue;
  VkSurfaceKHR windowSurface;
  VkQueue presentQueue;

  //create window surface
  if (!SDL_Vulkan_CreateSurface(window, vkInstance, nullptr,
			       &windowSurface)) {
    kz::errorExit("couldn't create window surface (sdl)");
  }
 

  //get device
  std::tie(physicalDevice, queueFamilyIndices) =
    kz::getPhysicalDevice(vkInstance, windowSurface);

  if (!queueFamilyIndices.isComplete()) {
    kz::errorExit("queue family isn't complete");
  }
  // DEVICE creation
  device = kz::createVkDevice(physicalDevice, queueFamilyIndices);

  //that 0 means 0th queue index
  // TODO: put this in a kaze:: function
  vkGetDeviceQueue(device, queueFamilyIndices.graphicsFamily.value(),
  		   0, &graphicsQueue);

  vkGetDeviceQueue(device, queueFamilyIndices.presentFamily.value(), 0,
  		   &presentQueue);
  // vma
  kz::logInfo("creating vma allocator");
  VmaAllocatorCreateInfo vmaAllocatorInfo {};
  vmaAllocatorInfo.device = device;
  vmaAllocatorInfo.physicalDevice = physicalDevice;
  vmaAllocatorInfo.instance = vkInstance;
  vmaAllocatorInfo.vulkanApiVersion = VK_API_VERSION_1_0;

  VmaAllocator vmaAllocator;
  vmaCreateAllocator(&vmaAllocatorInfo, &vmaAllocator);

  VkSwapchainKHR swapchain;
  std::vector<VkImage> swapchainImages;
  VkFormat swapchainImageFormat;
  VkExtent2D swapchainExtent;

  // crazy
  std::tie(swapchain, swapchainImages, swapchainImageFormat, swapchainExtent) =
      kz::createSwapchain(physicalDevice, device, windowSurface, window,
                            queueFamilyIndices,
                            DEFAULT_PRESENT_MODE);

  std::vector<VkImageView> swapchainImageViews;
  swapchainImageViews =
    kz::createSwapchainImageViews(swapchainImages, swapchainImageFormat,
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

  std::unique_ptr<kz::Shader> vertShader =
    std::make_unique<kz::Shader>(basePath + "basic.vert.spv", device,
				   kz::vertexShaderStage);

  std::unique_ptr<kz::Shader> fragShader =
    std::make_unique<kz::Shader>(basePath + "basic.frag.spv", device,
				   kz::fragmentShaderStage);

  kz::logInfo("logging frag shader info");
  // not frag anymore
  std::vector<VkDescriptorSetLayout> reflectedSetLayoutsVert =
    vertShader->getVariables();

  VkPipelineShaderStageCreateInfo pipelineShaderStageInfos[2] = {
    fragShader->getShaderStageInfo(),
    vertShader->getShaderStageInfo(),
  };
  // ---

  kz::ColorBlendInfo colorBlendInfo =
    kz::createColorBlendInfo();
    
  VkPipelineRasterizationStateCreateInfo rasterizerInfo;
  rasterizerInfo = kz::createRasterizationInfo(false);

  // need to destroy
  renderPass =
    kz::createRenderPass(swapchainImageFormat, device);
    
  kz::PipelineCreationInfo pipelineInfo {};
  pipelineInfo.device = device;
  pipelineInfo.stageCount = 2;
  pipelineInfo.shaderStageInfo = pipelineShaderStageInfos;
  pipelineInfo.swapchainExtent = swapchainExtent;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.colorBlendStateInfo = colorBlendInfo.createInfo;
  pipelineInfo.rasterizerInfo = rasterizerInfo;

  // vertex stuff;

  VkVertexInputBindingDescription vertexBinding = {};
  vertexBinding.binding = 0;
  vertexBinding.stride = sizeof(Vertex);
  vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  std::array<VkVertexInputAttributeDescription, 2> vertexAttributes {};
  vertexAttributes[0].binding = 0;
  vertexAttributes[0].location = 0;
  vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  vertexAttributes[0].offset = offsetof(Vertex, pos);

  vertexAttributes[1].binding = 0;
  vertexAttributes[1].location = 1;
  vertexAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
  vertexAttributes[1].offset = offsetof(Vertex, uv);

  VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo {};
  vertexInputCreateInfo.sType =
    VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
  vertexInputCreateInfo.pVertexBindingDescriptions = &vertexBinding;

  vertexInputCreateInfo.vertexAttributeDescriptionCount = 2;
  vertexInputCreateInfo.pVertexAttributeDescriptions = &vertexAttributes[0];
  // finally creating pipelineLayout + pipeline

  // TODO: make the pipeline layout thing a seperate function.
  // it would be cool to have a funtion that generates a
  // pipeline layout via shader reflection.
  // also, returning a graphicsPipeline from a function that is called
  // createPipelineLayout is bad.
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType =
    VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  // EXPERIMENTS!
  pipelineLayoutInfo.setLayoutCount = reflectedSetLayoutsVert.size(); 
  pipelineLayoutInfo.pSetLayouts = reflectedSetLayoutsVert.data();
  pipelineLayoutInfo.pushConstantRangeCount = 0; // ALSO PUSH CONSTANTS.

  // create pipeline layout
  if (vkCreatePipelineLayout( device, &pipelineLayoutInfo, nullptr,
			      &pipelineLayout) != VK_SUCCESS) {
    kaze::errorExit("couldn't create pipline layout");
  }

  graphicsPipeline =
    kz::createPipeline(pipelineInfo, vertexInputCreateInfo, pipelineLayout);

  swapchainFramebuffers =
    kz::createSwapchainFramebuffers(swapchainImageViews,
				      swapchainExtent, renderPass,
				      device);

  // one of this dude
  VkCommandPool cmdPool =
    kz::createCommandPool(queueFamilyIndices, device);

  Vertex testVerts[] = {
    {glm::vec3(-0.5,-0.5,0), glm::vec2(0,0)},
    {glm::vec3(0.5,-0.5,0), glm::vec2(1,0)},
    {glm::vec3(0.5,0.5,0), glm::vec2(1,1)},
    {glm::vec3(-0.5,0.5,0), glm::vec2(0,1)}
  };

  kz::GpuMemoryBuffer vertexBuffer =
      kz::uploadStaticData(vmaAllocator, &testVerts, sizeof(testVerts), cmdPool,
                           device, graphicsQueue,
                           VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

  std::uint16_t indicies[] = {
    0, 1, 2, 0, 3, 2
  };
  kz::GpuMemoryBuffer indexBuffer = kz::uploadStaticData(
      vmaAllocator, &indicies, sizeof(indicies), cmdPool, device, graphicsQueue,
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

  kz::logInfo("allocating descriptors");
  kz::DeletionStack deletionStack;
  // --- descriptors
  std::vector<VkBuffer>      descBuffers(FRAMES_IN_FLIGHT);
  std::vector<VmaAllocation> descBufferAllocations(FRAMES_IN_FLIGHT);
  std::vector<Ubo>           Ubos(FRAMES_IN_FLIGHT, {0.5});

  deletionStack.add([&]() -> void {
    for (int i{}; i < FRAMES_IN_FLIGHT; i++) {
      vmaUnmapMemory(vmaAllocator, descBufferAllocations[i]);
      vmaDestroyBuffer(vmaAllocator, descBuffers[i], descBufferAllocations[i]);
    }
  });
  for (std::uint32_t i{}; i < FRAMES_IN_FLIGHT; i++) {

    VkBufferCreateInfo descriptorBufferInfo{};
    descriptorBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    descriptorBufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    descriptorBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    descriptorBufferInfo.size = sizeof(Ubo);

    VmaAllocationCreateInfo descriptorAllocationCreateInfo {};
    // i think?
    descriptorAllocationCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    //stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    // TODO: use this?
    //VmaAllocationInfo allocationInfo{};
    //allocationInfo.
    
    vmaCreateBuffer(vmaAllocator, &descriptorBufferInfo,
                    &descriptorAllocationCreateInfo, &descBuffers[i],
		    &descBufferAllocations[i], nullptr);

    void* mappedData = nullptr;

    vmaMapMemory(vmaAllocator, descBufferAllocations[i], &mappedData);

    std::memcpy(mappedData, &Ubos[i], sizeof(Ubo));

    vmaFlushAllocation(vmaAllocator, descBufferAllocations[i], 0, VK_WHOLE_SIZE);


    // not unmaping
  }
  // ---
  //VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT

  VkDescriptorPoolSize poolSize{};
  poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSize.descriptorCount = FRAMES_IN_FLIGHT; //if theres an error, here.

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.pPoolSizes = &poolSize;
  poolInfo.poolSizeCount = 1;
  poolInfo.maxSets = FRAMES_IN_FLIGHT;

  VkDescriptorPool descriptorPool;
  if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) !=
      VK_SUCCESS) {
    kz::errorExit("failed descriptor pool creation");
  }
  deletionStack.add([&]() -> void {
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
  });

  std::vector<VkDescriptorSet> descriptorSets(FRAMES_IN_FLIGHT);
  // for some reason he used reserve here.

  //cheating
  std::vector<VkDescriptorSetLayout> layouts(FRAMES_IN_FLIGHT, reflectedSetLayoutsVert[0]); 
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(FRAMES_IN_FLIGHT);
  allocInfo.pSetLayouts = layouts.data();

  if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
    kz::errorExit("failed to allocate descriptor sets!");
  }
  deletionStack.add([&]() -> void {
    for (auto& descriptorLayout : reflectedSetLayoutsVert) {
      vkDestroyDescriptorSetLayout(device, descriptorLayout, nullptr);
    }
  });

  for (int i{}; i < FRAMES_IN_FLIGHT; i++) {

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = descBuffers[i];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(Ubo);
    // VK_WHOLE_SIZE;?

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.descriptorCount = 1;
    write.dstSet = descriptorSets[i];
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
  }

  std::vector<VkCommandBuffer> cmdBuffers(swapchainImages.size());
  std::vector<VkSemaphore> imageAvailableSemaphores(swapchainImages.size());
  std::vector<VkSemaphore> renderFinishedSemaphores(swapchainImages.size());
  std::vector<VkFence> inFlightFences(swapchainImages.size());
  // this dude lets you know if the frame is being processed.
  std::vector<VkFence> imagesInFlightFences(swapchainImages.size(), VK_NULL_HANDLE);

  for (uint i{}; i < swapchainImages.size(); i++) {
    // each for swapchain image.
    cmdBuffers[i] =
      kz::createCommandBuffer(cmdPool, device);

    imageAvailableSemaphores[i] = kz::createSemaphore(device);
    renderFinishedSemaphores[i] = kz::createSemaphore(device);

    // signals when a frame is finished.
    inFlightFences[i] = kz::createFence(device, true);

  }
  uint32_t currentFrame = 0;
  // --- drawing shit.
  SDL_Event event;
  bool running = true;
  while (running) {

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
	kz::logInfo("close requested");
	running = false;
      }
    }

    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    // if the fence is active
    if (imagesInFlightFences[imageIndex] != VK_NULL_HANDLE) {
      vkWaitForFences(device, 1, &imagesInFlightFences[imageIndex], VK_TRUE,
		      UINT64_MAX);
    }

    imagesInFlightFences[imageIndex] = inFlightFences[currentFrame];

    vkResetFences(device, 1,  &inFlightFences[currentFrame]);

    vkResetCommandBuffer(cmdBuffers[imageIndex], 0);
    

    kz::startCommandBufferRenderPass(cmdBuffers[imageIndex], swapchainFramebuffers[imageIndex], renderPass, swapchainExtent);
    kz::testBG(cmdBuffers[imageIndex], graphicsPipeline, swapchainExtent);

    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmdBuffers[imageIndex], 0, 1, &vertexBuffer.buffer, offsets);
    vkCmdBindIndexBuffer(cmdBuffers[imageIndex], indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);
    //vkCmdDraw(cmdBuffers[imageIndex], 3, 1, 0, 0);

    vkCmdBindDescriptorSets(cmdBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

    vkCmdDrawIndexed(cmdBuffers[imageIndex], 6, 1, 0, 0, 0);

    kz::endCommandBufferRenderPass(cmdBuffers[imageIndex]);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffers[imageIndex];

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[imageIndex]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
      kz::errorExit("couldn't submit command buffer");
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

    // window resize handle. it has to be down here because of sync complications
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR){
      vkDeviceWaitIdle(device);

      for (auto imageView : swapchainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
      }
      for (auto framebuffer : swapchainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
      }

      vkDestroySwapchainKHR(device, swapchain, nullptr);



      std::tie(swapchain, swapchainImages,
          swapchainImageFormat, swapchainExtent) =
        kz::createSwapchain(physicalDevice, device, windowSurface, window,
			      queueFamilyIndices, DEFAULT_PRESENT_MODE);

      swapchainImageViews =
        kz::createSwapchainImageViews(swapchainImages, swapchainImageFormat,
            device);

      swapchainFramebuffers =
        kz::createSwapchainFramebuffers(swapchainImageViews,
            swapchainExtent, renderPass,
            device);


      currentFrame = (currentFrame + 1) % FRAMES_IN_FLIGHT;
      continue;
    }

    currentFrame = (currentFrame + 1) % FRAMES_IN_FLIGHT;

    SDL_Delay(1000/144);
  }
  vkDeviceWaitIdle(device);
  

  // std::cin.get();


  // cleanup
  kz::logInfo("cleanup");

  deletionStack.deleteAll();
  kz::logInfo("ended stack deletion");

  vmaDestroyBuffer(vmaAllocator, vertexBuffer.buffer, vertexBuffer.bufferAllocation);
  vmaDestroyBuffer(vmaAllocator, vertexBuffer.stagingBuffer, vertexBuffer.stagingAllocation);

  vmaDestroyBuffer(vmaAllocator, indexBuffer.buffer, indexBuffer.bufferAllocation);
  vmaDestroyBuffer(vmaAllocator, indexBuffer.stagingBuffer, indexBuffer.stagingAllocation);

  kz::logInfo("deleting allocator!");
  vmaDestroyAllocator(vmaAllocator);

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
  kz::logInfo("destroying Device");
  vkDestroyDevice(device, nullptr);
  vkDestroyInstance(vkInstance, nullptr);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
