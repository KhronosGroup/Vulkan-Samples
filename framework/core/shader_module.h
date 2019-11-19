/* Copyright (c) 2019, Arm Limited and Contributors
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
#include "common/vk_common.h"

namespace vkb
{
class Device;

/// Types of shader resources
enum class ShaderResourceType
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
	SpecializationConstant
};

/// Store shader resource data.
/// Used by the shader module.
struct ShaderResource
{
	VkShaderStageFlags stages;

	ShaderResourceType type;

	uint32_t set;

	uint32_t binding;

	uint32_t location;

	uint32_t input_attachment_index;

	uint32_t vec_size;

	uint32_t columns;

	uint32_t array_size;

	uint32_t offset;

	uint32_t size;

	uint32_t constant_id;

	bool dynamic;

	std::string name;
};

/**
 * @brief Adds support for C style preprocessor macros to glsl shaders
 *        enabling you to define or undefine certain symbols
 */
class ShaderVariant
{
  public:
	ShaderVariant() = default;

	ShaderVariant(std::string &&preamble, std::vector<std::string> &&processes);

	size_t get_id() const;

	/**
	 * @brief Adds a define macro to the shader
	 * @param def String which should go to the right of a define directive
	 */
	void add_define(const std::string &def);

	/**
	 * @brief Adds an undef macro to the shader
	 * @param undef String which should go to the right of an undef directive
	 */
	void add_undefine(const std::string &undef);

	/**
	 * @brief Specifies the size of a named runtime array for automatic reflection. If already specified, overrides the size.
	 * @param runtime_array_name String under which the runtime array is named in the shader
	 * @param size Integer specifying the wanted size of the runtime array (in number of elements, not size in bytes), used for automatic allocation of buffers.
	 * See get_declared_struct_size_runtime_array() in spirv_cross.h
	 */
	void add_runtime_array_size(const std::string &runtime_array_name, size_t size);

	void set_runtime_array_sizes(const std::unordered_map<std::string, size_t> &sizes);

	const std::string &get_preamble() const;

	const std::vector<std::string> &get_processes() const;

	const std::unordered_map<std::string, size_t> &get_runtime_array_sizes() const;

	void clear();

  private:
	size_t id;

	std::string preamble;

	std::vector<std::string> processes;

	std::unordered_map<std::string, size_t> runtime_array_sizes;

	void update_id();
};

class ShaderSource
{
  public:
	ShaderSource() = default;

	ShaderSource(const std::string &filename);

	ShaderSource(std::vector<uint8_t> &&data);

	size_t get_id() const;

	const std::string &get_filename() const;

	const std::vector<uint8_t> &get_data() const;

  private:
	size_t id;

	std::string filename;

	std::vector<uint8_t> data;
};

/**
 * @brief Contains shader code, with an entry point, for a specific shader stage.
 * It is needed by a PipelineLayout to create a Pipeline.
 * ShaderModule can do auto-pairing between shader code and textures.
 * The low level code can change bindings, just keeping the name of the texture.
 * Variants for each texture are also generated, such as HAS_BASE_COLOR_TEX.
 * It works similarly for attribute locations. A current limitation is that only set 0
 * is considered. Uniform buffers are currently hardcoded as well.
 */
class ShaderModule
{
  public:
	ShaderModule(Device &              device,
	             VkShaderStageFlagBits stage,
	             const ShaderSource &  glsl_source,
	             const std::string &   entry_point,
	             const ShaderVariant & shader_variant);

	ShaderModule(const ShaderModule &) = delete;

	ShaderModule(ShaderModule &&other);

	ShaderModule &operator=(const ShaderModule &) = delete;

	ShaderModule &operator=(ShaderModule &&) = delete;

	size_t get_id() const;

	VkShaderStageFlagBits get_stage() const;

	const std::string &get_entry_point() const;

	const std::vector<ShaderResource> &get_resources() const;

	const std::string &get_info_log() const;

	const std::vector<uint32_t> &get_binary() const;

	void set_resource_dynamic(const std::string &resource_name);

  private:
	Device &device;

	/// Shader unique id
	size_t id;

	/// Stage of the shader (vertex, fragment, etc)
	VkShaderStageFlagBits stage{};

	/// Name of the main function
	std::string entry_point;

	/// Compiled source
	std::vector<uint32_t> spirv;

	std::vector<ShaderResource> resources;

	std::string info_log;
};
}        // namespace vkb
