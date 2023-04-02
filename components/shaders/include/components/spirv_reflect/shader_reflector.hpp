#pragma once

#include <components/shaders/reflector.hpp>

namespace components
{
namespace spirv_reflect
{
class SpirvReflectShaderReflector : public shaders::ShaderReflector
{
  public:
	SpirvReflectShaderReflector()  = default;
	~SpirvReflectShaderReflector() = default;

	shaders::ShaderResources reflect_spirv(const std::vector<uint32_t> &spirv) const override;
};
}        // namespace spirv_cross
}        // namespace components