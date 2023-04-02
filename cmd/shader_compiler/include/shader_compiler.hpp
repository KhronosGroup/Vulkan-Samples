#pragma once

#include <components/shaders/shader_resource.hpp>

namespace shader_compiler
{
struct StageInfo
{
	VkShaderStageFlags                   stage;
	std::vector<uint32_t>                spirv;
	components::shaders::ShaderResources resources;
};

struct ShaderDescriptor;
void compile_and_reflect_shader(ShaderDescriptor &descriptor);
}        // namespace shader_compiler