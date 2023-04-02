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

#include <deque>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <volk.h>

namespace components
{
namespace shaders
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

	static std::shared_ptr<RawShaderData> create_unknown()
	{
		return std::make_shared<RawShaderData>();
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
	std::shared_ptr<RawShaderData> element_type;
	uint32_t                       element_count{0};
};

struct StructShaderData : RawShaderData
{
	std::vector<std::shared_ptr<RawShaderData>> members;
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
	std::shared_ptr<RawShaderData> data;
	ResourceLookup                 lookup;
};

// A set of shader resources
class ShaderResources
{
  public:
	ShaderResources()
	{}

	ShaderResources(const std::vector<ShaderResource> &resources) :
	    resources(resources)
	{
		process_resources();
	}

	static ShaderResources merge(const ShaderResources &a, const ShaderResources &b)
	{
		ShaderResources copy = a;
		copy.merge(b.resources);
		return copy;
	}

	std::vector<StructShaderData> structs_in_topological_order() const
	{
		std::vector<std::pair<std::string, StructShaderData>> out_structs;

		for (auto &[name, data] : structs)
		{
			std::vector<std::string> dependencies;

			for (auto &member : data.members)
			{
				if (member->type == ShaderRawDataType::Struct)
				{
					auto struct_member = std::static_pointer_cast<StructShaderData>(member);
					dependencies.push_back(struct_member->type_name);
				}
			}

			size_t insert_index = 0;

			for (size_t i = 0; i < out_structs.size(); i++)
			{
				if (out_structs[i].first == name)
				{
					insert_index = i + 1;
					break;
				}
			}

			out_structs.insert(out_structs.begin() + insert_index, std::pair<std::string, StructShaderData>{name, data});
		}

		std::vector<StructShaderData> out_structs_final;

		for (auto &[name, data] : out_structs)
		{
			out_structs_final.push_back(data);
		}

		return out_structs_final;
	}

  private:
	std::vector<ShaderResource> resources;

	void merge(const std::vector<ShaderResource> &new_resources) noexcept
	{
		for (auto &new_resource : new_resources)
		{
			bool found{false};

			for (auto &resource : resources)
			{
				if (resource.name == new_resource.name)
				{
					found = true;
					break;
				}
			}

			if (!found)
			{
				resources.push_back(new_resource);
			}
		}

		process_resources();
	}

	std::unordered_map<std::string, StructShaderData> structs;

	void process_resources()
	{
		extract_unique_structs();
	}

	void extract_unique_structs()
	{
		structs.clear();

		std::deque<std::shared_ptr<StructShaderData>> structs_to_process;

		for (auto &resource : resources)
		{
			if (resource.data->type == ShaderRawDataType::Struct)
			{
				auto struct_data = std::static_pointer_cast<StructShaderData>(resource.data);
				structs_to_process.push_back(struct_data);
			}
		}

		while (!structs_to_process.empty())
		{
			auto struct_data = structs_to_process.front();
			structs_to_process.pop_front();

			if (structs.find(struct_data->type_name) == structs.end())
			{
				structs[struct_data->type_name] = *struct_data;

				for (auto &member : struct_data->members)
				{
					if (member->type == ShaderRawDataType::Struct)
					{
						if (auto struct_member = std::static_pointer_cast<StructShaderData>(member))
						{
							structs_to_process.push_back(struct_member);
						}
					}

					if (member->type == ShaderRawDataType::Array)
					{
						if (auto array_member = std::static_pointer_cast<ArrayShaderData>(member))
						{
							if (array_member->element_type->type == ShaderRawDataType::Struct)
							{
								if (auto struct_member = std::static_pointer_cast<StructShaderData>(array_member->element_type))
								{
									structs_to_process.push_back(struct_member);
								}
							}
						}
					}
				}
			}
		}
	}

	// std::vector<ShaderResource> filter(ShaderResourceType type) const
	// {
	// 	std::vector<ShaderResource> resources_of_type;

	// 	for (auto &resource : resources)
	// 	{
	// 		if (resource.type == type)
	// 		{
	// 			resources_of_type.push_back(resource);
	// 		}
	// 	}

	// 	return resources_of_type;
	// }
};
}        // namespace shaders
}        // namespace components