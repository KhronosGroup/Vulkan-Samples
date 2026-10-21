/* Copyright (c) 2019-2026, Arm Limited and Contributors
 * Copyright (c) 2026, NVIDIA CORPORATION. All rights reserved.
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

#pragma once

#include "common/helpers.h"
#include "core/shader_module.h"

#pragma warning(push)
#pragma warning(disable : 4065)
#include <spirv_glsl.hpp>
#pragma warning(pop)

namespace vkb
{
/// Generate a list of shader resource based on SPIRV reflection code, and provided ShaderVariant
template <vkb::BindingType bindingType>
class SPIRVReflection
{
  public:
	using ShaderStageFlagBitsType = typename std::conditional<bindingType == BindingType::Cpp, vk::ShaderStageFlagBits, VkShaderStageFlagBits>::type;

  public:
	/// @brief Reflects shader resources from SPIRV code
	/// @param stage The Vulkan shader stage flag
	/// @param spirv The SPIRV code of shader
	/// @param[out] resources The list of reflected shader resources
	/// @param variant ShaderVariant used for reflection to specify the size of the runtime arrays in Storage Buffers
	bool reflect_shader_resources(ShaderStageFlagBitsType                              stage,
	                              std::vector<uint32_t> const                         &spirv,
	                              std::vector<vkb::core::ShaderResource<bindingType>> &resources,
	                              vkb::core::ShaderVariant const                      &variant);

  private:
	void parse_push_constants(spirv_cross::Compiler const                         &compiler,
	                          ShaderStageFlagBitsType                              stage,
	                          std::vector<vkb::core::ShaderResource<bindingType>> &resources,
	                          vkb::core::ShaderVariant const                      &variant);
	void parse_shader_resources(spirv_cross::Compiler const                         &compiler,
	                            ShaderStageFlagBitsType                              stage,
	                            std::vector<vkb::core::ShaderResource<bindingType>> &resources,
	                            vkb::core::ShaderVariant const                      &variant);
	void parse_specialization_constants(spirv_cross::Compiler const                         &compiler,
	                                    ShaderStageFlagBitsType                              stage,
	                                    std::vector<vkb::core::ShaderResource<bindingType>> &resources,
	                                    vkb::core::ShaderVariant const                      &variant);
};

using SPIRVReflectionC   = SPIRVReflection<vkb::BindingType::C>;
using SPIRVReflectionCpp = SPIRVReflection<vkb::BindingType::Cpp>;

namespace
{
inline void read_resource_size(const spirv_cross::Compiler    &compiler,
                               const spirv_cross::Resource    &resource,
                               vkb::core::ShaderResourceCpp   &shader_resource,
                               const vkb::core::ShaderVariant &variant)
{
	const auto &spirv_type = compiler.get_type_from_variable(resource.id);

	auto const &array_sizes = variant.get_runtime_array_sizes();
	auto        it          = array_sizes.find(resource.name);
	size_t      array_size  = (it == array_sizes.end()) ? 0 : it->second;

	shader_resource.size = to_u32(compiler.get_declared_struct_size_runtime_array(spirv_type, array_size));
}

template <spv::Decoration T>
inline void read_resource_decoration([[maybe_unused]] spirv_cross::Compiler const    &compiler,
                                     [[maybe_unused]] spirv_cross::Resource const    &resource,
                                     [[maybe_unused]] vkb::core::ShaderResourceCpp   &shader_resource,
                                     [[maybe_unused]] vkb::core::ShaderVariant const &variant)
{
	LOGE("Not implemented! Read resources decoration of type.");
}

template <>
inline void read_resource_decoration<spv::DecorationBinding>(spirv_cross::Compiler const                     &compiler,
                                                             spirv_cross::Resource const                     &resource,
                                                             vkb::core::ShaderResourceCpp                    &shader_resource,
                                                             [[maybe_unused]] vkb::core::ShaderVariant const &variant)
{
	shader_resource.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
}

template <>
inline void read_resource_decoration<spv::DecorationDescriptorSet>(spirv_cross::Compiler const                     &compiler,
                                                                   spirv_cross::Resource const                     &resource,
                                                                   vkb::core::ShaderResourceCpp                    &shader_resource,
                                                                   [[maybe_unused]] vkb::core::ShaderVariant const &variant)
{
	shader_resource.set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
}

template <>
inline void read_resource_decoration<spv::DecorationInputAttachmentIndex>(spirv_cross::Compiler const                     &compiler,
                                                                          spirv_cross::Resource const                     &resource,
                                                                          vkb::core::ShaderResourceCpp                    &shader_resource,
                                                                          [[maybe_unused]] vkb::core::ShaderVariant const &variant)
{
	shader_resource.input_attachment_index = compiler.get_decoration(resource.id, spv::DecorationInputAttachmentIndex);
}

template <>
inline void read_resource_decoration<spv::DecorationLocation>(spirv_cross::Compiler const                     &compiler,
                                                              spirv_cross::Resource const                     &resource,
                                                              vkb::core::ShaderResourceCpp                    &shader_resource,
                                                              [[maybe_unused]] vkb::core::ShaderVariant const &variant)
{
	shader_resource.location = compiler.get_decoration(resource.id, spv::DecorationLocation);
}

template <>
inline void read_resource_decoration<spv::DecorationNonReadable>([[maybe_unused]] spirv_cross::Compiler const    &compiler,
                                                                 [[maybe_unused]] spirv_cross::Resource const    &resource,
                                                                 vkb::core::ShaderResourceCpp                    &shader_resource,
                                                                 [[maybe_unused]] vkb::core::ShaderVariant const &variant)
{
	shader_resource.qualifiers |= vkb::core::ShaderResourceQualifierFlagBits::NonReadable;
}

template <>
inline void read_resource_decoration<spv::DecorationNonWritable>([[maybe_unused]] spirv_cross::Compiler const    &compiler,
                                                                 [[maybe_unused]] spirv_cross::Resource const    &resource,
                                                                 vkb::core::ShaderResourceCpp                    &shader_resource,
                                                                 [[maybe_unused]] vkb::core::ShaderVariant const &variant)
{
	shader_resource.qualifiers |= vkb::core::ShaderResourceQualifierFlagBits::NonWritable;
}

inline void read_resource_array_size(spirv_cross::Compiler const                     &compiler,
                                     spirv_cross::Resource const                     &resource,
                                     vkb::core::ShaderResourceCpp                    &shader_resource,
                                     [[maybe_unused]] vkb::core::ShaderVariant const &variant)
{
	const auto &spirv_type = compiler.get_type_from_variable(resource.id);

	shader_resource.array_size = spirv_type.array.size() ? spirv_type.array[0] : 1;
}

inline void read_resource_size(spirv_cross::Compiler const                     &compiler,
                               spirv_cross::SPIRConstant const                 &constant,
                               vkb::core::ShaderResourceCpp                    &shader_resource,
                               [[maybe_unused]] vkb::core::ShaderVariant const &variant)
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

inline void read_resource_vec_size(spirv_cross::Compiler const                     &compiler,
                                   spirv_cross::Resource const                     &resource,
                                   vkb::core::ShaderResourceCpp                    &shader_resource,
                                   [[maybe_unused]] vkb::core::ShaderVariant const &variant)
{
	const auto &spirv_type = compiler.get_type_from_variable(resource.id);

	shader_resource.vec_size = spirv_type.vecsize;
	shader_resource.columns  = spirv_type.columns;
}

template <vkb::core::ShaderResourceType T>
inline void read_shader_resource([[maybe_unused]] spirv_cross::Compiler const               &compiler,
                                 [[maybe_unused]] vk::ShaderStageFlagBits                    stage,
                                 [[maybe_unused]] std::vector<vkb::core::ShaderResourceCpp> &resources,
                                 [[maybe_unused]] vkb::core::ShaderVariant const            &variant)
{
	LOGE("Not implemented! Read shader resources of type.");
}

template <>
inline void read_shader_resource<vkb::core::ShaderResourceType::Input>(spirv_cross::Compiler const               &compiler,
                                                                       vk::ShaderStageFlagBits                    stage,
                                                                       std::vector<vkb::core::ShaderResourceCpp> &resources,
                                                                       vkb::core::ShaderVariant const            &variant)
{
	auto input_resources = compiler.get_shader_resources().stage_inputs;

	for (auto &resource : input_resources)
	{
		vkb::core::ShaderResourceCpp shader_resource{.stages = stage, .type = vkb::core::ShaderResourceType::Input, .name = resource.name};

		read_resource_vec_size(compiler, resource, shader_resource, variant);
		read_resource_array_size(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationLocation>(compiler, resource, shader_resource, variant);

		resources.push_back(shader_resource);
	}
}

template <>
inline void read_shader_resource<vkb::core::ShaderResourceType::InputAttachment>(spirv_cross::Compiler const               &compiler,
                                                                                 [[maybe_unused]] vk::ShaderStageFlagBits   stage,
                                                                                 std::vector<vkb::core::ShaderResourceCpp> &resources,
                                                                                 vkb::core::ShaderVariant const            &variant)
{
	auto subpass_resources = compiler.get_shader_resources().subpass_inputs;

	for (auto &resource : subpass_resources)
	{
		vkb::core::ShaderResourceCpp shader_resource{.stages = vk::ShaderStageFlagBits::eFragment,
		                                             .type   = vkb::core::ShaderResourceType::InputAttachment,
		                                             .name   = resource.name};

		read_resource_array_size(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationInputAttachmentIndex>(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationDescriptorSet>(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationBinding>(compiler, resource, shader_resource, variant);

		resources.push_back(shader_resource);
	}
}

template <>
inline void read_shader_resource<vkb::core::ShaderResourceType::Output>(spirv_cross::Compiler const               &compiler,
                                                                        vk::ShaderStageFlagBits                    stage,
                                                                        std::vector<vkb::core::ShaderResourceCpp> &resources,
                                                                        vkb::core::ShaderVariant const            &variant)
{
	auto output_resources = compiler.get_shader_resources().stage_outputs;

	for (auto &resource : output_resources)
	{
		vkb::core::ShaderResourceCpp shader_resource{.stages = stage, .type = vkb::core::ShaderResourceType::Output, .name = resource.name};

		read_resource_array_size(compiler, resource, shader_resource, variant);
		read_resource_vec_size(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationLocation>(compiler, resource, shader_resource, variant);

		resources.push_back(shader_resource);
	}
}

template <>
inline void read_shader_resource<vkb::core::ShaderResourceType::Image>(spirv_cross::Compiler const               &compiler,
                                                                       vk::ShaderStageFlagBits                    stage,
                                                                       std::vector<vkb::core::ShaderResourceCpp> &resources,
                                                                       vkb::core::ShaderVariant const            &variant)
{
	auto image_resources = compiler.get_shader_resources().separate_images;

	for (auto &resource : image_resources)
	{
		vkb::core::ShaderResourceCpp shader_resource{.stages = stage, .type = vkb::core::ShaderResourceType::Image, .name = resource.name};

		read_resource_array_size(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationDescriptorSet>(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationBinding>(compiler, resource, shader_resource, variant);

		resources.push_back(shader_resource);
	}
}

template <>
inline void read_shader_resource<vkb::core::ShaderResourceType::ImageSampler>(spirv_cross::Compiler const               &compiler,
                                                                              vk::ShaderStageFlagBits                    stage,
                                                                              std::vector<vkb::core::ShaderResourceCpp> &resources,
                                                                              vkb::core::ShaderVariant const            &variant)
{
	auto image_resources = compiler.get_shader_resources().sampled_images;

	for (auto &resource : image_resources)
	{
		vkb::core::ShaderResourceCpp shader_resource{.stages = stage, .type = vkb::core::ShaderResourceType::ImageSampler, .name = resource.name};

		read_resource_array_size(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationDescriptorSet>(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationBinding>(compiler, resource, shader_resource, variant);

		resources.push_back(shader_resource);
	}
}

template <>
inline void read_shader_resource<vkb::core::ShaderResourceType::ImageStorage>(spirv_cross::Compiler const               &compiler,
                                                                              vk::ShaderStageFlagBits                    stage,
                                                                              std::vector<vkb::core::ShaderResourceCpp> &resources,
                                                                              vkb::core::ShaderVariant const            &variant)
{
	auto storage_resources = compiler.get_shader_resources().storage_images;

	for (auto &resource : storage_resources)
	{
		vkb::core::ShaderResourceCpp shader_resource{.stages = stage, .type = vkb::core::ShaderResourceType::ImageStorage, .name = resource.name};

		read_resource_array_size(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationNonReadable>(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationNonWritable>(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationDescriptorSet>(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationBinding>(compiler, resource, shader_resource, variant);

		resources.push_back(shader_resource);
	}
}

template <>
inline void read_shader_resource<vkb::core::ShaderResourceType::Sampler>(spirv_cross::Compiler const               &compiler,
                                                                         vk::ShaderStageFlagBits                    stage,
                                                                         std::vector<vkb::core::ShaderResourceCpp> &resources,
                                                                         vkb::core::ShaderVariant const            &variant)
{
	auto sampler_resources = compiler.get_shader_resources().separate_samplers;

	for (auto &resource : sampler_resources)
	{
		vkb::core::ShaderResourceCpp shader_resource{.stages = stage, .type = vkb::core::ShaderResourceType::Sampler, .name = resource.name};

		read_resource_array_size(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationDescriptorSet>(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationBinding>(compiler, resource, shader_resource, variant);

		resources.push_back(shader_resource);
	}
}

template <>
inline void read_shader_resource<vkb::core::ShaderResourceType::BufferUniform>(spirv_cross::Compiler const               &compiler,
                                                                               vk::ShaderStageFlagBits                    stage,
                                                                               std::vector<vkb::core::ShaderResourceCpp> &resources,
                                                                               vkb::core::ShaderVariant const            &variant)
{
	auto uniform_resources = compiler.get_shader_resources().uniform_buffers;

	for (auto &resource : uniform_resources)
	{
		vkb::core::ShaderResourceCpp shader_resource{.stages = stage, .type = vkb::core::ShaderResourceType::BufferUniform, .name = resource.name};

		read_resource_size(compiler, resource, shader_resource, variant);
		read_resource_array_size(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationDescriptorSet>(compiler, resource, shader_resource, variant);
		read_resource_decoration<spv::DecorationBinding>(compiler, resource, shader_resource, variant);

		resources.push_back(shader_resource);
	}
}

template <>
inline void read_shader_resource<vkb::core::ShaderResourceType::BufferStorage>(spirv_cross::Compiler const               &compiler,
                                                                               vk::ShaderStageFlagBits                    stage,
                                                                               std::vector<vkb::core::ShaderResourceCpp> &resources,
                                                                               vkb::core::ShaderVariant const            &variant)
{
	auto storage_resources = compiler.get_shader_resources().storage_buffers;

	for (auto &resource : storage_resources)
	{
		vkb::core::ShaderResourceCpp shader_resource{.stages = stage, .type = vkb::core::ShaderResourceType::BufferStorage, .name = resource.name};

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

template <vkb::BindingType bindingType>
inline bool SPIRVReflection<bindingType>::reflect_shader_resources(ShaderStageFlagBitsType                              stage,
                                                                   std::vector<uint32_t> const                         &spirv,
                                                                   std::vector<vkb::core::ShaderResource<bindingType>> &resources,
                                                                   vkb::core::ShaderVariant const                      &variant)
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

template <vkb::BindingType bindingType>
inline void SPIRVReflection<bindingType>::parse_push_constants(spirv_cross::Compiler const                         &compiler,
                                                               ShaderStageFlagBitsType                              stage,
                                                               std::vector<vkb::core::ShaderResource<bindingType>> &resources,
                                                               vkb::core::ShaderVariant const                      &variant)
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

		vkb::core::ShaderResourceCpp shader_resource{
		    .stages = stage, .type = vkb::core::ShaderResourceType::PushConstant, .offset = offset, .name = resource.name};

		read_resource_size(compiler, resource, shader_resource, variant);

		shader_resource.size -= shader_resource.offset;

		resources.push_back(shader_resource);
	}
}

template <vkb::BindingType bindingType>
inline void SPIRVReflection<bindingType>::parse_shader_resources(spirv_cross::Compiler const                         &compiler,
                                                                 ShaderStageFlagBitsType                              stage,
                                                                 std::vector<vkb::core::ShaderResource<bindingType>> &resources,
                                                                 vkb::core::ShaderVariant const                      &variant)
{
	read_shader_resource<vkb::core::ShaderResourceType::Input>(compiler, stage, resources, variant);
	read_shader_resource<vkb::core::ShaderResourceType::InputAttachment>(compiler, stage, resources, variant);
	read_shader_resource<vkb::core::ShaderResourceType::Output>(compiler, stage, resources, variant);
	read_shader_resource<vkb::core::ShaderResourceType::Image>(compiler, stage, resources, variant);
	read_shader_resource<vkb::core::ShaderResourceType::ImageSampler>(compiler, stage, resources, variant);
	read_shader_resource<vkb::core::ShaderResourceType::ImageStorage>(compiler, stage, resources, variant);
	read_shader_resource<vkb::core::ShaderResourceType::Sampler>(compiler, stage, resources, variant);
	read_shader_resource<vkb::core::ShaderResourceType::BufferUniform>(compiler, stage, resources, variant);
	read_shader_resource<vkb::core::ShaderResourceType::BufferStorage>(compiler, stage, resources, variant);
}

template <vkb::BindingType bindingType>
inline void SPIRVReflection<bindingType>::parse_specialization_constants(spirv_cross::Compiler const                         &compiler,
                                                                         ShaderStageFlagBitsType                              stage,
                                                                         std::vector<vkb::core::ShaderResource<bindingType>> &resources,
                                                                         const vkb::core::ShaderVariant                      &variant)
{
	auto specialization_constants = compiler.get_specialization_constants();

	for (auto &resource : specialization_constants)
	{
		auto &spirv_value = compiler.get_constant(resource.id);

		vkb::core::ShaderResourceCpp shader_resource{
		    .stages      = stage,
		    .type        = vkb::core::ShaderResourceType::SpecializationConstant,
		    .offset      = 0,
		    .constant_id = resource.constant_id,
		    .name        = compiler.get_name(resource.id),
		};

		read_resource_size(compiler, spirv_value, shader_resource, variant);

		resources.push_back(shader_resource);
	}
}
}        // namespace vkb
