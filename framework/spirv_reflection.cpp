/* Copyright (c) 2019-2020, Arm Limited and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "spirv_reflection.h"

namespace vkb
{
namespace
{
template <ShaderResourceType T>
inline void read_shader_resource(const spirv_cross::Compiler &compiler,
                                 VkShaderStageFlagBits        stage,
                                 std::vector<ShaderResource> &resources,
                                 const ShaderVariant &        variant)
{
	LOGE("Not implemented! Read shader resources of type.");
}

template <spv::Decoration T>
inline void read_resource_decoration(const spirv_cross::Compiler & /*compiler*/,
                                     const spirv_cross::Resource & /*resource*/,
                                     ShaderResource & /*shader_resource*/,
                                     const ShaderVariant & /* variant */)
{
	LOGE("Not implemented! Read resources decoration of type.");
}

template <>
inline void read_resource_decoration<spv::DecorationLocation>(const spirv_cross::Compiler &compiler,
                                                              const spirv_cross::Resource &resource,
                                                              ShaderResource &             shader_resource,
                                                              const ShaderVariant &        variant)
{
	shader_resource.location = compiler.get_decoration(resource.id, spv::DecorationLocation);
}

template <>
inline void read_resource_decoration<spv::DecorationDescriptorSet>(const spirv_cross::Compiler &compiler,
                                                                   const spirv_cross::Resource &resource,
                                                                   ShaderResource &             shader_resource,
                                                                   const ShaderVariant &        variant)
{
	shader_resource.set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
}

template <>
inline void read_resource_decoration<spv::DecorationBinding>(const spirv_cross::Compiler &compiler,
                                                             const spirv_cross::Resource &resource,
                                                             ShaderResource &             shader_resource,
                                                             const ShaderVariant &        variant)
{
	shader_resource.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
}

template <>
inline void read_resource_decoration<spv::DecorationInputAttachmentIndex>(const spirv_cross::Compiler &compiler,
                                                                          const spirv_cross::Resource &resource,
                                                                          ShaderResource &             shader_resource,
                                                                          const ShaderVariant &        variant)
{
	shader_resource.input_attachment_index = compiler.get_decoration(resource.id, spv::DecorationInputAttachmentIndex);
}

template <>
inline void read_resource_decoration<spv::DecorationNonWritable>(const spirv_cross::Compiler &compiler,
                                                                 const spirv_cross::Resource &resource,
                                                                 ShaderResource &             shader_resource,
                                                                 const ShaderVariant &        variant)
{
	shader_resource.qualifiers |= ShaderResourceQualifiers::NonWritable;
}

template <>
inline void read_resource_decoration<spv::DecorationNonReadable>(const spirv_cross::Compiler &compiler,
                                                                 const spirv_cross::Resource &resource,
                                                                 ShaderResource &             shader_resource,
                                                                 const ShaderVariant &        variant)
{
	shader_resource.qualifiers |= ShaderResourceQualifiers::NonReadable;
}

inline void read_resource_vec_size(const spirv_cross::Compiler &compiler,
                                   const spirv_cross::Resource &resource,
                                   ShaderResource &             shader_resource,
                                   const ShaderVariant &        variant)
{
	const auto &spirv_type = compiler.get_type_from_variable(resource.id);

	shader_resource.vec_size = spirv_type.vecsize;
	shader_resource.columns  = spirv_type.columns;
}

inline void read_resource_array_size(const spirv_cross::Compiler &compiler,
                                     const spirv_cross::Resource &resource,
                                     ShaderResource &             shader_resource,
                                     const ShaderVariant &        variant)
{
	const auto &spirv_type = compiler.get_type_from_variable(resource.id);

	shader_resource.array_size = spirv_type.array.size() ? spirv_type.array[0] : 1;
}

