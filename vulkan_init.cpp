
// this file makes it easier to create and initate vulkan files

#include"vulkan_init.hpp"
#include"vulkan_validation_layers.hpp"
#include"error_handling.hpp"

#include<iostream>
#include<vector>
#include<set>
#include<algorithm>
#include<vulkan/vulkan.h>
#include<SDL3/SDL_video.h>
#include<SDL3/SDL_vulkan.h>
#include<vulkan/vulkan_core.h>

bool kaze::QueueFamilyIndices::isComplete() {
  bool result = ( graphicsFamily.has_value()
		  && presentFamily.has_value() );
  if (!result) {
    //std::cerr<< "queue family not complete" << std::endl;
    //exit(1);
  }
  return result;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device) { //internal
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
				       nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
				       availableExtensions.data());

  // this confused me, i thought it was a function, it's a constructor lol.

  std::set<std::string>
    requiredExtensions(kaze::deviceExtensions.begin(),
		       kaze::deviceExtensions.end());

  for (const auto& extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName);
  }

  // quote from the vulkan tutorial website
  // "The performance difference is irrelevant."
  if (!requiredExtensions.empty()) {
    std::cerr << "extentions not supported \n";
    for (const auto& extension : requiredExtensions) {
      std::cerr << extension << "\n";
    }
    std::cerr << std::endl;
  }

  return requiredExtensions.empty();
}

std::pair<bool, kaze::QueueFamilyIndices>
kaze::isDeviceSuitable(const VkPhysicalDevice vkPhysicalDevice,
		       const VkSurfaceKHR surface) {
  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(vkPhysicalDevice, &deviceProperties);

  VkPhysicalDeviceFeatures deviceFeatures;
  vkGetPhysicalDeviceFeatures(vkPhysicalDevice, &deviceFeatures);

  QueueFamilyIndices familyIndices = findQueueFamilies(vkPhysicalDevice,
						       surface);

  kaze::SwapchainSupportDetails swapchainSupportDetails =
    querySwapchainSupportDetails(vkPhysicalDevice, surface);

  // TODO: too many if statements, make a better system
  if (!familyIndices.isComplete()) {
    return {false, familyIndices};
  }

  else if (!checkDeviceExtensionSupport(vkPhysicalDevice)) {
    return {false, familyIndices};
  }


  else if (swapchainSupportDetails.formats.empty() ||
	   swapchainSupportDetails.presentModes.empty()) {
    return {false, familyIndices};
  }

  //TODO: check for 64 bit floats and multi viewport rendering
  bool suitable = false;

  suitable =
    (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
     && deviceFeatures.geometryShader);

  return {suitable, familyIndices};
}

std::pair<VkPhysicalDevice, kaze::QueueFamilyIndices>
kaze::getPhysicalDevice(const VkInstance vkInstance,
			const VkSurfaceKHR surface) {
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr);
  if (deviceCount == 0) {
    errorExit("vulkan physical device not found");
  }
  std::vector<VkPhysicalDevice> physicalDevices(deviceCount);

  vkEnumeratePhysicalDevices(vkInstance, &deviceCount,
			     physicalDevices.data());

  QueueFamilyIndices familyIndices;

  for (auto d : physicalDevices) {
    // lol
    std::pair<bool, QueueFamilyIndices> isSuitable =
      kaze::isDeviceSuitable(d, surface);

    if (isSuitable.first) {
      physicalDevice = d;
      familyIndices = isSuitable.second;
      break;
    }
  }

  if (physicalDevice == VK_NULL_HANDLE) {
    kaze::errorExit("couldn't find suitable physical device");
  }

  // the whole reason of this is to do less allocations
  return {physicalDevice, familyIndices};
}


