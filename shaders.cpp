#include "shaders.hpp"
#include <vulkan/vulkan.h>
#include <fstream>
#include <filesystem>
#include <iostream>
#include "gitsubmodules.d/SPIRV-Reflect/spirv_reflect.h"

// cursed function
kaze::Shader::Shader(const std::string& filename, VkDevice _device,
		     VkShaderStageFlagBits shaderStage)
  : mDevice(_device), mShaderStage(shaderStage) {

  std::ifstream file(filename, std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "couldn't open shader file " << std::filesystem::canonical(".") << std::endl;
    exit(1);
  }

  // the curser is at the end, tellg tells us where it at.
  size_t filesize = (size_t) file.tellg();

  // i dont get what .get is, but it works
  std::string code("",filesize);

  file.seekg(0);
  // one of the worst peice of shits ive written
  file.read(code.data(), filesize);
  file.close();

  VkShaderModuleCreateInfo moduleCreateInfo {};
  moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  moduleCreateInfo.codeSize = filesize;

  // read it as uint32 t, vulkans' type system is like this.
  moduleCreateInfo.pCode =
    reinterpret_cast<const uint32_t*>(code.c_str());

  VkShaderModule _shaderModule;
  if (vkCreateShaderModule(mDevice, &moduleCreateInfo, nullptr,
			   &_shaderModule) != VK_SUCCESS) {
    std::cerr << "couldn't make shader module" << std::endl;
  }


  // setting
  mShaderModule = _shaderModule;
  mCode = std::string(code);
}

kaze::Shader::~Shader() {
  vkDestroyShaderModule(mDevice, mShaderModule, nullptr);
}

VkPipelineShaderStageCreateInfo
kaze::Shader::getShaderStageInfo() {
  VkPipelineShaderStageCreateInfo shaderStageInfo{};

  shaderStageInfo.sType =
    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStageInfo.stage = mShaderStage;
  shaderStageInfo.module = mShaderModule;
  shaderStageInfo.pName = "main";

  // pSpecializationInfo, i dont think this will be added.

  return shaderStageInfo;
}
kaze::ShaderVariables
kaze::Shader::getVariables() {
  kaze::ShaderVariables variables;

  
  
  return variables;
}
