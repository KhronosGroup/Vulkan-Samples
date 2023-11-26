#pragma once

#include "shaders/shader_resources.hpp"

class SpvReflectShaderModule;

namespace vkb
{
class SpirvReflector
{
  public:
	SpirvReflector()  = default;
	~SpirvReflector() = default;

	ShaderResourceSet reflect(const std::vector<uint8_t> &code) const;

  private:
	std::vector<ShaderResource> reflect_input_variables(const SpvReflectShaderModule &module) const;
	std::vector<ShaderResource> reflect_output_variables(const SpvReflectShaderModule &module) const;
	std::vector<ShaderResource> reflect_descriptor_bindings(const SpvReflectShaderModule &module) const;
	std::vector<ShaderResource> reflect_push_constant(const SpvReflectShaderModule &module) const;
};
}        // namespace vkb