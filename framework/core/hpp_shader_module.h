/* Copyright (c) 2023, NVIDIA CORPORATION. All rights reserved.
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

#include "core/shader_module.h"
#include <vulkan/vulkan.hpp>

namespace vkb
{
namespace core
{
/**
 * @brief facade class around vkb::ShaderModule, providing a vulkan.hpp-based interface
 *
 * See vkb::ShaderModule for documentation
 */

/// This determines the type and method of how descriptor set should be created and bound
enum class HPPShaderResourceMode
{
	Static,
	Dynamic,
	UpdateAfterBind
};

/// Types of shader resources
enum class HPPShaderResourceType
{
	Input,
	InputAttachment,
	Output,
	Image,
	ImageSampler,
	ImageStorage,
	Sampler,
	BufferUniform,
	BufferStorage,
	PushConstant,
	SpecializationConstant,
	All
};

/// Store shader resource data.
/// Used by the shader module.
struct HPPShaderResource
{
	vk::ShaderStageFlags  stages;
	HPPShaderResourceType type;
	HPPShaderResourceMode mode;
	uint32_t              set;
	uint32_t              binding;
	uint32_t              location;
	uint32_t              input_attachment_index;
	uint32_t              vec_size;
	uint32_t              columns;
	uint32_t              array_size;
	uint32_t              offset;
	uint32_t              size;
	uint32_t              constant_id;
	uint32_t              qualifiers;
	std::string           name;
};

}        // namespace core
}        // namespace vkb
