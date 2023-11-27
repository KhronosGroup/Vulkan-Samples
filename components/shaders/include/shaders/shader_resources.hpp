#pragma once

#include <string>
#include <vector>

#include <core/util/hash.hpp>
#include <vulkan/vulkan.hpp>

namespace vkb
{

/// Types of shader resources
enum class ShaderResourceType
{
	Input,
	InputAttachment,
	Output,
	Image,
	ImageSampler,
	ImageStorage,
	Sampler,
	BufferUniform,
	BufferStorage,
	PushConstant,
	SpecializationConstant,
	All
};

std::string to_string(const ShaderResourceType &type);

/// This determines the type and method of how descriptor set should be created and bound
enum class ShaderResourceMode
{
	Static,
	Dynamic,
	UpdateAfterBind
};

std::string to_string(const ShaderResourceMode &mode);

/// A bitmask of qualifiers applied to a resource
struct ShaderResourceQualifiers
{
	enum : uint32_t
	{
		None        = 0,
		NonReadable = 1,
		NonWritable = 2,
	};
};

/// Store shader resource data.
/// Used by the shader module.
struct ShaderResource
{
	std::string          name;
	ShaderResourceType   type;
	ShaderResourceMode   mode;
	vk::ShaderStageFlags stages;

	uint32_t set                    = UINT32_MAX;
	uint32_t binding                = UINT32_MAX;
	uint32_t location               = UINT32_MAX;
	uint32_t input_attachment_index = UINT32_MAX;
	uint32_t vec_size               = UINT32_MAX;
	uint32_t columns                = UINT32_MAX;
	uint32_t array_size             = UINT32_MAX;
	uint32_t offset                 = UINT32_MAX;
	uint32_t size                   = UINT32_MAX;
	uint32_t constant_id            = UINT32_MAX;
	uint32_t qualifiers             = UINT32_MAX;
};        // namespace vkb

class ShaderResourceSet
{
  public:
	ShaderResourceSet() = default;

	ShaderResourceSet(vk::ShaderStageFlagBits stage, std::vector<ShaderResource> &&resources) :
	    m_stage{stage},
	    m_resources{std::move(resources)}
	{
		for (auto &resource : m_resources)
		{
			resource.stages |= stage;
		}
	};

	~ShaderResourceSet() = default;

	// Get all resources
	const std::vector<ShaderResource> &resources() const;

	// Get all resources of a specific type
	std::vector<ShaderResource> resources(const ShaderResourceType &type) const;

	// Update the resource mode of a specific resource
	void update_resource_mode(const std::string &name, const ShaderResourceMode &mode);

	vk::ShaderStageFlagBits stage() const
	{
		return m_stage;
	}

	VkShaderStageFlagBits c_stage() const
	{
		return static_cast<VkShaderStageFlagBits>(m_stage);
	}

  private:
	vk::ShaderStageFlagBits     m_stage;
	std::vector<ShaderResource> m_resources;
};
}        // namespace vkb

namespace std
{
template <>
struct hash<vkb::ShaderResource>
{
	inline size_t operator()(const vkb::ShaderResource &resource) const
	{
		std::size_t result = 0;

		if (resource.type == vkb::ShaderResourceType::Input ||
		    resource.type == vkb::ShaderResourceType::Output ||
		    resource.type == vkb::ShaderResourceType::PushConstant ||
		    resource.type == vkb::ShaderResourceType::SpecializationConstant)
		{
			return result;
		}

		vkb::hash_combine(result, resource.set);
		vkb::hash_combine(result, resource.binding);
		vkb::hash_combine(result, static_cast<std::underlying_type<vkb::ShaderResourceType>::type>(resource.type));
		vkb::hash_combine(result, resource.mode);

		return result;
	}
};

template <>
struct hash<vkb::ShaderResourceSet>
{
	inline size_t operator()(const vkb::ShaderResourceSet &resource_set) const
	{
		size_t h = 0;
		for (auto &resource : resource_set.resources())
		{
			h ^= hash<vkb::ShaderResource>{}(resource);
		}
		return h;
	}
};
}        // namespace std