inline void read_resource_size(const spirv_cross::Compiler &compiler,
                               const spirv_cross::Resource &resource,
                               ShaderResource &             shader_resource,
                               const ShaderVariant &        variant)
{
	const auto &spirv_type = compiler.get_type_from_variable(resource.id);

	size_t array_size = 0;
	if (variant.get_runtime_array_sizes().count(resource.name) != 0)
	{
		array_size = variant.get_runtime_array_sizes().at(resource.name);
	}

	shader_resource.size = to_u32(compiler.get_declared_struct_size_runtime_array(spirv_type, array_size));
}

inline void read_resource_size(const spirv_cross::Compiler &    compiler,
                               const spirv_cross::SPIRConstant &constant,
                               ShaderResource &                 shader_resource,
                               const ShaderVariant &            variant)
{
	auto spirv_type = compiler.get_type(constant.constant_type);

	switch (spirv_type.basetype)
	{
		case spirv_cross::SPIRType::BaseType::Boolean:
		case spirv_cross::SPIRType::BaseType::Char:
		case spirv_cross::SPIRType::BaseType::Int:
		case spirv_cross::SPIRType::BaseType::UInt:
		case spirv_cross::SPIRType::BaseType::Float:
			shader_resource.size = 4;
			break;
		case spirv_cross::SPIRType::BaseType::Int64:
		case spirv_cross::SPIRType::BaseType::UInt64:
		case spirv_cross::SPIRType::BaseType::Double:
			shader_resource.size = 8;
			break;
		default:
			shader_resource.size = 0;
			break;
	}
}

