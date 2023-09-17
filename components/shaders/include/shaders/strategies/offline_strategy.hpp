#pragma once

#include "shaders/shader_cache.hpp"

namespace vkb
{
class OfflineShaderCacheStrategy : public ShaderCacheStrategy
{
  public:
	OfflineShaderCacheStrategy()          = default;
	virtual ~OfflineShaderCacheStrategy() = default;

	virtual std::vector<uint32_t> load_shader(const ShaderHandle &handle) override;

	virtual ShaderResourceSet reflect(const std::vector<uint32_t> &code) override;
};
}        // namespace vkb