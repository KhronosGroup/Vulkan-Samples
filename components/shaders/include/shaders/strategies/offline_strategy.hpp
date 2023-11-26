#pragma once

#include <unordered_map>

#include "shaders/shader_cache.hpp"

namespace vkb
{

// An atlas entry contains a set of variants related to a single shader
struct AtlasShader
{
	std::unordered_map<std::string, ShaderPtr> variants;
};

// An atlas of offline compiled shaders
struct Atlas
{
	std::unordered_map<std::string, AtlasShader> shaders;
};

// An offline shader cache strategy
// It loads shaders from an atlas which is generated offline
class OfflineShaderstrategy : public ShaderStrategy
{
  public:
	OfflineShaderstrategy();
	virtual ~OfflineShaderstrategy() = default;

	virtual ShaderPtr             load_shader(const ShaderHandle &handle) override;
	virtual std::vector<uint32_t> load_spirv(const ShaderHandle &handle) override;
	virtual ShaderResourceSet     reflect(const ShaderHandle &handle) override;

  private:
	void      load_atlas(const std::string &atlas_path);
	ShaderPtr load_shader_from_atlas(const ShaderHandle &handle);

	Atlas atlas;
};
}        // namespace vkb