kaze::QueueFamilyIndices
kaze::findQueueFamilies(const VkPhysicalDevice vkPhysicalDevice,
			const VkSurfaceKHR surface) {
  QueueFamilyIndices indices;

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice,
					   &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice,
					   &queueFamilyCount,
					   queueFamilies.data());
					  

  // i do not understand whats happening here, i need to understand flags.
  // this might cause big issues.
  int i = 0;
  for (const auto& queueFamily : queueFamilies) {
    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphicsFamily = i;

      // TODO: 
      // this bit is really important i bet but,
      // i kinda just pick the first one and it works,
      // prob i need to make a better system to support
      // more gpus. this works for now
      //if (indices.isComplete()) { //no idea why we do it like this.
      //break;
      //}
      break;
    }

    i++;
  }

  //check surface support
  VkBool32 presentSupport = false;
  vkGetPhysicalDeviceSurfaceSupportKHR(vkPhysicalDevice, i,
				       surface, &presentSupport);

  // TODO: 
  // https://vulkan-tutorial.com/Drawing_a_triangle/Presentation
  // /Window_surface
  // Note that it's very likely that these end up being the same queue
  // family after all, but throughout the program we will treat them as if
  // they were separate queues for a uniform approach. Nevertheless, you
  // could add logic to explicitly prefer a physical device that supports
  // drawing and presentation in the same queue for improved performance.
  if (presentSupport) {
    indices.presentFamily = i;
  } else {
    std::cerr << "no present support" << std::endl;
  }

  return indices;
}

VkDeviceQueueCreateInfo //internal
getDeviceQueueCreateInfo(const kaze::QueueFamilyIndices indices) {
  VkDeviceQueueCreateInfo queueCreateInfo{};
  queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo.queueFamilyIndex =
    indices.graphicsFamily.value();

  //TODO: right now its only one queue count, later it'll be more
  queueCreateInfo.queueCount = 1;
  float queuePriority = 1.0f;
  queueCreateInfo.pQueuePriorities = &queuePriority;

  return queueCreateInfo;
}

VkDevice kaze::createVkDevice(const VkPhysicalDevice vkPhysicalDevice,
			      const QueueFamilyIndices indices) {

  //TODO: very important thing here, take more care
  //as of now everything is VK_FALSE, no features
  VkPhysicalDeviceFeatures deviceFeatures{};

  VkDeviceQueueCreateInfo deviceQueueCreateInfo =
    getDeviceQueueCreateInfo(indices);

  VkDevice vkDevice;

  VkDeviceCreateInfo deviceCreateInfo{};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

  deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
  deviceCreateInfo.queueCreateInfoCount = 1;

  deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

  // the extentions have already been tested, this is fine.
  deviceCreateInfo.enabledExtensionCount =
    static_cast<uint32_t>(kaze::deviceExtensions.size());

  deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

  // --- magic, TODO: put this in a function
  std::vector<VkDeviceQueueCreateInfo> queueInfos;
  std::set<uint32_t> uniqueQueueFamilies = {
    indices.graphicsFamily.value(),
    indices.presentFamily.value()
  };

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueInfos.push_back(queueCreateInfo);
  }

  // ---

  deviceCreateInfo.queueCreateInfoCount =
    static_cast<uint32_t>(queueInfos.size());
  deviceCreateInfo.pQueueCreateInfos = queueInfos.data();
  if (vkCreateDevice(vkPhysicalDevice, &deviceCreateInfo, nullptr,
		     &vkDevice) != VK_SUCCESS) {
    errorExit("failed to create logical device!");
  }

  return vkDevice;
}

VkInstance kaze::createVkInstance(const bool enableValidationLayers) {
  VkInstance instance;
  
  uint32_t extentionCount;

  char const* const* extentions =
    SDL_Vulkan_GetInstanceExtensions(&extentionCount);

  VkInstanceCreateInfo instanceCreateInfo = {};
  // --- validation layers 
  if (enableValidationLayers) {
    instanceCreateInfo.enabledLayerCount =
      static_cast<uint32_t>(kaze::validationLayers.size());

    instanceCreateInfo.ppEnabledLayerNames =
      kaze::validationLayers.data();
  } else {
    instanceCreateInfo.enabledLayerCount = 0;
  }
  // --- 

  instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instanceCreateInfo.pApplicationInfo = nullptr;
  instanceCreateInfo.enabledExtensionCount = extentionCount;
  instanceCreateInfo.ppEnabledExtensionNames = extentions;

  // finale
  VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr,
				     &instance);
  if (result != VK_SUCCESS) {
    kaze::errorExit("couldn't create vulkan instance");
  }

  return instance;
}

