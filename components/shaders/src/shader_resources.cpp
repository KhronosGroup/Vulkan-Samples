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
}        // namespace vkb