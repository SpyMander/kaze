#include"shaders.hpp"
#include<vulkan/vulkan.h>
#include<fstream>
#include<filesystem>
#include<iostream>
#include<vector>


// cursed function
kaze::Shader::Shader(const std::string& filename, VkDevice _device,
		     VkShaderStageFlagBits shaderStage)
  : device(_device), shaderStage(shaderStage) {

  std::ifstream file(filename, std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "couldn't open shader file " << std::filesystem::canonical(".") << std::endl;
    exit(1);
  }

  // the curser is at the end, tellg tells us where it at.
  size_t filesize = (size_t) file.tellg();

  // i dont get what .get is, but it works
  std::unique_ptr<char[]> code =
    std::make_unique<char[]>(filesize);

  file.seekg(0);
  // one of the worst peice of shits ive written
  file.read(code.get(), filesize);
  file.close();

  VkShaderModuleCreateInfo moduleCreateInfo {};
  moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  moduleCreateInfo.codeSize = filesize;

  // read it as uint32 t, vulkans' type system is like this.
  moduleCreateInfo.pCode =
    reinterpret_cast<const uint32_t*>(code.get());

  VkShaderModule _shaderModule;
  if (vkCreateShaderModule(device, &moduleCreateInfo, nullptr,
			   &_shaderModule) != VK_SUCCESS) {
    std::cerr << "couldn't make shader module" << std::endl;
  }


  // setting
  shaderModule = _shaderModule;
  pCode = std::move(code); 
}

kaze::Shader::~Shader() {
  vkDestroyShaderModule(device, shaderModule, nullptr);
}

VkPipelineShaderStageCreateInfo
kaze::Shader::getShaderStageInfo() {
  VkPipelineShaderStageCreateInfo shaderStageInfo{};

  shaderStageInfo.sType =
    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStageInfo.stage = shaderStage;
  shaderStageInfo.module = shaderModule;
  shaderStageInfo.pName = "main";

  // pSpecializationInfo, i dont think this will be added.

  return shaderStageInfo;
}
