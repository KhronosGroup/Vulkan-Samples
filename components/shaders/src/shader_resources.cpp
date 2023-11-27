#include "shaders/shader_resources.hpp"

#include <algorithm>

#include <core/util/logging.hpp>

namespace vkb
{
std::string to_string(const ShaderResourceType &type)
{
#define STR(r)                  \
	case ShaderResourceType::r: \
		return #r
	switch (type)
	{
		STR(Input);
		STR(InputAttachment);
		STR(Output);
		STR(Image);
		STR(ImageSampler);
		STR(ImageStorage);
		STR(Sampler);
		STR(BufferUniform);
		STR(BufferStorage);
		STR(PushConstant);
		STR(SpecializationConstant);
		STR(All);
	}
#undef STR
	return "Unknown";
}

std::string to_string(const ShaderResourceMode &mode)
{
#define STR(r)                  \
	case ShaderResourceMode::r: \
		return #r
	switch (mode)
	{
		STR(Static);
		STR(Dynamic);
		STR(UpdateAfterBind);
	}
#undef STR
	return "Unknown";
}

const std::vector<ShaderResource> &ShaderResourceSet::resources() const
{
	return m_resources;
}

std::vector<ShaderResource> ShaderResourceSet::resources(const ShaderResourceType &type) const
{
	std::vector<ShaderResource> result;
	for (const auto &resource : m_resources)
	{
		if (resource.type == type)
		{
			result.push_back(resource);
		}
	}
	return result;
}

void ShaderResourceSet::update_resource_mode(const std::string &name, const ShaderResourceMode &mode)
{
	auto it = std::find_if(m_resources.begin(), m_resources.end(), [&name](const ShaderResource &resource) { return resource.name == name; });

	if (it == m_resources.end())
	{
		LOGW("Resource `{}` not found for shader. Could not update ShaderResourceMode", name);
		return;
	}

	if (mode == ShaderResourceMode::Dynamic && it->type != ShaderResourceType::BufferUniform && it->type != ShaderResourceType::BufferStorage)
	{
		LOGW("Resource `{}` does not support dynamic.", name);
		return;
	}

	it->mode = mode;
	LOGI("ShaderResourceMode for `{}` was updated", name);
}
}        // namespace vkb