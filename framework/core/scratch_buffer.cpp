/* Copyright (c) 2021, Sascha Willems
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

#include "scratch_buffer.h"

#include "device.h"

namespace vkb
{
namespace core
{
ScratchBuffer::ScratchBuffer(Device &device, VkDeviceSize _size) :
    device{device}
{
	VkBufferCreateInfo buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	buffer_info.size  = _size;

	VmaAllocationCreateInfo memory_info{};
	memory_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	VmaAllocationInfo allocation_info{};
	auto              result = vmaCreateBuffer(device.get_memory_allocator(),
                                  &buffer_info, &memory_info,
                                  &handle, &allocation,
                                  &allocation_info);

	if (result != VK_SUCCESS)
	{
		throw VulkanException{result, "Could not create Scratch buffer"};
	}

	VkBufferDeviceAddressInfoKHR buffer_device_address_info{};
	buffer_device_address_info.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	buffer_device_address_info.buffer = handle;
	device_address                    = vkGetBufferDeviceAddressKHR(device.get_handle(), &buffer_device_address_info);
}

ScratchBuffer::~ScratchBuffer()
{
	if (handle != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE)
	{
		vmaDestroyBuffer(device.get_memory_allocator(), handle, allocation);
	}
}

VkBuffer ScratchBuffer::get_handle() const
{
	return handle;
}

uint64_t ScratchBuffer::get_device_address() const
{
	return device_address;
}
}        // namespace core
}        // namespace vkb
