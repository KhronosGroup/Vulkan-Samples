/* Copyright (c) 2019-2021, Arm Limited and Contributors
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

#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <volk.h>

namespace components
{
namespace vulkan
{
// Types of shader resources
enum class ShaderResourceType
{
	Input,
	Uniform,
	Output,
	PushConstant,
	Unknown
};

enum class ShaderRawDataType
{
	Void,
	Bool,
	Int,
	Float,
	Vector,
	Matrix,
	Image,
	Sampler,
	SampledImage,
	Array,
	RuntimeArray,
	Struct,
	Unknown
};

struct RawShaderData
{
	ShaderRawDataType type{ShaderRawDataType::Unknown};
	std::string       struct_member_name;
	std::string       type_name;

	static std::unique_ptr<RawShaderData> create_unknown()
	{
		return std::make_unique<RawShaderData>();
	}
};

struct NumericShaderData : RawShaderData
{
	uint32_t width{0};
	bool     is_signed{false};
};

struct VectorShaderData : RawShaderData
{
	uint32_t component_count{0};
	uint32_t component_width{0};
	bool     is_signed{false};
};

struct MatrixShaderData : RawShaderData
{
	uint32_t columns{0};
	uint32_t rows{0};
};

struct ImageShaderData : RawShaderData
{
	VkFormat    format{VK_FORMAT_UNDEFINED};
	VkImageType type{VK_IMAGE_TYPE_MAX_ENUM};
};

struct ArrayShaderData : RawShaderData
{
	std::unique_ptr<RawShaderData>              element_type;
	uint32_t                                    element_count{0};
	std::vector<std::unique_ptr<RawShaderData>> members;
};

struct StructShaderData : RawShaderData
{
	std::vector<std::unique_ptr<RawShaderData>> members;
};

// A bitmask of qualifiers applied to a resource
enum ShaderResourceQualifiers : uint32_t
{
	Read      = 1,
	Write     = 2,
	ReadWrite = Read | Write,
};

struct ResourceLookup
{
	uint32_t set{UINT32_MAX};
	uint32_t binding{UINT32_MAX};
	uint32_t location{UINT32_MAX};
	uint32_t input_attachment_index{UINT32_MAX};
	uint32_t push_constant_offset{UINT32_MAX};

	bool has_set_and_binding() const noexcept
	{
		return set != UINT32_MAX && binding != UINT32_MAX;
	}

	bool has_location() const noexcept
	{
		return location != UINT32_MAX;
	}

	bool has_input_attachment_index() const noexcept
	{
		return input_attachment_index != UINT32_MAX;
	}

	bool has_push_constant_offset() const noexcept
	{
		return push_constant_offset != UINT32_MAX;
	}

	bool is_valid() const noexcept
	{
		return has_set_and_binding() || has_location() || (has_input_attachment_index() && has_set_and_binding()) || has_push_constant_offset();
	}
};

struct ShaderResource
{
	std::string                    name;
	ShaderResourceType             type{ShaderResourceType::Unknown};
	ShaderResourceQualifiers       qualifiers{ShaderResourceQualifiers::ReadWrite};
	std::unique_ptr<RawShaderData> data;
	ResourceLookup                 lookup;
};

// A set of shader resources
struct ShaderResources
{
	std::vector<ShaderResource> resources;
};
}        // namespace vulkan
}        // namespace components