kaze::SwapchainSupportDetails
kaze::querySwapchainSupportDetails(const VkPhysicalDevice device,
				   const VkSurfaceKHR surface) {
  SwapchainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
					    &details.capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
				       nullptr);

  if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
					 details.formats.data());
  } else {
    kaze::errorExit("no surface formats?!");
  }
  // about presentation modes:
  // https://registry.khronos.org/vulkan/specs/latest/man/html
  // /VkPresentModeKHR.html

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface,
					    &presentModeCount, nullptr);
  if (presentModeCount > 0) {
    // resize in this case to still add the indexes.
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface,
					      &presentModeCount,
					      details.presentModes.data());
  } else {
    kaze::errorExit("no present modes!?");
  }

  return details;
}

VkSurfaceFormatKHR
kaze::chooseSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>&
			      availableFormats) {

  for (const auto& format : availableFormats) {
    // we are targeting sRGB(8 8 8) in non-linear colorspace
    // dunno why its B G R and not RGB though

    if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
	format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return format;
    }
  }
  

  // TODO: if the one we want isnt specified, we will settle
  // for the next best one. but rn we just get the first one

  return availableFormats[0];
}

VkPresentModeKHR
kaze::chooseSwapchainPresentMode(const std::vector<VkPresentModeKHR>&
				 availablePresentModes) {

  for (const auto& presentMode : availablePresentModes) {
    if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      std::cout<<"warn: using mailbox present mode!" << std::endl;
      return presentMode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D
kaze::chooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities,
			    SDL_Window* window) {
  if (capabilities.currentExtent.width != UINT32_MAX) {
    return capabilities.currentExtent;
  } else {
    int width, height;
    SDL_GetWindowSizeInPixels(window, &width, &height);
    std::cout << width << " x " << height << std::endl;
    VkExtent2D actualExtent = {
      static_cast<uint32_t> (width),
      static_cast<uint32_t> (height)
    };
    
    actualExtent.width =
      std::clamp(actualExtent.width, capabilities.minImageExtent.width,
		 capabilities.maxImageExtent.width);

    actualExtent.height =
      std::clamp(actualExtent.height, capabilities.minImageExtent.height,
		 capabilities.maxImageExtent.height);

    return actualExtent;

  }
}

std::tuple<VkSwapchainKHR, std::vector<VkImage>, VkFormat, VkExtent2D>
kaze::createSwapchain(const VkPhysicalDevice physicalDevice,
		      const VkDevice device,
		      const VkSurfaceKHR surface, SDL_Window* window,
		      const QueueFamilyIndices indices) {

  SwapchainSupportDetails swapchainSupportDetails =
    querySwapchainSupportDetails(physicalDevice, surface);

  VkSurfaceFormatKHR surfaceFormat =
    chooseSwapchainSurfaceFormat(swapchainSupportDetails.formats);

  VkPresentModeKHR presentMode = 
    chooseSwapchainPresentMode(swapchainSupportDetails.presentModes);

  VkExtent2D extent =
    chooseSwapchainExtent(swapchainSupportDetails.capabilities, window);

  // it is possilbe to add buffer frames here?
  uint32_t imageCount =
    swapchainSupportDetails.capabilities.minImageCount + 1;

  std::cout << "swapchain image min count: " << imageCount << std::endl;

  if (swapchainSupportDetails.capabilities.maxImageCount != 0 &&
      imageCount > swapchainSupportDetails.capabilities.maxImageCount) {
    std::cout<< "swapchain, image count: " 
	     << swapchainSupportDetails.capabilities.maxImageCount
	     << std::endl;

    imageCount = swapchainSupportDetails.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo {}; 

  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = surface;
  createInfo.minImageCount = imageCount;
  // colors
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;

  createInfo.imageExtent = extent;

  // important! this might if i add 3d stuff.
  createInfo.imageArrayLayers = 1;

  // VK_IMAGE_USAGE_TRANSFER_DST_BIT for post processing
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  // NOTE: queue family indices is passed by argument, this might cause
  // issues, for e.g, passing the queue family of a seperate device that 
  // doesn't have this family queue.
  uint32_t queueFamilyIndices[] =
    {indices.graphicsFamily.value(), indices.presentFamily.value()};

  if (indices.graphicsFamily != indices.presentFamily) {
    std::cout << "graphics family is present family :)" << std::endl;
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    std::cout << "graphics family is not present family :(" << std::endl;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0; // Optional
    createInfo.pQueueFamilyIndices = nullptr; // Optional
  }

  // we want no transforms
  createInfo.preTransform =
    swapchainSupportDetails.capabilities.currentTransform;

  // this is window transparency.
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;

  // TODO: resizing windows
  createInfo.oldSwapchain = VK_NULL_HANDLE;

  VkSwapchainKHR swapchain;
  if(vkCreateSwapchainKHR(device, &createInfo, nullptr,
			  &swapchain) != VK_SUCCESS)  {
    kaze::errorExit("couldn't create swapchain");
  }

  // TODO: abstract to internal function

  std::vector<VkImage> swapchainImages;
  
  vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);

  swapchainImages.resize(imageCount);

  vkGetSwapchainImagesKHR(device, swapchain, &imageCount,
			  swapchainImages.data());

  return {swapchain, swapchainImages, surfaceFormat.format, extent};
  //return std::make_tuple(swapchain, swapchainImages, surfaceFormat);
}

std::vector<VkImageView>
kaze::createSwapchainImageViews(const std::vector<VkImage>& swapchainImages,
				const VkFormat format,
				const VkDevice device) {

  std::vector<VkImageView> imageViews(swapchainImages.size());
  // cannot pass by refrence, dangling data.

  for (size_t i=0; i < swapchainImages.size(); i++) {
    VkImageViewCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;
    createInfo.image = swapchainImages[i];
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device, &createInfo, nullptr, &imageViews[i])
	!= VK_SUCCESS) {
      kaze::errorExit("couldn't create image view for swapchain");
    }
  }

  // hope to god this turns into a std::move();
  return imageViews;
}


