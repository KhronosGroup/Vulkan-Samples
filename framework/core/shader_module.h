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

#include "common/helpers.h"
#include "common/vk_common.h"

#include <shaders/shader_handle.hpp>
#include <shaders/shader_resources.hpp>

#if defined(VK_USE_PLATFORM_XLIB_KHR)
#	undef None
#endif

namespace vkb
{
class Device;

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
	 * @brief Add definitions to shader variant
	 * @param definitions Vector of definitions to add to the variant
	 */
	void add_definitions(const std::vector<std::string> &definitions);

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

	ShaderSource(const std::string &filename) :
	    filename{filename}
	{}

	// A temporary shim to bridge the gap between the old and new shader system
	ShaderHandle handle(const std::vector<std::string> &defines) const
	{
		return ShaderHandleBuilder()
		    .with_path(filename)
		    .with_defines(defines)
		    .build();
	}

  private:
	std::string filename;
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
	ShaderModule(Device               &device,
	             VkShaderStageFlagBits stage,
	             const ShaderSource   &glsl_source,
	             const std::string    &entry_point,
	             const ShaderVariant  &shader_variant);

	ShaderModule(const ShaderModule &) = delete;

	ShaderModule(ShaderModule &&other);

	ShaderModule &operator=(const ShaderModule &) = delete;

	ShaderModule &operator=(ShaderModule &&) = delete;

	size_t get_id() const;

	VkShaderStageFlagBits get_stage() const;

	const std::string &get_entry_point() const;

	const ShaderResourceSet &get_resources() const;

	const std::string &get_info_log() const;

	const std::vector<uint32_t> &get_binary() const;

	inline const std::string &get_debug_name() const
	{
		return debug_name;
	}

	inline void set_debug_name(const std::string &name)
	{
		debug_name = name;
	}

	/**
	 * @brief Flags a resource to use a different method of being bound to the shader
	 * @param resource_name The name of the shader resource
	 * @param resource_mode The mode of how the shader resource will be bound
	 */
	void set_resource_mode(const std::string &resource_name, const ShaderResourceMode &resource_mode);

  private:
	Device &device;

	/// Shader unique id
	size_t id;

	/// Stage of the shader (vertex, fragment, etc)
	VkShaderStageFlagBits stage{};

	/// Name of the main function
	std::string entry_point;

	/// Human-readable name for the shader
	std::string debug_name;

	/// Compiled source
	std::vector<uint32_t> spirv;

	ShaderResourceSet resources;

	std::string info_log;
};
}        // namespace vkb
