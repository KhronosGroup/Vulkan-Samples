#include "shaders/shader_resources.hpp"

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

nlohmann::json ShaderResource::to_json() const
{
	nlohmann::json json;
	json["name"] = name;
	json["type"] = to_string(type);
	json["mode"] = to_string(mode);

#define SET_IF_NOT_MAX(json, field) \
	if (field == UINT32_MAX)        \
	{                               \
		json[#field] = nullptr;     \
	}                               \
	else                            \
	{                               \
		json[#field] = field;       \
	}

	SET_IF_NOT_MAX(json, set);
	SET_IF_NOT_MAX(json, binding);
	SET_IF_NOT_MAX(json, location);
	SET_IF_NOT_MAX(json, input_attachment_index);
	SET_IF_NOT_MAX(json, vec_size);
	SET_IF_NOT_MAX(json, columns);
	SET_IF_NOT_MAX(json, array_size);
	SET_IF_NOT_MAX(json, offset);
	SET_IF_NOT_MAX(json, size);
	SET_IF_NOT_MAX(json, constant_id);
	SET_IF_NOT_MAX(json, qualifiers);

	return json;

#undef SET_IF_NOT_MAX
}
}        // namespace vkb