// pipeline stuff
VkPipelineLayout createPipelineLayout(VkDevice device) {

    // TODO: this is the guy who holds the uniforms. and push constants
    // i will also have to free this guy.
    VkPipelineLayout pipelineLayout;
    VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
    pipelineLayoutInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    pipelineLayoutInfo.setLayoutCount = 0; // Optional
    pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
			       &pipelineLayout) != VK_SUCCESS) {
      kaze::errorExit("couldn't create vk pipeline layout");
    }

    return pipelineLayout;
}

kaze::ColorBlendInfo
kaze::createColorBlendInfo() {
  ColorBlendInfo colorBlendInfo;

  VkPipelineColorBlendAttachmentState colorBlendAttachment {};
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
    VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
    VK_COLOR_COMPONENT_A_BIT;

  // TODO: take better care of this, color blending is important.
  // only alpha blending for now
  colorBlendAttachment.blendEnable = VK_FALSE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstColorBlendFactor =
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  colorBlendInfo.attachmentState = colorBlendAttachment;

  // no idea what this is.
  // multiple attachments?
  VkPipelineColorBlendStateCreateInfo colorBlending {};
  colorBlending.sType =
    VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY; // O
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendInfo.attachmentState;
  colorBlending.blendConstants[0] = 0.0f; // O
  colorBlending.blendConstants[1] = 0.0f; // O
  colorBlending.blendConstants[2] = 0.0f; // O
  colorBlending.blendConstants[3] = 0.0f; // O

  colorBlendInfo.createInfo = colorBlending;

  return colorBlendInfo;
}

VkPipelineRasterizationStateCreateInfo
kaze::createRasterizationInfo(bool wireframe) {
  VkPipelineRasterizationStateCreateInfo rasterizer {};
  rasterizer.sType =
    VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  // most piplines exept shadow maps need this true
  rasterizer.depthClampEnable = VK_FALSE;

  rasterizer.rasterizerDiscardEnable = VK_FALSE;

  // VK_POLYGON_MODE_LINE for wireframe
  // i did it like this to make it clear whats going on
  if (wireframe) {
    rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
  } else {
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  }

  rasterizer.lineWidth = 1.0f;

  // backface culling
  // this might cause errors but i wont cull the backface
  // VK_CULL_MODE_BACK_BIT for backface culling
  rasterizer.cullMode = VK_CULL_MODE_NONE;
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

  // not altering the depth values
  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f;
  rasterizer.depthBiasClamp = 0.0f;
  rasterizer.depthBiasSlopeFactor = 0.0f;

  return rasterizer;
}

// HARDCODED
VkPipelineVertexInputStateCreateInfo
createVertexInputStateInfo () {
  // i might add arguments to the function too.
  // how vertexes are passed.
  VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo {};
  vertexInputCreateInfo.sType =
    VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  // TODO: THIS WILL BE CHANGED, since we will pass vertexes
  vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
  vertexInputCreateInfo.pVertexBindingDescriptions = nullptr;

  vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
  vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;

  return vertexInputCreateInfo;
}

