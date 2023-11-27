#pragma once

#include <algorithm>
#include <memory>
#include <numeric>
#include <string>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.hpp>

#include "shaders/shader_handle.hpp"
#include "shaders/shader_resources.hpp"

namespace vkb
{

// SPIRV shader code and its resource reflection
class Shader
{
  public:
	Shader(std::vector<uint32_t> &&code, ShaderResourceSet &&resource_set, vk::ShaderStageFlagBits stage) :
	    code{std::move(code)},
	    resource_set{std::move(resource_set)},
	    stage{stage}
	{
	}
	Shader(const Shader &)            = delete;
	Shader &operator=(const Shader &) = delete;

	Shader(Shader &&)            = default;
	Shader &operator=(Shader &&) = default;

	~Shader() = default;

	std::vector<uint32_t>   code;
	ShaderResourceSet       resource_set;
	vk::ShaderStageFlagBits stage;
};

using ShaderPtr = std::shared_ptr<Shader>;

// A shader cache strategy is responsible for loading and reflecting shaders
class ShaderStrategy
{
  public:
	ShaderStrategy()          = default;
	virtual ~ShaderStrategy() = default;

	virtual ShaderPtr             load_shader(const ShaderHandle &handle) = 0;
	virtual std::vector<uint32_t> load_spirv(const ShaderHandle &handle)  = 0;
	virtual ShaderResourceSet     reflect(const ShaderHandle &handle)     = 0;
};

// A shader cache singleton to load and reflect shaders using a strategy
class ShaderCache : public ShaderStrategy
{
  public:
	~ShaderCache() = default;

	static ShaderCache *get();

	void                  set_strategy(std::unique_ptr<ShaderStrategy> &&strategy);
	ShaderPtr             load_shader(const ShaderHandle &handle);
	std::vector<uint32_t> load_spirv(const ShaderHandle &handle);
	ShaderResourceSet     reflect(const ShaderHandle &handle);

	vk::ShaderModule create_shader_module(const vk::Device &device, const ShaderHandle &handle);

  private:
	ShaderCache();

	std::unique_ptr<ShaderStrategy> strategy;
};

}        // namespace vkb