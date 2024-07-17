/* Copyright (c) 2023-2024, NVIDIA CORPORATION. All rights reserved.
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

#include "resource_binding_state.h"
#include <core/hpp_image_view.h>
#include <core/hpp_sampler.h>

namespace vkb
{
/**
 * @brief facade class around vkb::ResourceBindingState, providing a vulkan.hpp-based interface
 *
 * See vkb::ResourceBindingState for documentation
 */
struct HPPResourceInfo
{
	bool                           dirty      = false;
	const vkb::core::BufferCpp    *buffer     = nullptr;
	vk::DeviceSize                 offset     = 0;
	vk::DeviceSize                 range      = 0;
	const vkb::core::HPPImageView *image_view = nullptr;
	const vkb::core::HPPSampler   *sampler    = nullptr;
};

class HPPResourceSet : private vkb::ResourceSet
{
  public:
	using vkb::ResourceSet::is_dirty;

  public:
	const BindingMap<HPPResourceInfo> &get_resource_bindings() const
	{
		return reinterpret_cast<BindingMap<HPPResourceInfo> const &>(vkb::ResourceSet::get_resource_bindings());
	}
};

class HPPResourceBindingState : private vkb::ResourceBindingState
{
  public:
	using vkb::ResourceBindingState::clear_dirty;
	using vkb::ResourceBindingState::is_dirty;
	using vkb::ResourceBindingState::reset;

  public:
	void bind_buffer(const vkb::core::BufferCpp &buffer, vk::DeviceSize offset, vk::DeviceSize range, uint32_t set, uint32_t binding, uint32_t array_element)
	{
		vkb::ResourceBindingState::bind_buffer(reinterpret_cast<vkb::core::BufferC const &>(buffer),
		                                       static_cast<VkDeviceSize>(offset),
		                                       static_cast<VkDeviceSize>(range),
		                                       set,
		                                       binding,
		                                       array_element);
	}

	void bind_image(const vkb::core::HPPImageView &image_view, const vkb::core::HPPSampler &sampler, uint32_t set, uint32_t binding, uint32_t array_element)
	{
		vkb::ResourceBindingState::bind_image(
		    reinterpret_cast<vkb::core::ImageView const &>(image_view), reinterpret_cast<vkb::core::Sampler const &>(sampler), set, binding, array_element);
	}

	void bind_image(const vkb::core::HPPImageView &image_view, uint32_t set, uint32_t binding, uint32_t array_element)
	{
		vkb::ResourceBindingState::bind_image(reinterpret_cast<vkb::core::ImageView const &>(image_view), set, binding, array_element);
	}

	void bind_input(const vkb::core::HPPImageView &image_view, uint32_t set, uint32_t binding, uint32_t array_element)
	{
		vkb::ResourceBindingState::bind_input(reinterpret_cast<vkb::core::ImageView const &>(image_view), set, binding, array_element);
	}

	const std::unordered_map<uint32_t, vkb::HPPResourceSet> &get_resource_sets()
	{
		return reinterpret_cast<std::unordered_map<uint32_t, vkb::HPPResourceSet> const &>(vkb::ResourceBindingState::get_resource_sets());
	}
};
}        // namespace vkb
