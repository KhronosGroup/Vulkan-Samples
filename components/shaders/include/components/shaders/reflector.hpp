#pragma once

#include "components/shaders/shader.hpp"

namespace components
{
namespace shaders
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