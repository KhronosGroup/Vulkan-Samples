#pragma once

#include "shader_compiler.hpp"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace shader_compiler
{

enum class ShaderType
{
	Vertex,
	Fragment,
	Compute,
	Geometry,
	TessellationControl,
	TessellationEvaluation,
	RayGeneration,
	Intersection,
	AnyHit,
	ClosestHit,
	Miss,
	Callable,
	Task,
	Mesh,
	Count
};

std::string to_string(ShaderType type);

ShaderType to_shader_type(const std::string &name);

VkShaderStageFlagBits to_vk_shader_stage(ShaderType type);

struct ShaderVariant
{
	size_t                               hash;
	std::vector<std::string>             defines;
	components::shaders::ShaderResources resources;
	std::vector<uint32_t>                spirv;
};

struct ShaderDescriptor
{
	std::string name;
	std::string language;
	std::string path;
	std::string entry_point;
	ShaderType  type;

	std::vector<std::string> include_folders;
	std::vector<std::string> defines;
	std::vector<std::string> variant_defines;

	std::unordered_map<size_t, ShaderVariant> variants;
};

ShaderDescriptor shader_descriptor_from_json(const json &json);

void print_shader_descriptor(const ShaderDescriptor &descriptor);
}        // namespace shader_compiler