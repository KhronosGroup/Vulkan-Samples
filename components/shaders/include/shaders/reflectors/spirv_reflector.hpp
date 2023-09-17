#pragma once

#include "shaders/shader_resources.hpp"

namespace vkb
{
class SpirvReflector
{
  public:
	SpirvReflector()  = default;
	~SpirvReflector() = default;

	ShaderResourceSet reflect(const std::vector<uint32_t> &code) const;
};
}        // namespace vkb