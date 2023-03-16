#pragma once

#include "components/vulkan/shaders/shader.hpp"

namespace components
{
namespace vulkan
{
class ShaderReflector
{
  public:
	ShaderReflector()  = default;
	~ShaderReflector() = default;

	// Reflect shader SPIR-V
	virtual ShaderResources reflect_spirv(const std::vector<uint32_t> &shader_spirv) const = 0;
};
}        // namespace vulkan
}        // namespace components