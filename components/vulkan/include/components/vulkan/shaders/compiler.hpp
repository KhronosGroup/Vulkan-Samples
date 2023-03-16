#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include <volk.h>

namespace components
{
namespace vulkan
{

// Configuration for shader compilers
struct CompilerConfig
{
	VkShaderStageFlagBits                        stage{VK_SHADER_STAGE_ALL};
	std::string                                  entry_point{"main"};
	std::unordered_map<std::string, std::string> defines{};

	bool is_valid() const noexcept
	{
		return stage != VK_SHADER_STAGE_ALL && !entry_point.empty();
	}
};

// Base class for shader compilers
class ShaderCompiler
{
  public:
	ShaderCompiler()  = default;
	~ShaderCompiler() = default;

	// Compile shader source code to SPIR-V
	virtual std::vector<uint32_t> compile_spirv(const CompilerConfig &config, const std::vector<uint8_t> &shader_source) const = 0;
};
}        // namespace vulkan
}        // namespace components