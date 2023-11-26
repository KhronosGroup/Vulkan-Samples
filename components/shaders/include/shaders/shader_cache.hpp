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
class Shader
{
  public:
	Shader(std::vector<uint32_t> &&code, ShaderResourceSet &&resource_set) :
	    code{std::move(code)},
	    resource_set{std::move(resource_set)}
	{
	}
	Shader(const Shader &)            = delete;
	Shader &operator=(const Shader &) = delete;

	Shader(Shader &&)            = default;
	Shader &operator=(Shader &&) = default;

	~Shader() = default;

	std::vector<uint32_t> code;
	ShaderResourceSet     resource_set;
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
	static ShaderCache *get()
	{
		static ShaderCache instance;
		return &instance;
	}

	~ShaderCache() = default;

	void set_strategy(std::unique_ptr<ShaderStrategy> &&strategy)
	{
		this->strategy = std::move(strategy);
	}

	ShaderPtr load_shader(const ShaderHandle &handle)
	{
		assert(strategy);
		return strategy->load_shader(handle);
	}

	std::vector<uint32_t> load_spirv(const ShaderHandle &handle)
	{
		assert(strategy);
		return strategy->load_spirv(handle);
	}

	ShaderResourceSet reflect(const ShaderHandle &handle)
	{
		assert(strategy);
		return strategy->reflect(handle);
	}

  private:
	ShaderCache() = default;

	std::unique_ptr<ShaderStrategy> strategy;
};

}        // namespace vkb