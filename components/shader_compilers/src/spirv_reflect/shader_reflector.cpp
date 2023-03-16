#include "components/spirv_reflect/shader_reflector.hpp"

#include <stdexcept>

#include <spirv_reflect.h>

namespace components
{
using namespace vulkan;

namespace spirv_reflect
{
namespace detail
{
inline std::string result_c_str(SpvReflectResult result)
{
#define SPIRV_REFLECT_RESULT_CASE(r) \
	case r:                          \
		return #r;

	switch (result)
	{
		SPIRV_REFLECT_RESULT_CASE(SPV_REFLECT_RESULT_SUCCESS)
		SPIRV_REFLECT_RESULT_CASE(SPV_REFLECT_RESULT_NOT_READY)
		SPIRV_REFLECT_RESULT_CASE(SPV_REFLECT_RESULT_ERROR_PARSE_FAILED)
		SPIRV_REFLECT_RESULT_CASE(SPV_REFLECT_RESULT_ERROR_ALLOC_FAILED)
		SPIRV_REFLECT_RESULT_CASE(SPV_REFLECT_RESULT_ERROR_RANGE_EXCEEDED)
		SPIRV_REFLECT_RESULT_CASE(SPV_REFLECT_RESULT_ERROR_NULL_POINTER)
		SPIRV_REFLECT_RESULT_CASE(SPV_REFLECT_RESULT_ERROR_INTERNAL_ERROR)
		SPIRV_REFLECT_RESULT_CASE(SPV_REFLECT_RESULT_ERROR_COUNT_MISMATCH)
		SPIRV_REFLECT_RESULT_CASE(SPV_REFLECT_RESULT_ERROR_ELEMENT_NOT_FOUND)
		SPIRV_REFLECT_RESULT_CASE(SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_CODE_SIZE)
		SPIRV_REFLECT_RESULT_CASE(SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_MAGIC_NUMBER)
		SPIRV_REFLECT_RESULT_CASE(SPV_REFLECT_RESULT_ERROR_SPIRV_UNEXPECTED_EOF)
		SPIRV_REFLECT_RESULT_CASE(SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_ID_REFERENCE)
		SPIRV_REFLECT_RESULT_CASE(SPV_REFLECT_RESULT_ERROR_SPIRV_SET_NUMBER_OVERFLOW)
		SPIRV_REFLECT_RESULT_CASE(SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_STORAGE_CLASS)
		SPIRV_REFLECT_RESULT_CASE(SPV_REFLECT_RESULT_ERROR_SPIRV_RECURSION)
		SPIRV_REFLECT_RESULT_CASE(SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_INSTRUCTION)
		SPIRV_REFLECT_RESULT_CASE(SPV_REFLECT_RESULT_ERROR_SPIRV_UNEXPECTED_BLOCK_DATA)
		SPIRV_REFLECT_RESULT_CASE(SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_BLOCK_MEMBER_REFERENCE)
		SPIRV_REFLECT_RESULT_CASE(SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_ENTRY_POINT)
		SPIRV_REFLECT_RESULT_CASE(SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_EXECUTION_MODE)
		default:
			return "Unknown SPIRV Reflect result";
	}
}

inline VkFormat spv_reflect_format_to_vk_format(SpvReflectFormat format)
{
#define CASE(x)                  \
	case SPV_REFLECT_FORMAT_##x: \
		return VK_FORMAT_##x;

	switch (format)
	{
		CASE(UNDEFINED)
		CASE(R32_UINT)
		CASE(R32_SINT)
		CASE(R32_SFLOAT)
		CASE(R32G32_UINT)
		CASE(R32G32_SINT)
		CASE(R32G32_SFLOAT)
		CASE(R32G32B32_UINT)
		CASE(R32G32B32_SINT)
		CASE(R32G32B32_SFLOAT)
		CASE(R32G32B32A32_UINT)
		CASE(R32G32B32A32_SINT)
		CASE(R32G32B32A32_SFLOAT)
		CASE(R64_UINT)
		CASE(R64_SINT)
		CASE(R64_SFLOAT)
		CASE(R64G64_UINT)
		CASE(R64G64_SINT)
		CASE(R64G64_SFLOAT)
		CASE(R64G64B64_UINT)
		CASE(R64G64B64_SINT)
		CASE(R64G64B64_SFLOAT)
		CASE(R64G64B64A64_UINT)
		CASE(R64G64B64A64_SINT)
		CASE(R64G64B64A64_SFLOAT)
		default:
			return VK_FORMAT_UNDEFINED;
	}
}

inline ShaderResourceType storage_class_to_resource_type(SpvStorageClass storage_class)
{
	switch (storage_class)
	{
		case SpvStorageClassUniformConstant:
		case SpvStorageClassUniform:
			return ShaderResourceType::Uniform;
		case SpvStorageClassInput:
			return ShaderResourceType::Input;
		case SpvStorageClassOutput:
			return ShaderResourceType::Output;
		case SpvStorageClassPushConstant:
			return ShaderResourceType::PushConstant;
		default:
			return ShaderResourceType::Unknown;
	}
}

#define REFLECT_CHECK(x, message)                                                                                               \
	if (x != SPV_REFLECT_RESULT_SUCCESS)                                                                                        \
	{                                                                                                                           \
		std::string msg = std::string{"SPIRV Reflect failed:\n\tResult:"} + detail::result_c_str(x) + "\n\tMessage:" + message; \
		throw std::runtime_error(msg);                                                                                          \
	}
}        // namespace detail

std::unique_ptr<RawShaderData> process_resource_type(const SpvReflectTypeDescription &description);

std::unique_ptr<RawShaderData> process_numeric_type(const SpvReflectTypeDescription &description)
{
	auto data = std::make_unique<NumericShaderData>();

	switch (description.op)
	{
		case SpvOpTypeBool:
			data->type = ShaderRawDataType::Bool;
			break;
		case SpvOpTypeInt:
			data->type = ShaderRawDataType::Int;
			break;
		case SpvOpTypeFloat:
			data->type = ShaderRawDataType::Float;
			break;
		default:
			throw std::runtime_error("Unsupported numeric type");
	}

	data->type_name          = description.type_name ? description.type_name : "";
	data->struct_member_name = description.struct_member_name ? description.struct_member_name : "";
	data->width              = description.traits.numeric.scalar.width;
	data->is_signed          = description.traits.numeric.scalar.signedness;

	return data;
}

std::unique_ptr<RawShaderData> process_vector_type(const SpvReflectTypeDescription &description)
{
	auto data                = std::make_unique<VectorShaderData>();
	data->type               = ShaderRawDataType::Vector;
	data->type_name          = description.type_name ? description.type_name : "";
	data->struct_member_name = description.struct_member_name ? description.struct_member_name : "";
	data->component_count    = description.traits.numeric.vector.component_count;
	data->component_width    = description.traits.numeric.scalar.width;
	data->is_signed          = !!description.traits.numeric.scalar.signedness;
	return data;
}

std::unique_ptr<RawShaderData> process_matrix_type(const SpvReflectTypeDescription &description)
{
	auto data                = std::make_unique<MatrixShaderData>();
	data->type               = ShaderRawDataType::Matrix;
	data->type_name          = description.type_name ? description.type_name : "";
	data->struct_member_name = description.struct_member_name ? description.struct_member_name : "";
	return data;
}

std::unique_ptr<RawShaderData> process_array_type(const SpvReflectTypeDescription &description)
{
	if (description.traits.array.dims_count != 1)
	{
		throw std::runtime_error("Only 1D arrays are supported for now. (TODO: Support multidimensional arrays)");
	}

	auto data                = std::make_unique<ArrayShaderData>();
	data->type               = ShaderRawDataType::Array;
	data->type_name          = description.type_name ? description.type_name : "";
	data->struct_member_name = description.struct_member_name ? description.struct_member_name : "";
	data->element_count      = description.traits.array.dims[0];

	for (size_t i = 0; i < description.member_count; i++)
	{
		SpvReflectTypeDescription member = description.members[i];
		data->members.push_back(process_resource_type(member));
	}

	return data;
}

std::unique_ptr<RawShaderData> process_struct_type(const SpvReflectTypeDescription &description)
{
	auto data                = std::make_unique<StructShaderData>();
	data->type               = ShaderRawDataType::Struct;
	data->type_name          = description.type_name ? description.type_name : "";
	data->struct_member_name = description.struct_member_name ? description.struct_member_name : "";

	for (size_t i = 0; i < description.member_count; i++)
	{
		SpvReflectTypeDescription member = description.members[i];
		data->members.push_back(process_resource_type(member));
	}

	return data;
}

std::unique_ptr<RawShaderData> process_resource_type(const SpvReflectTypeDescription &description)
{
	switch (description.op)
	{
		case SpvOpTypeStruct:
			return process_struct_type(description);

		case SpvOpTypeBool:
		case SpvOpTypeInt:
		case SpvOpTypeFloat:
			return process_numeric_type(description);

		case SpvOpTypeVector:
			return process_vector_type(description);

		case SpvOpTypeMatrix:
			return process_matrix_type(description);

		case SpvOpTypeImage:
		case SpvOpTypeSampler:
		case SpvOpTypeSampledImage:
		case SpvOpTypeArray:
			return process_array_type(description);
		case SpvOpTypeRuntimeArray:
		case SpvOpTypeOpaque:
		case SpvOpTypePointer:
		case SpvOpTypeFunction:
		case SpvOpTypeEvent:
		case SpvOpTypeDeviceEvent:
		case SpvOpTypeReserveId:
		case SpvOpTypeQueue:
		case SpvOpTypePipe:
		case SpvOpTypeForwardPointer:
		case SpvOpTypeVoid:
			return RawShaderData::create_unknown();

		default:
			return RawShaderData::create_unknown();
	}
}

ShaderResource process_resource(const SpvReflectInterfaceVariable &variable)
{
	ShaderResource resource;
	resource.name       = variable.name;
	resource.type       = detail::storage_class_to_resource_type(variable.storage_class);
	resource.qualifiers = ShaderResourceQualifiers::ReadWrite;
	resource.data       = variable.type_description ? process_resource_type(*variable.type_description) : RawShaderData::create_unknown();

	if (variable.location != UINT32_MAX)
	{
		resource.lookup.location = variable.location;
	}

	return resource;
}

ShaderResources SpirvReflectShaderReflector::reflect_spirv(const std::vector<uint32_t> &spirv) const
{
	SpvReflectShaderModule module;
	REFLECT_CHECK(spvReflectCreateShaderModule(spirv.size() * sizeof(uint32_t), spirv.data(), &module), "Failed to create shader module");

	vulkan::ShaderResources                resources;
	std::unordered_map<uint32_t, uint32_t> spirv_id_to_resource_index;

	auto get_resource_at_index = [&](uint32_t index) -> ShaderResource * {
		if (resources.resources.size() <= index)
		{
			return nullptr;
		}
		return &resources.resources[index];
	};

	auto get_resource_from_spirv_id = [&](uint32_t spirv_id) -> ShaderResource * {
		if (spirv_id_to_resource_index.find(spirv_id) == spirv_id_to_resource_index.end())
		{
			return nullptr;
		}
		return get_resource_at_index(spirv_id_to_resource_index[spirv_id]);
	};

	// Process all interfaces first

	uint32_t resource_count = 0;
	REFLECT_CHECK(spvReflectEnumerateInterfaceVariables(&module, &resource_count, nullptr), "Failed to enumerate interface variables");
	std::vector<SpvReflectInterfaceVariable *> variables(resource_count);
	REFLECT_CHECK(spvReflectEnumerateInterfaceVariables(&module, &resource_count, variables.data()), "Failed to enumerate interface variables");

	for (auto *variable : variables)
	{
		if (strcmp(variable->name, "") == 0)
		{
			continue;
		}

		resources.resources.push_back(process_resource(*variable));
		spirv_id_to_resource_index[variable->spirv_id] = static_cast<uint32_t>(resources.resources.size() - 1);
	}

	// Process descriptor sets

	uint32_t descriptor_set_count = 0;
	REFLECT_CHECK(spvReflectEnumerateDescriptorSets(&module, &descriptor_set_count, nullptr), "Failed to enumerate descriptor sets");
	std::vector<SpvReflectDescriptorSet *> descriptor_sets(descriptor_set_count);
	REFLECT_CHECK(spvReflectEnumerateDescriptorSets(&module, &descriptor_set_count, descriptor_sets.data()), "Failed to enumerate descriptor sets");

	for (auto *set : descriptor_sets)
	{
		for (size_t i = 0; i < set->binding_count; i++)
		{
			auto *binding  = set->bindings[i];
			auto *resource = get_resource_from_spirv_id(binding->spirv_id);
			if (resource == nullptr)
			{
				continue;
			}

			resource->lookup.set     = set->set;
			resource->lookup.binding = binding->binding;

			// TODO: only set this for subpass attachments
			//  resource->lookup.input_attachment_index = binding->input_attachment_index;
		}
	}

	// Process push constants

	uint32_t push_constant_block_count = 0;
	REFLECT_CHECK(spvReflectEnumeratePushConstantBlocks(&module, &push_constant_block_count, nullptr), "Failed to enumerate push constant blocks");
	std::vector<SpvReflectBlockVariable *> push_constant_blocks(push_constant_block_count);
	REFLECT_CHECK(spvReflectEnumeratePushConstantBlocks(&module, &push_constant_block_count, push_constant_blocks.data()), "Failed to enumerate push constant blocks");

	for (auto *block : push_constant_blocks)
	{
		auto *resource = get_resource_from_spirv_id(block->spirv_id);
		if (resource == nullptr)
		{
			continue;
		}

		resource->lookup.push_constant_offset = block->offset;
	}

	spvReflectDestroyShaderModule(&module);

	return resources;
}
}        // namespace spirv_reflect
}        // namespace components