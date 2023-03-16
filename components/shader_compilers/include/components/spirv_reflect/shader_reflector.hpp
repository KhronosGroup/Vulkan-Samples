#pragma once

#include <components/vulkan/shaders/reflector.hpp>

namespace components
{
namespace spirv_reflect
{
class SpirvReflectShaderReflector : public vulkan::ShaderReflector
{
  public:
	SpirvReflectShaderReflector()  = default;
	~SpirvReflectShaderReflector() = default;

	vulkan::ShaderResources reflect_spirv(const std::vector<uint32_t> &spirv) const override;
};
}        // namespace spirv_cross
}        // namespace components