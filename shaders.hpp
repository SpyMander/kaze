#pragma once

#include <vulkan/vulkan_core.h>
// forward declerations
typedef struct VkDevice_T* VkDevice;
typedef struct VkShaderModule_T* VkShaderModule;

#include <memory>
#include <vector>
#include <string>

namespace kaze {

  constexpr auto vertexShaderStage = VK_SHADER_STAGE_VERTEX_BIT;
  constexpr auto fragmentShaderStage = VK_SHADER_STAGE_FRAGMENT_BIT;

  struct ShaderVariable {
    std::string name;
    VkFormat format;
    uint32_t set;
    uint32_t binding;
    size_t size;
  };

  struct ShaderVariables {
    std::vector<ShaderVariable> pushConstants;
    std::vector<ShaderVariable> uniformBuffers;
    std::vector<ShaderVariable> storageBuffers;
    std::vector<ShaderVariable> images;
    std::vector<ShaderVariable> samplers;
  };

  class Shader {

  public:
    Shader(const std::string& filename, VkDevice device,
	   VkShaderStageFlagBits shaderStage);
    ~Shader();

    VkPipelineShaderStageCreateInfo
    getShaderStageInfo();

    ShaderVariables
    getVariables();

  private:
    // TODO: uniform informations
    // TODO: check if the passed shader stage is real.
    VkDevice mDevice;
    VkShaderModule mShaderModule;
    VkShaderStageFlagBits mShaderStage;
    std::string mCode;
  };

  // xxx THIS CODE IS GOING TO BE DELETED? IF NO USE IS FOUND!
  class PipelineShaders {

  public:
    PipelineShaders(const std::string& vertShaderFilepath,
		    const std::string& fragShaderFilepath,
		    VkDevice _device);
    ~PipelineShaders();

  private:
    Shader vertShader;
    Shader fragShader;
    VkDevice device;
  };
}
