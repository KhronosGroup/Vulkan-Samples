/* Copyright (c) 2019-2024, Arm Limited and Contributors
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

#include "buffer.h"

#include "device.h"

namespace vkb
{
namespace core
{

Buffer BufferBuilder::build(Device &device) const
{
	return Buffer(device, *this);
}

BufferPtr BufferBuilder::build_unique(Device &device) const
{
	return std::make_unique<Buffer>(device, *this);
}

Buffer Buffer::create_staging_buffer(Device &device, VkDeviceSize size, const void *data)
{
	BufferBuilder builder{size};
	builder.with_vma_flags(VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
	builder.with_usage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	Buffer result(device, builder);
	if (data != nullptr)
	{
		result.update(data, size);
	}
	return result;
}

Buffer::Buffer(vkb::Device                 &device,
               VkDeviceSize                 size,
               VkBufferUsageFlags           buffer_usage,
               VmaMemoryUsage               memory_usage,
               VmaAllocationCreateFlags     flags,
               const std::vector<uint32_t> &queue_family_indices) :
    Buffer(device,
           BufferBuilder(size)
               .with_usage(buffer_usage)
               .with_vma_usage(memory_usage)
               .with_vma_flags(flags)
               .with_queue_families(queue_family_indices)
               .with_implicit_sharing_mode())
{}

Buffer::Buffer(Device &device, const BufferBuilder &builder) :
    Allocated{builder.alloc_create_info, VK_NULL_HANDLE, &device}, size(builder.create_info.size)
{
	set_handle(create_buffer(builder.create_info));
	if (!builder.debug_name.empty())
	{
		set_debug_name(builder.debug_name);
	}
}

Buffer::Buffer(Buffer &&other) noexcept :
    Allocated{std::move(other)},
    size{std::exchange(other.size, {})}
{
}

Buffer::~Buffer()
{
	destroy_buffer(get_handle());
}

VkDeviceSize Buffer::get_size() const
{
	return size;
}

uint64_t Buffer::get_device_address()
{
	VkBufferDeviceAddressInfoKHR buffer_device_address_info{};
	buffer_device_address_info.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	buffer_device_address_info.buffer = get_handle();
	return vkGetBufferDeviceAddressKHR(get_device().get_handle(), &buffer_device_address_info);
}

}        // namespace core
}        // namespace vkb
