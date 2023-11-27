#pragma once

#include <algorithm>
#include <memory>
#include <numeric>
#include <set>
#include <string>
#include <vector>

#include <core/util/hash.hpp>

namespace vkb
{
const std::string HAS_BASE_COLOR_TEXTURE         = "HAS_BASE_COLOR_TEXTURE=1";
const std::string HAS_NORMAL_TEXTURE             = "HAS_NORMAL_TEXTURE=1";
const std::string HAS_METALLIC_ROUGHNESS_TEXTURE = "HAS_METALLIC_ROUGHNESS_TEXTURE=1";

struct ShaderHandle
{
	std::string              hash{0};
	std::string              path;
	std::string              define_hash{0};
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

class ShaderHandleBuilder
{
  public:
	ShaderHandleBuilder()          = default;
	virtual ~ShaderHandleBuilder() = default;

	ShaderHandleBuilder &with_path(const std::string &path)
	{
		this->path = path;
		return *this;
	}

	ShaderHandleBuilder &with_define(const std::string &define)
	{
		defines.emplace(define);
		return *this;
	}

	ShaderHandleBuilder &with_defines(const std::vector<std::string> &in_defines)
	{
		defines.insert(in_defines.begin(), in_defines.end());
		return *this;
	}

	ShaderHandle build()
	{
		std::string define_str = std::accumulate(defines.begin(), defines.end(), std::string(), [](const std::string &a, const std::string &b) {
			return a + b;
		});

		return {sha256(path + define_str), path, sha256(define_str), {defines.begin(), defines.end()}};
	}

  private:
	std::string           path;
	std::set<std::string> defines;
};

}        // namespace vkb

namespace std
{
template <>
struct hash<vkb::ShaderHandle>
{
	std::size_t operator()(const vkb::ShaderHandle &handle) const
	{
		return std::hash<std::string>()(handle.hash);
	}
};
}        // namespace std