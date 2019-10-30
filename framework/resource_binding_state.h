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

#include "common/vk_common.h"
#include "core/buffer.h"
#include "core/image_view.h"
#include "core/sampler.h"

namespace vkb
{
struct ResourceInfo
{
	bool dirty{false};

	const core::Buffer *buffer{nullptr};

	VkDeviceSize offset{0};

	VkDeviceSize range{0};

	const core::ImageView *image_view{nullptr};

	const core::Sampler *sampler{nullptr};
};

class SetBindings
{
  public:
	void reset();

	bool is_dirty() const;

	void clear_dirty();

	void clear_dirty(uint32_t binding, uint32_t array_element);

	void bind_buffer(const core::Buffer &buffer, VkDeviceSize offset, VkDeviceSize range, uint32_t binding, uint32_t array_element);

	void bind_image(const core::ImageView &image_view, const core::Sampler &sampler, uint32_t binding, uint32_t array_element);

	void bind_input(const core::ImageView &image_view, uint32_t binding, uint32_t array_element);

	const BindingMap<ResourceInfo> &get_resource_bindings() const;

  private:
	bool dirty{false};

	BindingMap<ResourceInfo> resource_bindings;
};

class ResourceBindingState
{
  public:
	void reset();

	bool is_dirty();

	void clear_dirty();

	void clear_dirty(uint32_t set);

	void bind_buffer(const core::Buffer &buffer, VkDeviceSize offset, VkDeviceSize range, uint32_t set, uint32_t binding, uint32_t array_element);

	void bind_image(const core::ImageView &image_view, const core::Sampler &sampler, uint32_t set, uint32_t binding, uint32_t array_element);

	void bind_input(const core::ImageView &image_view, uint32_t set, uint32_t binding, uint32_t array_element);

	const std::unordered_map<uint32_t, SetBindings> &get_set_bindings();

  private:
	bool dirty{false};

	std::unordered_map<uint32_t, SetBindings> set_bindings;
};
}        // namespace vkb
