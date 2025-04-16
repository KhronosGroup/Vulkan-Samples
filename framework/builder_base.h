/* Copyright (c) 2021-2025, NVIDIA CORPORATION. All rights reserved.
 * Copyright (c) 2025, Bradley Austin Davis. All rights reserved.
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

#include "common/vk_common.h"
#include "vulkan_type_mapping.h"
#include <vulkan/vulkan.hpp>

namespace vkb
{

namespace allocated
{

/**
 * @brief Many Vulkan resource types (most notably Images and to a lesser extent Buffers)
 * and and their corresponding memory allocations have many parameters that need to be setup
 * when creating them.  Although many of these have reasonable defaults, constructors with
 * numerous arguments, some or all of which may have default arguments, aren't well suited
 * to partial customization.  This is a common failing of languages that don't support named
 * arguments and has led to the common use of the [builder pattern](https://en.wikipedia.org/wiki/Builder_pattern),
 * where a helper class is used to store all the options that can be tweaked for an object
 * when it's created. A builder class will have reasonable defaults where appropriate and only
 * require arguments for the builder constructor when a value is always required for creation to occur
 * (for example, the size of a buffer or the extent of an image).  Remaining parameters can be set
 * with methods on the builder class, which return a reference to the builder object, allowing
 * chaining of the method calls.
 *
 * This builder class serves as a base containing options that are common to all
 * [VMA](https://gpuopen.com/vulkan-memory-allocator/) allocated and managed resources.
 * For instance, the VMA create and usage flags are set here, but the image or buffer
 * usage flags are handled in the derived builder classes specific to those types.
 *
 * The following is an example of how the builder pattern is used in the codebase:
 ```cpp
        vkb::core::ImageBuilder(VkExtent3D{grid_width, grid_height, 1})
            .with_format(VK_FORMAT_R8G8B8A8_UNORM)
            .with_usage(VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
            .with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY)
            .with_sample_count(VK_SAMPLE_COUNT_1_BIT)
            .with_mip_levels(1)
            .with_array_layers(1)
            .with_tiling(VK_IMAGE_TILING_OPTIMAL)
            .with_queue_families(static_cast<uint32_t>(queue_families.size()), queue_families.data())
            .with_sharing_mode(sharing_mode));
 ```
 * The actual image can be created with `build()` which returns a `vkb::core::Image` or `buildPtr` which returns a `std::unique_ptr<vkb::core::Image>`.
 * Alternatively, the builder can be used as an argument to the `Image` constructor, which will build the image for you in place.
 * @note The builder pattern is intended to displace the currently used `vkb::core::Image` and `vkb::core::Buffer` constructors with numerous
 * arguments, but this is a work in progress and not currently in wide use in the codebase.
 *
 * @tparam BuilderType Allow the same builder base class to be used
 * with a variety of subclasses while using casting to return the corect dervied type
 * from the modifier methods.
 * @tparam bindingType A flag indicating whether this is being used with the C or C++ API
 * @tparam CreateInfoType The type of the Vulkan create info structure. Either a `VkSomethingCreateInfo`
 * or `vk::SomethingCreateInfo` for the C or C++ API respectively.
 */
template <vkb::BindingType bindingType, typename BuilderType, typename CreateInfoType>
class BuilderBase
{
  public:
	using MemoryPropertyFlagsType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::MemoryPropertyFlags, VkMemoryPropertyFlags>::type;
	using SharingModeType         = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::SharingMode, VkSharingMode>::type;

  public:
	VmaAllocationCreateInfo const &get_allocation_create_info() const;
	CreateInfoType const          &get_create_info() const;
	std::string const             &get_debug_name() const;
	BuilderType                   &with_debug_name(const std::string &name);
	BuilderType                   &with_implicit_sharing_mode();
	BuilderType                   &with_memory_type_bits(uint32_t type_bits);
	BuilderType                   &with_queue_families(uint32_t count, const uint32_t *family_indices);
	BuilderType                   &with_queue_families(std::vector<uint32_t> const &queue_families);
	BuilderType                   &with_sharing_mode(SharingModeType sharing_mode);
	BuilderType                   &with_vma_flags(VmaAllocationCreateFlags flags);
	BuilderType                   &with_vma_pool(VmaPool pool);
	BuilderType                   &with_vma_preferred_flags(MemoryPropertyFlagsType flags);
	BuilderType                   &with_vma_required_flags(MemoryPropertyFlagsType flags);
	BuilderType                   &with_vma_usage(VmaMemoryUsage usage);

  protected:
	BuilderBase(const BuilderBase &other) = delete;
	BuilderBase(const CreateInfoType &create_info);

	CreateInfoType &get_create_info();

  private:
	// we always want to store a vk::CreateInfo, so we have to figure out that type, depending on the BindingType!
	using HPPCreateInfoType = typename vkb::VulkanTypeMapping<bindingType, CreateInfoType>::Type;

  protected:
	VmaAllocationCreateInfo alloc_create_info = {};
	HPPCreateInfoType       create_info       = {};
	std::string             debug_name        = {};
};

template <typename BuilderType, typename CreateInfoType>
using BuilderBaseC = BuilderBase<vkb::BindingType::C, BuilderType, CreateInfoType>;
template <typename BuilderType, typename CreateInfoType>
using BuilderBaseCpp = BuilderBase<vkb::BindingType::Cpp, BuilderType, CreateInfoType>;

