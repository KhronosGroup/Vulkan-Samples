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

#include "resource_binding_state.h"

namespace vkb
{
void ResourceBindingState::reset()
{
	clear_dirty();

	resource_sets.clear();
}

bool ResourceBindingState::is_dirty()
{
	return dirty;
}

void ResourceBindingState::clear_dirty()
{
	dirty = false;
}

void ResourceBindingState::clear_dirty(uint32_t set)
{
	resource_sets[set].clear_dirty();
}

void ResourceBindingState::bind_buffer(const core::Buffer &buffer, VkDeviceSize offset, VkDeviceSize range, uint32_t set, uint32_t binding, uint32_t array_element)
{
	resource_sets[set].bind_buffer(buffer, offset, range, binding, array_element);

	dirty = true;
}

void ResourceBindingState::bind_image(const core::ImageView &image_view, const core::Sampler &sampler, uint32_t set, uint32_t binding, uint32_t array_element)
{
	resource_sets[set].bind_image(image_view, sampler, binding, array_element);

	dirty = true;
}

void ResourceBindingState::bind_image(const core::ImageView &image_view, uint32_t set, uint32_t binding, uint32_t array_element)
{
	resource_sets[set].bind_image(image_view, binding, array_element);

	dirty = true;
}

void ResourceBindingState::bind_input(const core::ImageView &image_view, uint32_t set, uint32_t binding, uint32_t array_element)
{
	resource_sets[set].bind_input(image_view, binding, array_element);

	dirty = true;
}

const std::unordered_map<uint32_t, ResourceSet> &ResourceBindingState::get_resource_sets()
{
	return resource_sets;
}

void ResourceSet::reset()
{
	clear_dirty();

	resource_bindings.clear();
}

bool ResourceSet::is_dirty() const
{
	return dirty;
}

void ResourceSet::clear_dirty()
{
	dirty = false;
}

void ResourceSet::clear_dirty(uint32_t binding, uint32_t array_element)
{
	resource_bindings[binding][array_element].dirty = false;
}

void ResourceSet::bind_buffer(const core::Buffer &buffer, VkDeviceSize offset, VkDeviceSize range, uint32_t binding, uint32_t array_element)
{
	resource_bindings[binding][array_element].dirty  = true;
	resource_bindings[binding][array_element].buffer = &buffer;
	resource_bindings[binding][array_element].offset = offset;
	resource_bindings[binding][array_element].range  = range;

	dirty = true;
}

void ResourceSet::bind_image(const core::ImageView &image_view, const core::Sampler &sampler, uint32_t binding, uint32_t array_element)
{
	resource_bindings[binding][array_element].dirty      = true;
	resource_bindings[binding][array_element].image_view = &image_view;
	resource_bindings[binding][array_element].sampler    = &sampler;

	dirty = true;
}

void ResourceSet::bind_image(const core::ImageView &image_view, uint32_t binding, uint32_t array_element)
{
	resource_bindings[binding][array_element].dirty      = true;
	resource_bindings[binding][array_element].image_view = &image_view;
	resource_bindings[binding][array_element].sampler    = nullptr;

	dirty = true;
}

void ResourceSet::bind_input(const core::ImageView &image_view, const uint32_t binding, const uint32_t array_element)
{
	resource_bindings[binding][array_element].dirty      = true;
	resource_bindings[binding][array_element].image_view = &image_view;

	dirty = true;
}

const BindingMap<ResourceInfo> &ResourceSet::get_resource_bindings() const
{
	return resource_bindings;
}

}        // namespace vkb
