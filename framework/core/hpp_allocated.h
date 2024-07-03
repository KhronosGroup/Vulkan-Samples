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

#include "allocated.h"
#include "vulkan_resource.h"

namespace vkb
{
namespace allocated
{

template <
    typename BuilderType,
    typename CreateInfoType>
struct HPPBuilder : public Builder<BuilderType, CreateInfoType, vk::SharingMode>
{
  private:
	using Parent = Builder<BuilderType, CreateInfoType, vk::SharingMode>;

  protected:
	HPPBuilder(const HPPBuilder &other) = delete;

	HPPBuilder(const CreateInfoType &create_info) :
	    Parent(create_info)
	{}

  public:
	BuilderType &with_vma_required_flags(const vk::MemoryPropertyFlags &flags)
	{
		Parent::alloc_create_info.requiredFlags = flags.operator VkMemoryPropertyFlags();
		return *static_cast<BuilderType *>(this);
	}

	BuilderType &with_vma_preferred_flags(const vk::MemoryPropertyFlags &flags)
	{
		Parent::alloc_create_info.preferredFlags = flags.operator VkMemoryPropertyFlags();
		return *static_cast<BuilderType *>(this);
	}

	BuilderType &with_sharing(vk::SharingMode sharingMode)
	{
		Parent::create_info.sharingMode = sharingMode;
		return *static_cast<BuilderType *>(this);
	}
};

template <typename HandleType>
class HPPAllocated : public Allocated<
                         HandleType,
                         vk::DeviceMemory,
                         vkb::core::VulkanResourceCpp<HandleType>>
{
	using Parent = Allocated<HandleType, vk::DeviceMemory, vkb::core::VulkanResourceCpp<HandleType>>;

  public:
	using Parent::get_handle;
	using Parent::Parent;
	using Parent::update;

	/**
	 * @brief Copies byte data into the buffer
	 * @param data The data to copy from
	 * @param offset The offset to start the copying into the mapped data
	 */
	template <typename T>
	vk::DeviceSize update(const vk::ArrayProxy<const T> &data, size_t offset = 0)
	{
		return Parent::update(static_cast<const uint8_t *>(data.data()), data.size() * sizeof(T), offset);
	}

	/**
	 * @brief Copies byte data into the buffer
	 * @param data The data to copy from
	 * @param count The number of array elements
	 * @param offset The offset to start the copying into the mapped data
	 */
	template <typename T>
	vk::DeviceSize update_from_array(const T *data, size_t count, size_t offset = 0)
	{
		return update(vk::ArrayProxy<const T>{data, count}, offset);
	}
};

}        // namespace allocated
}        // namespace vkb
