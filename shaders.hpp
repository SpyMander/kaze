#pragma once

#include <vulkan/vulkan_core.h>
// forward declerations
typedef struct VkDevice_T* VkDevice;
typedef struct VkShaderModule_T* VkShaderModule;


#include <memory>

namespace kaze {

  constexpr auto vertexShaderStage = VK_SHADER_STAGE_VERTEX_BIT;
  constexpr auto fragmentShaderStage = VK_SHADER_STAGE_FRAGMENT_BIT;

  class Shader {

  public:
    Shader(const std::string& filename, VkDevice device,
	   VkShaderStageFlagBits shaderStage);
    ~Shader();

    VkPipelineShaderStageCreateInfo getShaderStageInfo();

  private:
    // TODO: uniform informations
    // TODO: check if the passed shader stage is real.
    VkDevice device;
    VkShaderModule shaderModule;
    VkShaderStageFlagBits shaderStage;
    std::unique_ptr<char[]> pCode;
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
  // xxx
}

