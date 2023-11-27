#include <shaders/shader_cache.hpp>
#include <shaders/strategies/offline_strategy.hpp>

namespace vkb
{

ShaderCache *ShaderCache::get()
{
	static ShaderCache instance{};

	return &instance;
}

ShaderCache::ShaderCache() :
    strategy{std::make_unique<OfflineShaderStrategy>()}
{
}

void ShaderCache::set_strategy(std::unique_ptr<ShaderStrategy> &&in_strategy)
{
	strategy = std::move(in_strategy);
}

ShaderPtr ShaderCache::load_shader(const ShaderHandle &handle)
{
	if (!strategy)
	{
		throw std::runtime_error{"Shader cache strategy is not set"};
	}
	return strategy->load_shader(handle);
}

std::vector<uint32_t> ShaderCache::load_spirv(const ShaderHandle &handle)
{
	if (!strategy)
	{
		throw std::runtime_error{"Shader cache strategy is not set"};
	}
	return strategy->load_spirv(handle);
}

ShaderResourceSet ShaderCache::reflect(const ShaderHandle &handle)
{
	if (!strategy)
	{
		throw std::runtime_error{"Shader cache strategy is not set"};
	}
	return strategy->reflect(handle);
}

vk::ShaderModule ShaderCache::create_shader_module(const vk::Device &device, const ShaderHandle &handle)
{
	auto spirv = load_spirv(handle);

	vk::ShaderModuleCreateInfo module_create_info{};
	module_create_info.codeSize = spirv.size() * sizeof(uint32_t);
	module_create_info.pCode    = spirv.data();

	return device.createShaderModule(module_create_info);
}

}        // namespace vkb