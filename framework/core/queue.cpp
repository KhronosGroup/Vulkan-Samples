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

#include "queue.h"

#include "command_buffer.h"
#include "device.h"

namespace vkb
{
Queue::Queue(Device &device, uint32_t family_index, VkQueueFamilyProperties properties, VkBool32 can_present, uint32_t index) :
    device{device},
    family_index{family_index},
    index{index},
    can_present{can_present},
    properties{properties}
{
	vkGetDeviceQueue(device.get_handle(), family_index, index, &handle);
}

Queue::Queue(Queue &&other)  noexcept :
    device{other.device},
    handle{other.handle},
    family_index{other.family_index},
    index{other.index},
    can_present{other.can_present},
    properties{other.properties}
{
	other.handle       = VK_NULL_HANDLE;
	other.family_index = {};
	other.properties   = {};
	other.can_present  = VK_FALSE;
	other.index        = 0;
}

const Device &Queue::get_device() const
{
	return device;
}

VkQueue Queue::get_handle() const
{
	return handle;
}

uint32_t Queue::get_family_index() const
{
	return family_index;
}

uint32_t Queue::get_index() const
{
	return index;
}

VkQueueFamilyProperties Queue::get_properties() const
{
	return properties;
}

VkBool32 Queue::support_present() const
{
	return can_present;
}

VkResult Queue::submit(const std::vector<VkSubmitInfo> &submit_infos, VkFence fence) const
{
	return vkQueueSubmit(handle, to_u32(submit_infos.size()), submit_infos.data(), fence);
}

VkResult Queue::submit(const CommandBuffer &command_buffer, VkFence fence) const
{
	VkSubmitInfo submit_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &command_buffer.get_handle();

	return submit({submit_info}, fence);
}

VkResult Queue::present(const VkPresentInfoKHR &present_info) const
{
	if (!can_present)
	{
		return VK_ERROR_INCOMPATIBLE_DISPLAY_KHR;
	}

	return vkQueuePresentKHR(handle, &present_info);
}        // namespace vkb

VkResult Queue::wait_idle() const
{
	return vkQueueWaitIdle(handle);
}
}        // namespace vkb
