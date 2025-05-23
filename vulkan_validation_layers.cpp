
#include<iostream>
#include<vector>
#include<cstring>
#include<vulkan/vulkan.h>
#include"vulkan_validation_layers.hpp"

bool kaze::checkValidationLayerSupport() {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  std::cout << "validation layers: \n";
  for (const char* layerName : kaze::validationLayers) {
    bool layerFound = false;

    for (const auto& layerProperties : availableLayers) {
      if (strcmp(layerName, layerProperties.layerName) == 0) {
	layerFound = true;
	break;
      }
    }

    if (!layerFound) {
      return false;
    } else {
      std::cout << layerName << '\n';
    }
  }

  std::cout << std::endl;

  return true;
  
}
