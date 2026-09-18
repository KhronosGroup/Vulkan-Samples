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

#include "common/hpp_error.h"
#include "common/vk_common.h"
#include "filesystem/legacy.h"
#include <vulkan/vulkan.hpp>

namespace vkb
{
namespace core
{
template <vkb::BindingType bindingType>
class Device;
using DeviceCpp = Device<vkb::BindingType::Cpp>;
using DeviceC   = Device<vkb::BindingType::C>;

// Types of shader resources
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
	SpecializationConstant,
	All
};

// This determines the type and method of how descriptor set should be created and bound
enum class ShaderResourceMode
{
	Static,
	Dynamic,
	UpdateAfterBind
};

// A bitmask of qualifiers applied to a resource
enum class ShaderResourceQualifierFlagBits
{
	NonReadable = 1,
	NonWritable = 2,
};
using ShaderResourceQualifierFlags = vk::Flags<ShaderResourceQualifierFlagBits>;

// Store shader resource data.
// Used by the shader module.
template <vkb::BindingType bindingType>
struct ShaderResource
{
	using ShaderStageFlagsType = std::conditional_t<bindingType == vkb::BindingType::Cpp, vk::ShaderStageFlags, VkShaderStageFlags>;

	ShaderStageFlagsType                    stages;
	vkb::core::ShaderResourceType           type;
	vkb::core::ShaderResourceMode           mode;
	uint32_t                                set;
	uint32_t                                binding;
	uint32_t                                location;
	uint32_t                                input_attachment_index;
	uint32_t                                vec_size;
	uint32_t                                columns;
	uint32_t                                array_size;
	uint32_t                                offset;
	uint32_t                                size;
	uint32_t                                constant_id;
	vkb::core::ShaderResourceQualifierFlags qualifiers;
	std::string                             name;
};

using ShaderResourceC   = ShaderResource<vkb::BindingType::C>;
using ShaderResourceCpp = ShaderResource<vkb::BindingType::Cpp>;
/**
 * @brief Adds support for C style preprocessor macros to glsl shaders enabling you to define or undefine certain symbols
 */
class ShaderVariant
{
  public:
	ShaderVariant() = default;

	/**
	 * @brief Specifies the size of a named runtime array for automatic reflection. If already specified, overrides the size.
	 * @param runtime_array_name String under which the runtime array is named in the shader
	 * @param size Integer specifying the wanted size of the runtime array (in number of elements, not size in bytes), used for automatic allocation of buffers.
	 * See get_declared_struct_size_runtime_array() in spirv_cross.h
	 */
	void add_runtime_array_size(std::string const &runtime_array_name, size_t size);

	void                                           clear();
	size_t                                         get_id() const;
	std::unordered_map<std::string, size_t> const &get_runtime_array_sizes() const;
	void                                           set_runtime_array_sizes(std::unordered_map<std::string, size_t> const &sizes);

  private:
	size_t                                  id;
	std::unordered_map<std::string, size_t> runtime_array_sizes;
};

class ShaderSource
{
  public:
	ShaderSource() = default;
	ShaderSource(std::string const &filename);

	std::string const &get_filename() const;
	size_t             get_id() const;
	std::string const &get_source() const;
	void               set_source(std::string const &source);

  private:
	size_t      id;
	std::string filename;
	std::string source;
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
template <vkb::BindingType bindingType>
class ShaderModule
{
  public:
	using ShaderStageFlagBitsType = std::conditional_t<bindingType == vkb::BindingType::Cpp, vk::ShaderStageFlagBits, VkShaderStageFlagBits>;

  public:
	ShaderModule(vkb::core::Device<bindingType> & /* device*/,
	             ShaderStageFlagBitsType         stage,
	             vkb::core::ShaderSource const  &shader_source,
	             std::string const              &entry_point,
	             vkb::core::ShaderVariant const &shader_variant);

	ShaderModule(ShaderModule const &) = delete;
	ShaderModule(ShaderModule &&other);

	ShaderModule &operator=(ShaderModule const &) = delete;
	ShaderModule &operator=(ShaderModule &&)      = delete;

	std::vector<uint32_t> const                               &get_binary() const;
	std::string const                                         &get_debug_name() const;
	std::string const                                         &get_entry_point() const;
	size_t                                                     get_id() const;
	std::vector<vkb::core::ShaderResource<bindingType>> const &get_resources() const;
	ShaderStageFlagBitsType                                    get_stage() const;
	void                                                       set_debug_name(std::string const &name);

	/**
	 * @brief Flags a resource to use a different method of being bound to the shader
	 * @param resource_name The name of the shader resource
	 * @param resource_mode The mode of how the shader resource will be bound
	 */
	void set_resource_mode(std::string const &resource_name, vkb::core::ShaderResourceMode const &resource_mode);

  private:
	void init(vkb::core::ShaderSource const &shader_source, vkb::core::ShaderVariant const &shader_variant);

  private:
	std::string                               debug_name;         // Human-readable name for the shader
	size_t                                    id;                 // Shader unique id
	std::string                               entry_point;        // Name of the main function
	std::vector<vkb::core::ShaderResourceCpp> resources;
	std::vector<uint32_t>                     spirv;             // Compiled source
	vk::ShaderStageFlagBits                   stage = {};        // Stage of the shader (vertex, fragment, etc)
};

using ShaderModuleC   = ShaderModule<vkb::BindingType::C>;
using ShaderModuleCpp = ShaderModule<vkb::BindingType::Cpp>;
}        // namespace core
}        // namespace vkb

#include "spirv_reflection.h"

