#include "shaders.hpp"
#include "error_handling.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vulkan/vulkan.h>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <vulkan/vulkan_core.h>
#include "gitsubmodules.d/SPIRV-Reflect/spirv_reflect.h"

// remove later (this include)
#include "logging.hpp"


// cursed function
kaze::Shader::Shader(const std::string& filename, VkDevice t_device,
		     const VkShaderStageFlagBits shaderStage)
  : mDevice(t_device), mShaderStage(shaderStage) {

  std::ifstream file(filename, std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "couldn't open shader file " << std::filesystem::canonical(".") << std::endl;
    exit(1);
  }

  // the curser is at the end, tellg tells us where it at.
  size_t filesize = static_cast<ssize_t>(file.tellg());

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

// WE SHOULD RETURN A DESCRIPTOR SET LAYOUT
// + block info. a pair maybe
// DO NOT CALL THIS EVERY FRAME
// call it once.
std::vector<VkDescriptorSetLayout>
kaze::Shader::getVariables() {

  SpvReflectShaderModule shaderModule;
  SpvReflectResult result =
    spvReflectCreateShaderModule(mCode.size(), mCode.data(), &shaderModule);

  if (result != SPV_REFLECT_RESULT_SUCCESS) {
    kaze::errorExit("couldn't create reflect shader module");
  }
    
  std::uint32_t descriptorSetCount;
  spvReflectEnumerateDescriptorSets(&shaderModule, &descriptorSetCount, nullptr);

  std::vector<SpvReflectDescriptorSet*> descriptorSets(descriptorSetCount);

  spvReflectEnumerateDescriptorSets(&shaderModule, &descriptorSetCount, descriptorSets.data());

  std::vector<VkDescriptorSetLayout> setLayouts(descriptorSetCount);

  for (std::uint32_t setIndex{}; setIndex < descriptorSetCount; setIndex++ ) {

    auto set = descriptorSets[setIndex];

    std::vector<VkDescriptorSetLayoutBinding> layoutBindings(set->binding_count);

    for (std::uint32_t bind{}; bind < set->binding_count; bind++) {
      //kaze::logInfo(set->bindings[bind]->name);
      //kaze::logInfo(set->bindings[bind]->block.members[0].name);
      kaze::logInfo("wtf is descriptor count?");
      kaze::logInfo(set->bindings[bind]->block.members[0].name);

      auto descriptorBinding = set->bindings[bind];

      layoutBindings[bind].binding = descriptorBinding->binding;
      // possible error
      layoutBindings[bind].descriptorCount = descriptorBinding->count;
      layoutBindings[bind].stageFlags = mShaderStage;

      layoutBindings[bind].descriptorType =
	static_cast<VkDescriptorType>(descriptorBinding->descriptor_type);



      // immutable samplers are set to default for now.
    }

    VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo {};
    setLayoutCreateInfo.bindingCount = set->binding_count;
    setLayoutCreateInfo.pBindings = layoutBindings.data();
    setLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

    auto result =
        vkCreateDescriptorSetLayout(mDevice, &setLayoutCreateInfo, nullptr,
				    &setLayouts[setIndex]);

    if (result != VK_SUCCESS) kaze::errorExit("couldn't create descriptor set layout");

  }

  spvReflectDestroyShaderModule(&shaderModule);

  return setLayouts;
}
