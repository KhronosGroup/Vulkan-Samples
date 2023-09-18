#pragma once

#include <string>
#include <vector>
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

/// This determines the type and method of how descriptor set should be created and bound
enum class ShaderResourceMode
{
	Static,
	Dynamic,
	UpdateAfterBind
};

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
	std::string        name;
	ShaderResourceType type;

	uint32_t set;
	uint32_t binding;
	uint32_t location;
	uint32_t input_attachment_index;
	uint32_t vec_size;
	uint32_t columns;
	uint32_t array_size;
	uint32_t offset;
	uint32_t size;
	uint32_t constant_id;
	uint32_t qualifiers;
};

class ShaderResourceSet
{
  public:
	ShaderResourceSet(std::vector<ShaderResource> &&resources) :
	    resources{std::move(resources)} {};

	~ShaderResourceSet() = default;

	const std::vector<ShaderResource> &get_resources() const
	{
		return resources;
	}

	std::vector<ShaderResource> get_resources(const ShaderResourceType &type) const
	{
		std::vector<ShaderResource> result;
		for (const auto &resource : resources)
		{
			if (resource.type == type)
			{
				result.push_back(resource);
			}
		}
		return result;
	}

  private:
	std::vector<ShaderResource> resources;
};
}        // namespace vkb
