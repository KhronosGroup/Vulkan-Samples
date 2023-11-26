#pragma once

#include <unordered_map>

#include "shaders/shader_cache.hpp"

namespace vkb
{

// An atlas entry contains a set of variants related to a single shader
struct AtlasShader
{
	std::unordered_map<std::string, Shader> variants;
};

// An atlas of offline compiled shaders
struct Atlas
{
	std::unordered_map<std::string, AtlasShader> shaders;
};

// An offline shader cache strategy
// It loads shaders from an atlas which is generated offline
class OfflineShaderCacheStrategy : public ShaderCacheStrategy
{
  public:
	OfflineShaderCacheStrategy();
	virtual ~OfflineShaderCacheStrategy() = default;

	virtual std::vector<uint32_t> load_shader(const ShaderHandle &handle) override;

	virtual ShaderResourceSet reflect(const std::vector<uint32_t> &code) override;

  private:
	void load_atlas(const std::string &atlas_path);

	Atlas atlas;
};
}        // namespace vkb