template <vkb::BindingType bindingType, typename BuilderType, typename CreateInfoType>
inline BuilderBase<bindingType, BuilderType, CreateInfoType>::BuilderBase(const CreateInfoType &create_info_) :
    create_info{reinterpret_cast<HPPCreateInfoType const &>(create_info_)}
{
	alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO;
};

template <vkb::BindingType bindingType, typename BuilderType, typename CreateInfoType>
inline VmaAllocationCreateInfo const &BuilderBase<bindingType, BuilderType, CreateInfoType>::get_allocation_create_info() const
{
	return alloc_create_info;
}

template <vkb::BindingType bindingType, typename BuilderType, typename CreateInfoType>
inline CreateInfoType const &BuilderBase<bindingType, BuilderType, CreateInfoType>::get_create_info() const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return create_info;
	}
	else
	{
		return *reinterpret_cast<typename HPPCreateInfoType::NativeType const *>(&create_info);
	}
}

template <vkb::BindingType bindingType, typename BuilderType, typename CreateInfoType>
inline CreateInfoType &BuilderBase<bindingType, BuilderType, CreateInfoType>::get_create_info()
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return create_info;
	}
	else
	{
		return *reinterpret_cast<typename HPPCreateInfoType::NativeType *>(&create_info);
	}
}

template <vkb::BindingType bindingType, typename BuilderType, typename CreateInfoType>
inline std::string const &BuilderBase<bindingType, BuilderType, CreateInfoType>::get_debug_name() const
{
	return debug_name;
}

template <vkb::BindingType bindingType, typename BuilderType, typename CreateInfoType>
inline BuilderType &BuilderBase<bindingType, BuilderType, CreateInfoType>::with_debug_name(const std::string &name)
{
	debug_name = name;
	return *static_cast<BuilderType *>(this);
}

template <vkb::BindingType bindingType, typename BuilderType, typename CreateInfoType>
inline BuilderType &BuilderBase<bindingType, BuilderType, CreateInfoType>::with_implicit_sharing_mode()
{
	create_info.sharingMode = (1 < create_info.queueFamilyIndexCount) ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive;
	return *static_cast<BuilderType *>(this);
}

template <vkb::BindingType bindingType, typename BuilderType, typename CreateInfoType>
inline BuilderType &BuilderBase<bindingType, BuilderType, CreateInfoType>::with_memory_type_bits(uint32_t type_bits)
{
	alloc_create_info.memoryTypeBits = type_bits;
	return *static_cast<BuilderType *>(this);
}

template <vkb::BindingType bindingType, typename BuilderType, typename CreateInfoType>
inline BuilderType &BuilderBase<bindingType, BuilderType, CreateInfoType>::with_queue_families(uint32_t count, const uint32_t *family_indices)
{
	create_info.queueFamilyIndexCount = count;
	create_info.pQueueFamilyIndices   = family_indices;
	return *static_cast<BuilderType *>(this);
}

template <vkb::BindingType bindingType, typename BuilderType, typename CreateInfoType>
inline BuilderType &BuilderBase<bindingType, BuilderType, CreateInfoType>::with_queue_families(std::vector<uint32_t> const &queue_families)
{
	return with_queue_families(static_cast<uint32_t>(queue_families.size()), queue_families.data());
}

template <vkb::BindingType bindingType, typename BuilderType, typename CreateInfoType>
inline BuilderType &BuilderBase<bindingType, BuilderType, CreateInfoType>::with_sharing_mode(SharingModeType sharing_mode)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		create_info.sharingMode = sharing_mode;
	}
	else
	{
		create_info.sharingMode = static_cast<vk::SharingMode>(sharing_mode);
	}
	return *static_cast<BuilderType *>(this);
}

template <vkb::BindingType bindingType, typename BuilderType, typename CreateInfoType>
inline BuilderType &BuilderBase<bindingType, BuilderType, CreateInfoType>::with_vma_flags(VmaAllocationCreateFlags flags)
{
	alloc_create_info.flags = flags;
	return *static_cast<BuilderType *>(this);
}

template <vkb::BindingType bindingType, typename BuilderType, typename CreateInfoType>
inline BuilderType &BuilderBase<bindingType, BuilderType, CreateInfoType>::with_vma_pool(VmaPool pool)
{
	alloc_create_info.pool = pool;
	return *static_cast<BuilderType *>(this);
}

template <vkb::BindingType bindingType, typename BuilderType, typename CreateInfoType>
inline BuilderType &BuilderBase<bindingType, BuilderType, CreateInfoType>::with_vma_preferred_flags(MemoryPropertyFlagsType flags)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		alloc_create_info.preferredFlags = static_cast<VkMemoryPropertyFlags>(flags);
	}
	else
	{
		alloc_create_info.preferredFlags = flags;
	}
	return *static_cast<BuilderType *>(this);
}

template <vkb::BindingType bindingType, typename BuilderType, typename CreateInfoType>
inline BuilderType &BuilderBase<bindingType, BuilderType, CreateInfoType>::with_vma_required_flags(MemoryPropertyFlagsType flags)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		alloc_create_info.requiredFlags = static_cast<VkMemoryPropertyFlags>(flags);
	}
	else
	{
		alloc_create_info.requiredFlags = flags;
	}
	return *static_cast<BuilderType *>(this);
}

template <vkb::BindingType bindingType, typename BuilderType, typename CreateInfoType>
inline BuilderType &BuilderBase<bindingType, BuilderType, CreateInfoType>::with_vma_usage(VmaMemoryUsage usage)
{
	alloc_create_info.usage = usage;
	return *static_cast<BuilderType *>(this);
}

}        // namespace allocated
}        // namespace vkb