VkPipelineInputAssemblyStateCreateInfo
createInputAssemblyStateinfo() {
    // input assembly - the topology of the passed triangles
    // for now, it will just be a basic triangles list.
    VkPipelineInputAssemblyStateCreateInfo inputAssembly {};
    inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
 
    // HARDCODED, a real use for VK_PRIMITIVE_TOPOLOGY_POINT_LIST
    // or line list
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    return inputAssembly;
}

// HARDCODED
// TODO: this shit is hardcoded, the subpasses need to be
// more dynamic, fix this.
VkRenderPass kaze::createRenderPass(VkFormat swapchainImageFormat,
				    VkDevice device) {
  VkAttachmentDescription colorAttachment {};
  colorAttachment.format = swapchainImageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  // VK_ATTACHMENT_STORE_OP_DONT_CARE for no clearing?
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

  // no stencil, this might change
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

  // we dont care what format it is initialy, since we clear it.
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  // HARDCODED
  // VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL for copying in memory
  // also, this means that it will only be used for presenting aka swapchain
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  // i have no idea what the fuck this means
  // TODO: understand whats going on here.
  // i know that a subpass is used to render other stuff with the
  // previously rendered stuff, but what does this have to do with that?
  VkAttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  // render pass is just a composite of subpasses
  // also, TODO: it should be possible to add multiple subpasses
  // to a renderpass, this guy is hard coded.
  // also the amount of attachments.
  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  // dep, HARDCODED
  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;

  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;

  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  
  
  VkRenderPass renderpass;

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  // kinda hardcoded.
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  // HARDCODED
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  // HARDCODED
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderpass)
      != VK_SUCCESS) {
    kaze::errorExit("failed to create render pass!");
  }
 
  return renderpass;
}

std::pair<VkPipeline, VkPipelineLayout>
kaze::createPipelineLayout(const PipelineCreationInfo info) {

    const int dynamicStateCount = 2;
    VkDynamicState dynamicStates[dynamicStateCount] {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamicState {};
    dynamicState.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;

    dynamicState.dynamicStateCount =
      static_cast<uint32_t>(dynamicStateCount);

    dynamicState.pDynamicStates = dynamicStates;

    // how vertexes are passed.
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo {};
    vertexInputCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    // HARDCODED
    // TODO: THIS WILL BE CHANGED, since we will pass vertexes
    // CHANGED !!! 
    vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
    vertexInputCreateInfo.pVertexBindingDescriptions = nullptr;

    vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
    vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;

    // input assembly - the topology of the passed triangles
    // for now, it will just be a basic triangles list.
    VkPipelineInputAssemblyStateCreateInfo inputAssembly {};
    inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // TODO: viewport creation should be more abstract
    VkViewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;

    viewport.width = (float) info.swapchainExtent.width;
    viewport.height = (float) info.swapchainExtent.height;

    // standard depth value.
    // ERR?
    viewport.minDepth = 0.0f;
    // viewport.minDepth = 1.0f;
    viewport.maxDepth = 1.0f;


    // no cuts, full framebuffer render
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = info.swapchainExtent;


    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;

    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // rasterizer
    // multi sampling (off for now)
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    multisampling.minSampleShading = 1.0f; // O
    multisampling.pSampleMask = nullptr; // O
    multisampling.alphaToCoverageEnable = VK_FALSE; // O
    multisampling.alphaToOneEnable = VK_FALSE; // O

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    VkPipelineLayout pipelineLayout;
    // create pipeline layout
    if (vkCreatePipelineLayout(info.device, &pipelineLayoutInfo, nullptr,
			       &pipelineLayout) != VK_SUCCESS) {
      kaze::errorExit("couldn't create pipline layout");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = info.stageCount;
    pipelineInfo.pStages = info.shaderStageInfo;

    pipelineInfo.pVertexInputState = &vertexInputCreateInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &info.rasterizerInfo;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &info.colorBlendStateInfo;
    pipelineInfo.pDynamicState = &dynamicState;


    pipelineInfo.layout = pipelineLayout;

    pipelineInfo.renderPass = info.renderPass;
    pipelineInfo.subpass = 0;

    // used to make derived pipelines
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    VkPipeline graphicsPipeline;

    if (vkCreateGraphicsPipelines(info.device, VK_NULL_HANDLE, 1,
				&pipelineInfo, nullptr,
				  &graphicsPipeline) != VK_SUCCESS) {
      kaze::errorExit("failed to create graphics pipeline!");
    }

    return {graphicsPipeline, pipelineLayout};
}

std::vector<VkFramebuffer>
kaze::createSwapchainFramebuffers
(const std::vector<VkImageView>& swapchainImageViews,
 VkExtent2D swapchainExtent, VkRenderPass renderpass, VkDevice device) {

  size_t size = swapchainImageViews.size();
  std::vector<VkFramebuffer> swapchainFramebuffers(size);

  for (size_t i = 0; i < swapchainImageViews.size(); i++) {
    VkImageView attachments[] = {
      swapchainImageViews[i]
    };

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderpass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = swapchainExtent.width;
    framebufferInfo.height = swapchainExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr,
			    &swapchainFramebuffers[i]) != VK_SUCCESS) {
      kaze::errorExit("failed to create framebuffer!");
    }
  }

  std::cout << swapchainFramebuffers[0] << std::endl;

  return swapchainFramebuffers;
}

