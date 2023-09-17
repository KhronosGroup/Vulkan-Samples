#pragma once

#include <algorithm>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

#include <core/util/hash.hpp>

namespace vkb
{
const std::string HAS_BASE_COLOR_TEXTURE         = "HAS_BASE_COLOR_TEXTURE";
const std::string HAS_NORMAL_TEXTURE             = "HAS_NORMAL_TEXTURE";
const std::string HAS_METALLIC_ROUGHNESS_TEXTURE = "HAS_METALLIC_ROUGHNESS_TEXTURE";

class ShaderBuilder
{
  public:
	ShaderBuilder()          = default;
	virtual ~ShaderBuilder() = default;

	ShaderBuilder &with_path(const std::string &path)
	{
		this->path = path;
		return *this;
	}

	ShaderBuilder &with_define(const std::string &define)
	{
		defines.push_back(define);
		return *this;
	}

	ShaderHandle build()
	{
		HashBuilder builder;

		std::sort(defines.begin(), defines.end());

		builder.with(path);

		for (auto &define : defines)
		{
			builder.with(define);
		}

		return {builder.build(), path, defines};
	}

  private:
	std::string              path;
	std::vector<std::string> defines;
};

struct ShaderHandle
{
	size_t                   hash{0};
	std::string              path;
	std::vector<std::string> defines;

	bool operator==(const ShaderHandle &other) const
	{
		return hash == other.hash;
	}

	bool operator!=(const ShaderHandle &other) const
	{
		return !(*this == other);
	}
};
}        // namespace vkb

namespace std
{
template <>
struct hash<vkb::ShaderHandle>
{
	size_t operator()(const vkb::ShaderHandle &handle) const
	{
		return handle.hash;
	}
};
}        // namespace std
