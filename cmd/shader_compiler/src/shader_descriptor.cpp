#include "shader_descriptor.hpp"

#include <components/common/logging.hpp>

namespace shader_compiler
{
std::string to_string(ShaderType type)
{
	switch (type)
	{
		case ShaderType::Vertex:
			return "Vertex";
		case ShaderType::Fragment:
			return "Fragment";
		case ShaderType::Compute:
			return "Compute";
		case ShaderType::Geometry:
			return "Geometry";
		case ShaderType::TessellationControl:
			return "TessellationControl";
		case ShaderType::TessellationEvaluation:
			return "TessellationEvaluation";
		case ShaderType::RayGeneration:
			return "RayGeneration";
		case ShaderType::Intersection:
			return "Intersection";
		case ShaderType::AnyHit:
			return "AnyHit";
		case ShaderType::ClosestHit:
			return "ClosestHit";
		case ShaderType::Miss:
			return "Miss";
		case ShaderType::Callable:
			return "Callable";
		case ShaderType::Task:
			return "Task";
		case ShaderType::Mesh:
			return "Mesh";
		case ShaderType::Count:
			return "Count";
		default:
			return "Unknown";
	}
}

ShaderType to_shader_type(const std::string &name)
{
	if (name == "Vertex")
	{
		return ShaderType::Vertex;
	}
	else if (name == "Fragment")
	{
		return ShaderType::Fragment;
	}
	else if (name == "Compute")
	{
		return ShaderType::Compute;
	}
	else if (name == "Geometry")
	{
		return ShaderType::Geometry;
	}
	else if (name == "TessellationControl")
	{
		return ShaderType::TessellationControl;
	}
	else if (name == "TessellationEvaluation")
	{
		return ShaderType::TessellationEvaluation;
	}
	else if (name == "RayGeneration")
	{
		return ShaderType::RayGeneration;
	}
	else if (name == "Intersection")
	{
		return ShaderType::Intersection;
	}
	else if (name == "AnyHit")
	{
		return ShaderType::AnyHit;
	}
	else if (name == "ClosestHit")
	{
		return ShaderType::ClosestHit;
	}
	else if (name == "Miss")
	{
		return ShaderType::Miss;
	}
	else if (name == "Callable")
	{
		return ShaderType::Callable;
	}
	else if (name == "Task")
	{
		return ShaderType::Task;
	}
	else if (name == "Mesh")
	{
		return ShaderType::Mesh;
	}
	else if (name == "Count")
	{
		return ShaderType::Count;
	}
	else
	{
		return ShaderType::Count;
	}
}

VkShaderStageFlagBits to_vk_shader_stage(ShaderType type)
{
	switch (type)
	{
		case ShaderType::Vertex:
			return VK_SHADER_STAGE_VERTEX_BIT;
		case ShaderType::Fragment:
			return VK_SHADER_STAGE_FRAGMENT_BIT;
		case ShaderType::Compute:
			return VK_SHADER_STAGE_COMPUTE_BIT;
		case ShaderType::Geometry:
			return VK_SHADER_STAGE_GEOMETRY_BIT;
		case ShaderType::TessellationControl:
			return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		case ShaderType::TessellationEvaluation:
			return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		case ShaderType::RayGeneration:
			return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		case ShaderType::Intersection:
			return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
		case ShaderType::AnyHit:
			return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
		case ShaderType::ClosestHit:
			return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		case ShaderType::Miss:
			return VK_SHADER_STAGE_MISS_BIT_KHR;
		case ShaderType::Callable:
			return VK_SHADER_STAGE_CALLABLE_BIT_KHR;
		case ShaderType::Task:
			return VK_SHADER_STAGE_TASK_BIT_NV;
		case ShaderType::Mesh:
			return VK_SHADER_STAGE_MESH_BIT_NV;
		case ShaderType::Count:
			return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
		default:
			return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
	}
}

ShaderDescriptor shader_descriptor_from_json(const json &json)
{
	ShaderDescriptor descriptor;

	descriptor.name        = json["name"];
	descriptor.language    = json["language"];
	descriptor.path        = json["path"];
	descriptor.entry_point = json["entry_point"];
	descriptor.type        = to_shader_type(json["type"]);

	if (json.contains("include_folders"))
	{
		descriptor.include_folders = json["include_folders"];
	}

	if (json.contains("defines"))
	{
		descriptor.defines = json["defines"];
	}

	if (json.contains("variant_defines"))
	{
		descriptor.variant_defines = json["variant_defines"];
	}

	return descriptor;
}

void print_shader_descriptor(const ShaderDescriptor &descriptor)
{
	using namespace components::logging;

	LOGI("Name: {}", descriptor.name.c_str());
	LOGI("Language: {}", descriptor.language.c_str());
	LOGI("Path: {}", descriptor.path.c_str());
	LOGI("Entry Point: {}", descriptor.entry_point.c_str());
	LOGI("Type: {}", to_string(descriptor.type).c_str());

	for (auto &include_folder : descriptor.include_folders)
	{
		LOGI("Include Folder: {}{}{}", colors::blue, include_folder.c_str(), colors::reset);
	}

	for (auto &define : descriptor.defines)
	{
		LOGI("Define: {}", define.c_str());
	}

	for (auto &variant_define : descriptor.variant_defines)
	{
		LOGI("Variant Define: {}", variant_define.c_str());
	}
}
}        // namespace shader_compiler