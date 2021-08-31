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

#include "shader_binding_table.h"

#include "device.h"

namespace vkb
{
namespace core
{
ShaderBindingTable::ShaderBindingTable(Device &       device,
                                       uint32_t       handle_count,
                                       VkDeviceSize   handle_size_aligned,
                                       VmaMemoryUsage memory_usage) :
    device{device}
{
	VkBufferCreateInfo buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	buffer_info.usage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	buffer_info.size  = handle_count * handle_size_aligned;

	VmaAllocationCreateInfo memory_info{};
	memory_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	memory_info.usage = memory_usage;

	VmaAllocationInfo allocation_info{};
	auto              result = vmaCreateBuffer(device.get_memory_allocator(),
                                  &buffer_info, &memory_info,
                                  &handle, &allocation,
                                  &allocation_info);

	if (result != VK_SUCCESS)
	{
		throw VulkanException{result, "Could not create ShaderBindingTable"};
	}

	VkBufferDeviceAddressInfoKHR buffer_device_address_info{};
	buffer_device_address_info.sType            = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	buffer_device_address_info.buffer           = handle;
	strided_device_address_region.deviceAddress = vkGetBufferDeviceAddressKHR(device.get_handle(), &buffer_device_address_info);
	strided_device_address_region.stride        = handle_size_aligned;
	strided_device_address_region.size          = handle_count * handle_size_aligned;

	mapped_data = static_cast<uint8_t *>(allocation_info.pMappedData);
}

ShaderBindingTable::~ShaderBindingTable()
{
	if (handle != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE)
	{
		vmaDestroyBuffer(device.get_memory_allocator(), handle, allocation);
	}
}

const VkStridedDeviceAddressRegionKHR *vkb::core::ShaderBindingTable::get_strided_device_address_region() const
{
	return &strided_device_address_region;
}
uint8_t *ShaderBindingTable::get_data() const
{
	return mapped_data;
}
}        // namespace core
}        // namespace vkb
