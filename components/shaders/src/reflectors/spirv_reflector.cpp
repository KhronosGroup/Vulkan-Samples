#include "shaders/reflectors/spirv_reflector.hpp"

#include <core/util/logging.hpp>

#include <spirv-reflect/spirv_reflect.h>

namespace vkb
{

std::vector<ShaderResource> SpirvReflector::reflect_input_variables(const SpvReflectShaderModule &module) const
{
	std::vector<ShaderResource> resources;

	uint32_t input_variable_count = 0;
	spvReflectEnumerateInputVariables(&module, &input_variable_count, nullptr);
	std::vector<SpvReflectInterfaceVariable *> input_variables(input_variable_count);
	spvReflectEnumerateInputVariables(&module, &input_variable_count, input_variables.data());

	for (const auto *input : input_variables)
	{
		ShaderResource resource;
		resource.array_size = input->array.dims_count;
		resource.binding    = input->location;
		resource.columns    = input->numeric.matrix.column_count;
		resource.vec_size   = input->numeric.vector.component_count;
		resource.location   = input->location;
		resource.name       = input->name;
		resource.qualifiers = input->built_in;
		resource.type       = ShaderResourceType::Input;
		resources.push_back(resource);
	}

	return resources;
}

std::vector<ShaderResource> SpirvReflector::reflect_output_variables(const SpvReflectShaderModule &module) const
{
	std::vector<ShaderResource> resources;

	uint32_t output_variable_count = 0;
	spvReflectEnumerateOutputVariables(&module, &output_variable_count, nullptr);
	std::vector<SpvReflectInterfaceVariable *> output_variables(output_variable_count);
	spvReflectEnumerateOutputVariables(&module, &output_variable_count, output_variables.data());

	for (const auto *output : output_variables)
	{
		if (!output)
		{
			continue;
		}

		ShaderResource resource;
		resource.array_size = output->array.dims_count;
		resource.binding    = output->location;
		resource.columns    = output->numeric.matrix.column_count;
		resource.vec_size   = output->numeric.vector.component_count;
		resource.location   = output->location;
		resource.name       = output->name ? output->name : "";
		resource.qualifiers = output->built_in;
		resource.type       = ShaderResourceType::Output;
		resources.push_back(resource);
	}

	return resources;
}

ShaderResourceType to_shader_resource_type(SpvReflectDescriptorType type)
{
	switch (type)
	{
		case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
			return ShaderResourceType::Sampler;
		case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			return ShaderResourceType::ImageSampler;
		case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			return ShaderResourceType::Image;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			return ShaderResourceType::ImageStorage;
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
			return ShaderResourceType::BufferUniform;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
			return ShaderResourceType::BufferStorage;
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			return ShaderResourceType::BufferUniform;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
			return ShaderResourceType::BufferStorage;
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
			return ShaderResourceType::BufferUniform;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
			return ShaderResourceType::BufferStorage;
		case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
			return ShaderResourceType::InputAttachment;
		case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
			return ShaderResourceType::BufferStorage;

		default:
			LOGE("Unknown descriptor type");
			return ShaderResourceType::All;
	}
}

ShaderResourceMode to_shader_resource_mode(SpvReflectDescriptorType type)
{
	switch (type)
	{
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
			return ShaderResourceMode::Dynamic;
		default:
			return ShaderResourceMode::Static;
	}
}

std::vector<ShaderResource> SpirvReflector::reflect_descriptor_bindings(const SpvReflectShaderModule &module) const
{
	std::vector<ShaderResource> resources;

	uint32_t descriptor_binding_count = 0;
	spvReflectEnumerateDescriptorBindings(&module, &descriptor_binding_count, nullptr);
	std::vector<SpvReflectDescriptorBinding *> descriptor_bindings(descriptor_binding_count);
	spvReflectEnumerateDescriptorBindings(&module, &descriptor_binding_count, descriptor_bindings.data());

	for (const auto *desc_binding : descriptor_bindings)
	{
		ShaderResource resource;
		resource.mode       = to_shader_resource_mode(desc_binding->descriptor_type);
		resource.array_size = desc_binding->array.dims_count;
		resource.binding    = desc_binding->binding;
		resource.name       = desc_binding->name;
		resource.qualifiers = desc_binding->descriptor_type;
		resource.set        = desc_binding->set;
		resource.type       = to_shader_resource_type(desc_binding->descriptor_type);
		resources.push_back(resource);
	}

	return resources;
}

std::vector<ShaderResource> SpirvReflector::reflect_push_constant(const SpvReflectShaderModule &module) const
{
	return {};
}

ShaderResourceSet SpirvReflector::reflect(const std::vector<uint8_t> &code) const
{
	SpvReflectShaderModule module{};
	SpvReflectResult       result = spvReflectCreateShaderModule(code.size(), code.data(), &module);
	assert(result == SPV_REFLECT_RESULT_SUCCESS);

	auto inputs              = reflect_input_variables(module);
	auto outputs             = reflect_output_variables(module);
	auto descriptor_bindings = reflect_descriptor_bindings(module);

	spvReflectDestroyShaderModule(&module);

	std::vector<ShaderResource> resources;

	resources.reserve(
	    inputs.size() +
	    outputs.size() +
	    descriptor_bindings.size());

	resources.insert(resources.end(), inputs.begin(), inputs.end());
	resources.insert(resources.end(), outputs.begin(), outputs.end());
	resources.insert(resources.end(), descriptor_bindings.begin(), descriptor_bindings.end());

	return ShaderResourceSet{std::move(resources)};
}
}        // namespace vkb