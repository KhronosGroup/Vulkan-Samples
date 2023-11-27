#pragma once

#include <string>
#include <vector>

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
	std::string        name;
	ShaderResourceType type;
	ShaderResourceMode mode;

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
