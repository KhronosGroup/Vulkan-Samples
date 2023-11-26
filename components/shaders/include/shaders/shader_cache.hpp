#pragma once

#include <algorithm>
#include <memory>
#include <numeric>
#include <string>
#include <unordered_map>
#include <vector>

#include "shaders/shader_handle.hpp"
#include "shaders/shader_resources.hpp"

namespace vkb
{

// SPIRV shader code and its resource reflection
struct Shader
{
	std::vector<uint32_t> code;
	ShaderResourceSet     resource_set;
};

// A shader cache strategy is responsible for loading and reflecting shaders
class ShaderCacheStrategy
{
  public:
	ShaderCacheStrategy()          = default;
	virtual ~ShaderCacheStrategy() = default;

	virtual Shader                load_shader(const ShaderHandle &handle)    = 0;
	virtual std::vector<uint32_t> load_spirv(const ShaderHandle &handle)     = 0;
	virtual ShaderResourceSet     reflect(const std::vector<uint32_t> &code) = 0;
};

class ShaderCache
{
  public:
	ShaderCache(std::unique_ptr<ShaderCacheStrategy> &&strategy) :
	    strategy{std::move(strategy)}
	{}

	Shader load_shader(const ShaderHandle &handle)
	{
		assert(strategy != nullptr && "Shader cache strategy is not set");

		if (cached_shaders.find(handle) == cached_shaders.end())
		{
			auto code              = strategy->load_shader(handle);
			auto resource_set      = strategy->reflect(code);
			cached_shaders[handle] = {code, resource_set};
		}

		return cached_shaders[handle];
	}

  private:
	std::shared_ptr<ShaderCacheStrategy> strategy = nullptr;

	std::unordered_map<ShaderHandle, Shader> cached_shaders;
};
}        // namespace vkb