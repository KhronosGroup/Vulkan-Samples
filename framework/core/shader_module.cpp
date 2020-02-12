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

#include "shader_module.h"

#include "common/logging.h"
#include "device.h"
#include "glsl_compiler.h"
#include "platform/filesystem.h"
#include "spirv_reflection.h"

namespace vkb
{
ShaderModule::ShaderModule(Device &device, VkShaderStageFlagBits stage, const ShaderSource &glsl_source, const std::string &entry_point, const ShaderVariant &shader_variant) :
    device{device},
    stage{stage},
    entry_point{entry_point}
{
	// Check if application is passing in GLSL source code to compile to SPIR-V
	if (glsl_source.get_data().empty())
	{
		throw VulkanException{VK_ERROR_INITIALIZATION_FAILED};
	}

	// Compiling from GLSL source requires the entry point
	if (entry_point.empty())
	{
		throw VulkanException{VK_ERROR_INITIALIZATION_FAILED};
	}

	GLSLCompiler glsl_compiler;

	// Compile the GLSL source
	if (!glsl_compiler.compile_to_spirv(stage, glsl_source.get_data(), entry_point, shader_variant, spirv, info_log))
	{
		LOGE("Shader compilation failed for shader \"{}\"", glsl_source.get_filename());
		LOGE("{}", info_log);
		throw VulkanException{VK_ERROR_INITIALIZATION_FAILED};
	}

	SPIRVReflection spirv_reflection;

	// Reflect all shader resouces
	if (!spirv_reflection.reflect_shader_resources(stage, spirv, resources, shader_variant))
	{
		throw VulkanException{VK_ERROR_INITIALIZATION_FAILED};
	}

	// Generate a unique id, determined by source and variant
	std::hash<std::string> hasher{};
	id = hasher(std::string{spirv.cbegin(), spirv.cend()});
}

ShaderModule::ShaderModule(ShaderModule &&other) :
    device{other.device},
    id{other.id},
    stage{other.stage},
    entry_point{other.entry_point},
    spirv{other.spirv},
    resources{other.resources},
    info_log{other.info_log}
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

const std::string &ShaderModule::get_info_log() const
{
	return info_log;
}

const std::vector<uint32_t> &ShaderModule::get_binary() const
{
	return spirv;
}

void ShaderModule::set_resource_mode(const std::string &resource_name, const ShaderResourceMode &resource_mode)
{
	auto it = std::find_if(resources.begin(), resources.end(), [&resource_name](const ShaderResource &resource) { return resource.name == resource_name; });

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

ShaderVariant::ShaderVariant(std::string &&preamble, std::vector<std::string> &&processes) :
    preamble{std::move(preamble)},
    processes{std::move(processes)}
{
	update_id();
}

size_t ShaderVariant::get_id() const
{
	return id;
}

void ShaderVariant::add_definitions(const std::vector<std::string> &definitions)
{
	for (auto &definition : definitions)
	{
		add_define(definition);
	}
}

void ShaderVariant::add_define(const std::string &def)
{
	processes.push_back("D" + def);

	std::string tmp_def = def;

	// The "=" needs to turn into a space
	size_t pos_equal = tmp_def.find_first_of("=");
	if (pos_equal != std::string::npos)
	{
		tmp_def[pos_equal] = ' ';
	}

	preamble.append("#define " + tmp_def + "\n");

	update_id();
}

void ShaderVariant::add_undefine(const std::string &undef)
{
	processes.push_back("U" + undef);

	preamble.append("#undef " + undef + "\n");

	update_id();
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

const std::string &ShaderVariant::get_preamble() const
{
	return preamble;
}

const std::vector<std::string> &ShaderVariant::get_processes() const
{
	return processes;
}

const std::unordered_map<std::string, size_t> &ShaderVariant::get_runtime_array_sizes() const
{
	return runtime_array_sizes;
}

void ShaderVariant::clear()
{
	preamble.clear();
	processes.clear();
	runtime_array_sizes.clear();
	update_id();
}

void ShaderVariant::update_id()
{
	std::hash<std::string> hasher{};
	id = hasher(preamble);
}

ShaderSource::ShaderSource(std::vector<uint8_t> &&data) :
    data{std::move(data)}
{
	std::hash<std::string> hasher{};
	id = hasher(std::string{this->data.cbegin(), this->data.cend()});
}

ShaderSource::ShaderSource(const std::string &filename) :
    filename{filename},
    data{fs::read_shader(filename)}
{
	std::hash<std::string> hasher{};
	id = hasher(std::string{this->data.cbegin(), this->data.cend()});
}

size_t ShaderSource::get_id() const
{
	return id;
}

const std::string &ShaderSource::get_filename() const
{
	return filename;
}

const std::vector<uint8_t> &ShaderSource::get_data() const
{
	return data;
}
}        // namespace vkb
