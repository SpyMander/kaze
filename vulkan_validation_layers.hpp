#pragma once
#include<vector>

namespace kaze {
  const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
  };

  
  bool checkValidationLayerSupport();
}
