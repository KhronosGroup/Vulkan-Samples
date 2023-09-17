#include "shaders/reflectors/spirv_reflector.hpp"

namespace vkb
{
ShaderResourceSet SpirvReflector::reflect(const std::vector<uint32_t> &code) const
{
	return ShaderResourceSet{{}};
}
}        // namespace vkb