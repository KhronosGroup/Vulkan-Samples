/* Copyright (c) 2019-2023, Arm Limited and Contributors
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

#include "spdlog/sinks/basic_file_sink.h"

namespace vkb
{
/**
 * @brief Pre-compiles project shader files to include header code
 * @param source The shader file
 * @returns A byte array of the final shader
 */
inline std::vector<std::string> precompile_shader(const std::string &source)
{
	std::vector<std::string> final_file;

	auto lines = split(source, '\n');

	for (auto &line : lines)
	{
		if (line.find("#include \"") == 0)
		{
			// Include paths are relative to the base shader directory
			std::string include_path = line.substr(10);
			size_t      last_quote   = include_path.find("\"");
			if (!include_path.empty() && last_quote != std::string::npos)
			{
				include_path = include_path.substr(0, last_quote);
			}

			auto include_file = precompile_shader(fs::read_shader(include_path));
			for (auto &include_file_line : include_file)
			{
				final_file.push_back(include_file_line);
			}
		}
		else
		{
			final_file.push_back(line);
		}
	}

	return final_file;
}

inline std::vector<uint8_t> convert_to_bytes(std::vector<std::string> &lines)
{
	std::vector<uint8_t> bytes;

	for (auto &line : lines)
	{
		line += "\n";
		std::vector<uint8_t> line_bytes(line.begin(), line.end());
		bytes.insert(bytes.end(), line_bytes.begin(), line_bytes.end());
	}

	return bytes;
}

ShaderModule::ShaderModule(Device &device, VkShaderStageFlagBits stage, const ShaderSource &glsl_source, const std::string &entry_point, const ShaderVariant &shader_variant) :
    device{device},
    stage{stage},
    entry_point{entry_point}
{
	debug_name = fmt::format("{} [variant {:X}] [entrypoint {}]",
	                         glsl_source.get_filename(), shader_variant.get_id(), entry_point);

	// Compiling from GLSL source requires the entry point
	if (entry_point.empty())
	{
		throw std::runtime_error("Entry point must be specified when compiling from GLSL source");
	}

	if (glsl_source.get_type() == ShaderSource::Type::Source)
	{
		auto &source = glsl_source.get_source();

		// Check if application is passing in GLSL source code to compile to SPIR-V
		if (source.empty())
		{
			throw std::runtime_error("GLSL source is empty");
		}

		// Precompile source into the final spirv bytecode
		auto glsl_final_source = precompile_shader(source);

		// Compile the GLSL source
		GLSLCompiler glsl_compiler;

		if (!glsl_compiler.compile_to_spirv(stage, convert_to_bytes(glsl_final_source), entry_point, shader_variant, spirv, info_log))
		{
			LOGE("Shader compilation failed for shader \"{}\"", glsl_source.get_filename());
			LOGE("{}", info_log);
			throw VulkanException{VK_ERROR_INITIALIZATION_FAILED};
		}
	}
	else
	{
		spirv = glsl_source.get_spirv();
	}

	SPIRVReflection spirv_reflection;

	// Reflect all shader resources
	if (!spirv_reflection.reflect_shader_resources(stage, spirv, resources, shader_variant))
	{
		throw std::runtime_error("Failed to reflect shader resources");
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

void ShaderVariant::add_define(const std::string &def, const std::string &value)
{
	// processes.push_back("D" + def);

	definitions.emplace(def, value);

	preamble = "";
	for (auto &def : definitions)
	{
		preamble += "#define " + def.first + " " + def.second + "\n";
	}

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

ShaderSource::ShaderSource(const std::string &filename) :
    filename{filename}
{
	if (fs::is_file(fs::path::get(fs::path::Type::Shaders) + filename + ".spv"))
	{
		// We have a precompiled shader
		LOGI("Loading precompiled shader: {}", filename + ".spv");
		auto buffer = fs::read_shader_binary(filename + ".spv");
		spirv       = {reinterpret_cast<uint32_t *>(buffer.data()), reinterpret_cast<uint32_t *>(buffer.data() + buffer.size())};
		type        = ShaderSource::Type::Spirv;
	}
	else
	{
		LOGI("Precompiled shader was not found: {}", filename + ".spv");
		source = fs::read_shader(filename);
	}

	update_id();
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
	type   = ShaderSource::Type::Source;
	source = source_;
	update_id();
}

ShaderSource::Type ShaderSource::get_type() const
{
	return type;
}

const std::string &ShaderSource::get_source() const
{
	return source;
}

const std::vector<uint32_t> &ShaderSource::get_spirv() const
{
	return spirv;
}

void ShaderSource::update_id()
{
	std::hash<std::string> hasher{};
	if (type == ShaderSource::Type::Source)
	{
		id = hasher(std::string{source.cbegin(), source.cend()});
	}
	else if (type == ShaderSource::Type::Spirv)
	{
		id = hasher(std::string{reinterpret_cast<const char *>(spirv.data()), reinterpret_cast<const char *>(spirv.data() + spirv.size())});
	}
	else
	{
		throw std::runtime_error("Invalid shader source type");
	}
}

}        // namespace vkb
