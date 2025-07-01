/* Copyright (c) 2019-2025, Arm Limited and Contributors
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

#include "shader_module.h"

#include "core/util/logging.hpp"
#include "device.h"
#include "filesystem/legacy.h"
#include "spirv_reflection.h"

namespace vkb
{
ShaderModule::ShaderModule(vkb::core::DeviceC   &device,
                           VkShaderStageFlagBits stage,
                           const ShaderSource   &shader_source,
                           const std::string    &entry_point,
                           const ShaderVariant  &shader_variant) :
    device{device}, stage{stage}, entry_point{entry_point}
{
	debug_name = fmt::format("{} [variant {:X}] [entrypoint {}]", shader_source.get_filename(), shader_variant.get_id(), entry_point);

	// Shaders in binary SPIR-V format can be loaded directly
	spirv = vkb::fs::read_shader_binary_u32(shader_source.get_filename());

	// Reflection is used to dynamically create descriptor bindings

	SPIRVReflection spirv_reflection;
	// Reflect all shader resources
	if (!spirv_reflection.reflect_shader_resources(stage, spirv, resources, shader_variant))
	{
		throw VulkanException{VK_ERROR_INITIALIZATION_FAILED};
	}

	// Generate a unique id, determined by source and variant
	std::hash<std::string> hasher{};
	id = hasher(std::string{reinterpret_cast<const char *>(spirv.data()),
	                        reinterpret_cast<const char *>(spirv.data() + spirv.size())});
}

ShaderModule::ShaderModule(ShaderModule &&other) :
    device{other.device},
    id{other.id},
    stage{other.stage},
    entry_point{other.entry_point},
    debug_name{other.debug_name},
    spirv{other.spirv},
    resources{other.resources}
{
	other.stage = {};
}

size_t ShaderModule::get_id() const
{
	return id;
}

VkShaderStageFlagBits ShaderModule::get_stage() const
{
	return stage;
}

const std::string &ShaderModule::get_entry_point() const
{
	return entry_point;
}

const std::vector<ShaderResource> &ShaderModule::get_resources() const
{
	return resources;
}

const std::vector<uint32_t> &ShaderModule::get_binary() const
{
	return spirv;
}

void ShaderModule::set_resource_mode(const std::string &resource_name, const ShaderResourceMode &resource_mode)
{
	auto it = std::ranges::find_if(resources, [&resource_name](const ShaderResource &resource) { return resource.name == resource_name; });

	if (it != resources.end())
	{
		if (resource_mode == ShaderResourceMode::Dynamic)
		{
			if (it->type == ShaderResourceType::BufferUniform || it->type == ShaderResourceType::BufferStorage)
			{
				it->mode = resource_mode;
			}
			else
			{
				LOGW("Resource `{}` does not support dynamic.", resource_name);
			}
		}
		else
		{
			it->mode = resource_mode;
		}
	}
	else
	{
		LOGW("Resource `{}` not found for shader.", resource_name);
	}
}

size_t ShaderVariant::get_id() const
{
	return id;
}

void ShaderVariant::add_runtime_array_size(const std::string &runtime_array_name, size_t size)
{
	if (runtime_array_sizes.find(runtime_array_name) == runtime_array_sizes.end())
	{
		runtime_array_sizes.insert({runtime_array_name, size});
	}
	else
	{
		runtime_array_sizes[runtime_array_name] = size;
	}
}

void ShaderVariant::set_runtime_array_sizes(const std::unordered_map<std::string, size_t> &sizes)
{
	this->runtime_array_sizes = sizes;
}

const std::unordered_map<std::string, size_t> &ShaderVariant::get_runtime_array_sizes() const
{
	return runtime_array_sizes;
}

void ShaderVariant::clear()
{
	runtime_array_sizes.clear();
	id = 0;
}

ShaderSource::ShaderSource(const std::string &filename) :
    filename{filename},
    source{fs::read_text_file(filename)}
{
	std::hash<std::string> hasher{};
	id = hasher(std::string{this->source.cbegin(), this->source.cend()});
}

size_t ShaderSource::get_id() const
{
	return id;
}

const std::string &ShaderSource::get_filename() const
{
	return filename;
}

void ShaderSource::set_source(const std::string &source_)
{
	source = source_;
	std::hash<std::string> hasher{};
	id = hasher(std::string{this->source.cbegin(), this->source.cend()});
}

const std::string &ShaderSource::get_source() const
{
	return source;
}
}        // namespace vkb
