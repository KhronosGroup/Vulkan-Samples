/* Copyright (c) 2019-2022, Arm Limited and Contributors
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
#include "core/swapchain.h"

namespace vkb
{
namespace core
{
template <vkb::BindingType bindingType>
class CommandBuffer;
}
class Device;

class Queue
{
  public:
	Queue(Device &device, uint32_t family_index, VkQueueFamilyProperties properties, VkBool32 can_present, uint32_t index);

	Queue(const Queue &) = default;

	Queue(Queue &&other);

	Queue &operator=(const Queue &) = delete;

	Queue &operator=(Queue &&) = delete;

	const Device &get_device() const;

	VkQueue get_handle() const;

	uint32_t get_family_index() const;

	uint32_t get_index() const;

	const VkQueueFamilyProperties &get_properties() const;

	VkBool32 support_present() const;

	VkResult submit(const std::vector<VkSubmitInfo> &submit_infos, VkFence fence) const;

	VkResult submit(const vkb::core::CommandBuffer<vkb::BindingType::C> &command_buffer, VkFence fence) const;

	VkResult present(const VkPresentInfoKHR &present_infos) const;

	VkResult wait_idle() const;

  private:
	Device &device;

	VkQueue handle{VK_NULL_HANDLE};

	uint32_t family_index{0};

	uint32_t index{0};

	VkBool32 can_present{VK_FALSE};

	VkQueueFamilyProperties properties{};
};
}        // namespace vkb