namespace vkb
{
namespace core
{
inline void ShaderVariant::add_runtime_array_size(std::string const &runtime_array_name, size_t size)
{
	runtime_array_sizes[runtime_array_name] = size;
}

inline void ShaderVariant::clear()
{
	runtime_array_sizes.clear();
	id = 0;
}

inline size_t ShaderVariant::get_id() const
{
	return id;
}

inline std::unordered_map<std::string, size_t> const &ShaderVariant::get_runtime_array_sizes() const
{
	return runtime_array_sizes;
}

inline void ShaderVariant::set_runtime_array_sizes(std::unordered_map<std::string, size_t> const &sizes)
{
	runtime_array_sizes = sizes;
}

inline ShaderSource::ShaderSource(std::string const &filename) :
    filename{filename}, source{fs::read_text_file(filename)}
{
	std::hash<std::string> hasher{};
	id = hasher(std::string{this->source.cbegin(), this->source.cend()});
}

inline std::string const &ShaderSource::get_filename() const
{
	return filename;
}

inline size_t ShaderSource::get_id() const
{
	return id;
}

inline std::string const &ShaderSource::get_source() const
{
	return source;
}

inline void ShaderSource::set_source(std::string const &source_)
{
	source = source_;
	std::hash<std::string> hasher{};
	id = hasher(std::string{this->source.cbegin(), this->source.cend()});
}

template <>
inline ShaderModule<vkb::BindingType::C>::ShaderModule([[maybe_unused]] vkb::core::DeviceC &device,
                                                       VkShaderStageFlagBits                stage,
                                                       vkb::core::ShaderSource const       &shader_source,
                                                       std::string const                   &entry_point,
                                                       vkb::core::ShaderVariant const      &shader_variant) :
    stage{static_cast<vk::ShaderStageFlagBits>(stage)}, entry_point{entry_point}
{
	init(shader_source, shader_variant);
}

template <>
inline ShaderModule<vkb::BindingType::Cpp>::ShaderModule([[maybe_unused]] vkb::core::DeviceCpp &device,
                                                         vk::ShaderStageFlagBits                stage,
                                                         vkb::core::ShaderSource const         &shader_source,
                                                         std::string const                     &entry_point,
                                                         vkb::core::ShaderVariant const        &shader_variant) :
    stage{stage}, entry_point{entry_point}
{
	init(shader_source, shader_variant);
}

template <vkb::BindingType bindingType>
inline void ShaderModule<bindingType>::init(vkb::core::ShaderSource const &shader_source, vkb::core::ShaderVariant const &shader_variant)
{
	debug_name = fmt::format("{} [variant {:X}] [entrypoint {}]", shader_source.get_filename(), shader_variant.get_id(), entry_point);

	// Shaders in binary SPIR-V format can be loaded directly
	spirv = vkb::fs::read_shader_binary_u32(shader_source.get_filename());

	// Reflection is used to dynamically create descriptor bindings

	SPIRVReflectionCpp spirv_reflection;
	// Reflect all shader resources
	if (!spirv_reflection.reflect_shader_resources(stage, spirv, resources, shader_variant))
	{
		throw vkb::common::HPPVulkanException(vk::Result::eErrorInitializationFailed);
	}

	// Generate a unique id, determined by source and variant
	std::hash<std::string> hasher{};
	id = hasher(std::string{reinterpret_cast<char const *>(spirv.data()), reinterpret_cast<char const *>(spirv.data() + spirv.size())});
}

template <vkb::BindingType bindingType>
inline ShaderModule<bindingType>::ShaderModule(ShaderModule<bindingType> &&other) :
    id{other.id},
    stage{other.stage},
    entry_point{other.entry_point},
    debug_name{other.debug_name},
    spirv{other.spirv},
    resources{other.resources}
{
	other.stage = {};
}

template <vkb::BindingType bindingType>
inline std::vector<uint32_t> const &ShaderModule<bindingType>::get_binary() const
{
	return spirv;
}

template <vkb::BindingType bindingType>
inline std::string const &ShaderModule<bindingType>::get_debug_name() const
{
	return debug_name;
}

template <vkb::BindingType bindingType>
inline std::string const &ShaderModule<bindingType>::get_entry_point() const
{
	return entry_point;
}

template <vkb::BindingType bindingType>
inline size_t ShaderModule<bindingType>::get_id() const
{
	return id;
}

template <vkb::BindingType bindingType>
inline std::vector<vkb::core::ShaderResource<bindingType>> const &ShaderModule<bindingType>::get_resources() const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return resources;
	}
	else
	{
		return reinterpret_cast<std::vector<vkb::core::ShaderResourceC> const &>(resources);
	}
}

template <vkb::BindingType bindingType>
inline typename ShaderModule<bindingType>::ShaderStageFlagBitsType ShaderModule<bindingType>::get_stage() const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return stage;
	}
	else
	{
		return static_cast<VkShaderStageFlagBits>(stage);
	}
}

template <vkb::BindingType bindingType>
inline void ShaderModule<bindingType>::set_debug_name(std::string const &name)
{
	debug_name = name;
}

template <vkb::BindingType bindingType>
inline void ShaderModule<bindingType>::set_resource_mode(std::string const &resource_name, vkb::core::ShaderResourceMode const &resource_mode)
{
	auto it = std::ranges::find_if(resources, [&resource_name](vkb::core::ShaderResourceCpp const &resource) { return resource.name == resource_name; });

	if (it != resources.end())
	{
		if (resource_mode == vkb::core::ShaderResourceMode::Dynamic)
		{
			if (it->type == vkb::core::ShaderResourceType::BufferUniform ||
			    it->type == vkb::core::ShaderResourceType::BufferStorage)
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

}        // namespace core
}        // namespace vkb