VkCommandPool
kaze::createCommandPool(QueueFamilyIndices indicies, VkDevice device) {
  // guy needs to be destroyed
  VkCommandPool commandPool;
  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = indicies.graphicsFamily.value();

  if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool)
      != VK_SUCCESS) {
    kaze::errorExit("couldn't create a command pool");
  }

  return commandPool;
}

VkCommandBuffer
kaze::createCommandBuffer(VkCommandPool commandPool, VkDevice device) {
  // somehow you can create multiple of these to render multiple things
  // at once, or multithread the commands atleast.
  // really important guy basically.
  
  VkCommandBuffer commandBuffer;
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer)
      != VK_SUCCESS) {
    kaze::errorExit("couldn't create command buffer");
  }

  return commandBuffer;
}

void
kaze::startCommandBuffer(VkCommandBuffer cmdBuffer, VkFramebuffer frame,
			 VkRenderPass renderPass, VkExtent2D extent) {

  // this guy 'renders' to a framebuffer, starts it atleast

  VkCommandBufferBeginInfo beginInfo {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0; // Optional
  beginInfo.pInheritanceInfo = nullptr; // for secondary buffers

  if (vkBeginCommandBuffer(cmdBuffer, &beginInfo) != VK_SUCCESS) {
    kaze::errorExit("couldn't start recording command to cmd buffer");
  }

  // this should be threadable. don't use the same framebuffer doe.
  // passes will be made elswere at some point
  VkRenderPassBeginInfo renderPassInfo {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = frame;

  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = extent;
  
  VkClearValue clearColor = {{{1.0f, 0.0f, 0.0f, 1.0f}}};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearColor;

  vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo,
		       VK_SUBPASS_CONTENTS_INLINE);
  // VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS for secondary buffers
  

  // ! THIS GUY SHOULDNT BE HERE TODO: MOVE THIS MOFO !
}

void
kaze::endCommandBuffer(VkCommandBuffer cmdBuffer) {
  // de fuk is this guy supposed to be at?
  vkCmdEndRenderPass(cmdBuffer);
  if (vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS) {
    kaze::errorExit("couldn't commit/end the command buffer");
  }
}
// ! THIS GUY WILL BE REMOVED !
void
kaze::testBG(VkCommandBuffer cmdBuffer, VkPipeline graphicsPipeline,
	     VkExtent2D extent) {
  vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
  VkViewport viewport {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(extent.width);
  viewport.height = static_cast<float>(extent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = extent;

  vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
  vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
  // vertex count, !instance mode, vertex start index, instance start index
}


VkSemaphore kaze::createSemaphore(VkDevice device) {
  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  VkSemaphore semaphore;

  if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore) !=
      VK_SUCCESS) {
    kaze::errorExit("couldn't create a semaphore");
  }

  return semaphore;

}

VkFence kaze::createFence(VkDevice device, bool createSignaled) {
  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  if (createSignaled)
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  VkFence fence;


  if (vkCreateFence(device, &fenceInfo, nullptr, &fence) !=
      VK_SUCCESS) {
    kaze::errorExit("couldn't create a fence");
  }

  return fence;

}