template <>
inline void read_shader_resource<ShaderResourceType::Input>(const spirv_cross::Compiler &compiler,
                                                            VkShaderStageFlagBits        stage,
                                                            std::vector<ShaderResource> &resources,
                                                            const ShaderVariant &        variant)
{
	auto input_resources = compiler.get_shader_resources().stage_inputs;

	for (auto &resource : input_resources)
	{
		ShaderResource shader_resource{};
		shader_resource.type   = ShaderResourceType::Input;
		shader_resource.stages = stage;
		shader_resource.name   = resource.name;

		read_resource_vec_size(compiler, resource, shader_resource, variant);
		read_resource_array_size(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationLocation>(compiler, resource, shader_resource, variant);

		resources.push_back(shader_resource);
	}
}

template <>
inline void read_shader_resource<ShaderResourceType::InputAttachment>(const spirv_cross::Compiler &compiler,
                                                                      VkShaderStageFlagBits /*stage*/,
                                                                      std::vector<ShaderResource> &resources,
                                                                      const ShaderVariant &        variant)
{
	auto subpass_resources = compiler.get_shader_resources().subpass_inputs;

	for (auto &resource : subpass_resources)
	{
		ShaderResource shader_resource{};
		shader_resource.type   = ShaderResourceType::InputAttachment;
		shader_resource.stages = VK_SHADER_STAGE_FRAGMENT_BIT;
		shader_resource.name   = resource.name;

		read_resource_array_size(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationInputAttachmentIndex>(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationDescriptorSet>(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationBinding>(compiler, resource, shader_resource, variant);

		resources.push_back(shader_resource);
	}
}

template <>
inline void read_shader_resource<ShaderResourceType::Output>(const spirv_cross::Compiler &compiler,
                                                             VkShaderStageFlagBits        stage,
                                                             std::vector<ShaderResource> &resources,
                                                             const ShaderVariant &        variant)
{
	auto output_resources = compiler.get_shader_resources().stage_outputs;

	for (auto &resource : output_resources)
	{
		ShaderResource shader_resource{};
		shader_resource.type   = ShaderResourceType::Output;
		shader_resource.stages = stage;
		shader_resource.name   = resource.name;

		read_resource_array_size(compiler, resource, shader_resource, variant);
		read_resource_vec_size(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationLocation>(compiler, resource, shader_resource, variant);

		resources.push_back(shader_resource);
	}
}

template <>
inline void read_shader_resource<ShaderResourceType::Image>(const spirv_cross::Compiler &compiler,
                                                            VkShaderStageFlagBits        stage,
                                                            std::vector<ShaderResource> &resources,
                                                            const ShaderVariant &        variant)
{
	auto image_resources = compiler.get_shader_resources().separate_images;

	for (auto &resource : image_resources)
	{
		ShaderResource shader_resource{};
		shader_resource.type   = ShaderResourceType::Image;
		shader_resource.stages = stage;
		shader_resource.name   = resource.name;

		read_resource_array_size(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationDescriptorSet>(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationBinding>(compiler, resource, shader_resource, variant);

		resources.push_back(shader_resource);
	}
}

template <>
inline void read_shader_resource<ShaderResourceType::ImageSampler>(const spirv_cross::Compiler &compiler,
                                                                   VkShaderStageFlagBits        stage,
                                                                   std::vector<ShaderResource> &resources,
                                                                   const ShaderVariant &        variant)
{
	auto image_resources = compiler.get_shader_resources().sampled_images;

	for (auto &resource : image_resources)
	{
		ShaderResource shader_resource{};
		shader_resource.type   = ShaderResourceType::ImageSampler;
		shader_resource.stages = stage;
		shader_resource.name   = resource.name;

		read_resource_array_size(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationDescriptorSet>(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationBinding>(compiler, resource, shader_resource, variant);

		resources.push_back(shader_resource);
	}
}

template <>
inline void read_shader_resource<ShaderResourceType::ImageStorage>(const spirv_cross::Compiler &compiler,
                                                                   VkShaderStageFlagBits        stage,
                                                                   std::vector<ShaderResource> &resources,
                                                                   const ShaderVariant &        variant)
{
	auto storage_resources = compiler.get_shader_resources().storage_images;

	for (auto &resource : storage_resources)
	{
		ShaderResource shader_resource{};
		shader_resource.type   = ShaderResourceType::ImageStorage;
		shader_resource.stages = stage;
		shader_resource.name   = resource.name;

		read_resource_array_size(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationNonReadable>(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationNonWritable>(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationDescriptorSet>(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationBinding>(compiler, resource, shader_resource, variant);

		resources.push_back(shader_resource);
	}
}

template <>
inline void read_shader_resource<ShaderResourceType::Sampler>(const spirv_cross::Compiler &compiler,
                                                              VkShaderStageFlagBits        stage,
                                                              std::vector<ShaderResource> &resources,
                                                              const ShaderVariant &        variant)
{
	auto sampler_resources = compiler.get_shader_resources().separate_samplers;

	for (auto &resource : sampler_resources)
	{
		ShaderResource shader_resource{};
		shader_resource.type   = ShaderResourceType::Sampler;
		shader_resource.stages = stage;
		shader_resource.name   = resource.name;

		read_resource_array_size(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationDescriptorSet>(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationBinding>(compiler, resource, shader_resource, variant);

		resources.push_back(shader_resource);
	}
}

template <>
inline void read_shader_resource<ShaderResourceType::BufferUniform>(const spirv_cross::Compiler &compiler,
                                                                    VkShaderStageFlagBits        stage,
                                                                    std::vector<ShaderResource> &resources,
                                                                    const ShaderVariant &        variant)
{
	auto uniform_resources = compiler.get_shader_resources().uniform_buffers;

	for (auto &resource : uniform_resources)
	{
		ShaderResource shader_resource{};
		shader_resource.type   = ShaderResourceType::BufferUniform;
		shader_resource.stages = stage;
		shader_resource.name   = resource.name;

		read_resource_size(compiler, resource, shader_resource, variant);
		read_resource_array_size(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationDescriptorSet>(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationBinding>(compiler, resource, shader_resource, variant);

		resources.push_back(shader_resource);
	}
}

template <>
inline void read_shader_resource<ShaderResourceType::BufferStorage>(const spirv_cross::Compiler &compiler,
                                                                    VkShaderStageFlagBits        stage,
                                                                    std::vector<ShaderResource> &resources,
                                                                    const ShaderVariant &        variant)
{
	auto storage_resources = compiler.get_shader_resources().storage_buffers;

	for (auto &resource : storage_resources)
	{
		ShaderResource shader_resource;
		shader_resource.type   = ShaderResourceType::BufferStorage;
		shader_resource.stages = stage;
		shader_resource.name   = resource.name;

		read_resource_size(compiler, resource, shader_resource, variant);
		read_resource_array_size(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationNonReadable>(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationNonWritable>(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationDescriptorSet>(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationBinding>(compiler, resource, shader_resource, variant);

		resources.push_back(shader_resource);
	}
}
}        // namespace

bool SPIRVReflection::reflect_shader_resources(VkShaderStageFlagBits stage, const std::vector<uint32_t> &spirv, std::vector<ShaderResource> &resources, const ShaderVariant &variant)
{
	spirv_cross::CompilerGLSL compiler{spirv};

	auto opts                     = compiler.get_common_options();
	opts.enable_420pack_extension = true;

	compiler.set_common_options(opts);

	parse_shader_resources(compiler, stage, resources, variant);
	parse_push_constants(compiler, stage, resources, variant);
	parse_specialization_constants(compiler, stage, resources, variant);

	return true;
}

void SPIRVReflection::parse_shader_resources(const spirv_cross::Compiler &compiler, VkShaderStageFlagBits stage, std::vector<ShaderResource> &resources, const ShaderVariant &variant)
{
	read_shader_resource<ShaderResourceType::Input>(compiler, stage, resources, variant);
	read_shader_resource<ShaderResourceType::InputAttachment>(compiler, stage, resources, variant);
	read_shader_resource<ShaderResourceType::Output>(compiler, stage, resources, variant);
	read_shader_resource<ShaderResourceType::Image>(compiler, stage, resources, variant);
	read_shader_resource<ShaderResourceType::ImageSampler>(compiler, stage, resources, variant);
	read_shader_resource<ShaderResourceType::ImageStorage>(compiler, stage, resources, variant);
	read_shader_resource<ShaderResourceType::Sampler>(compiler, stage, resources, variant);
	read_shader_resource<ShaderResourceType::BufferUniform>(compiler, stage, resources, variant);
	read_shader_resource<ShaderResourceType::BufferStorage>(compiler, stage, resources, variant);
}

void SPIRVReflection::parse_push_constants(const spirv_cross::Compiler &compiler, VkShaderStageFlagBits stage, std::vector<ShaderResource> &resources, const ShaderVariant &variant)
{
	auto shader_resources = compiler.get_shader_resources();

	for (auto &resource : shader_resources.push_constant_buffers)
	{
		const auto &spivr_type = compiler.get_type_from_variable(resource.id);

		std::uint32_t offset = std::numeric_limits<std::uint32_t>::max();

		for (auto i = 0U; i < spivr_type.member_types.size(); ++i)
		{
			auto mem_offset = compiler.get_member_decoration(spivr_type.self, i, spv::DecorationOffset);

			offset = std::min(offset, mem_offset);
		}

		ShaderResource shader_resource{};
		shader_resource.type   = ShaderResourceType::PushConstant;
		shader_resource.stages = stage;
		shader_resource.name   = resource.name;
		shader_resource.offset = offset;

		read_resource_size(compiler, resource, shader_resource, variant);

		shader_resource.size -= shader_resource.offset;

		resources.push_back(shader_resource);
	}
}

void SPIRVReflection::parse_specialization_constants(const spirv_cross::Compiler &compiler, VkShaderStageFlagBits stage, std::vector<ShaderResource> &resources, const ShaderVariant &variant)
{
	auto specialization_constants = compiler.get_specialization_constants();

	for (auto &resource : specialization_constants)
	{
		auto &spirv_value = compiler.get_constant(resource.id);

		ShaderResource shader_resource{};
		shader_resource.type        = ShaderResourceType::SpecializationConstant;
		shader_resource.stages      = stage;
		shader_resource.name        = compiler.get_name(resource.id);
		shader_resource.offset      = 0;
		shader_resource.constant_id = resource.constant_id;

		read_resource_size(compiler, spirv_value, shader_resource, variant);

		resources.push_back(shader_resource);
	}
}
}        // namespace vkb
