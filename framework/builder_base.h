/* Copyright (c) 2021-2024, NVIDIA CORPORATION. All rights reserved.
 * Copyright (c) 2024, Bradley Austin Davis. All rights reserved.
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
inline BuilderBase<bindingType, BuilderType, CreateInfoType>::BuilderBase(const CreateInfoType &create_info) :
    create_info(create_info)
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
	create_info.sharingMode = (create_info.queueFamilyIndexCount != 0) ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive;
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
		create_info.sharingMode = static_cast<VkSharingMode>(sharing_mode);
	}
	else
	{
		create_info.sharingMode = sharing_mode;
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

}        // namespace vkb
