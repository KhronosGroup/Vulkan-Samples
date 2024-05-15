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

#include "hpp_buffer.h"

#include "hpp_device.h"

namespace vkb
{
namespace core
{

HPPBuffer HPPBufferBuilder::build(HPPDevice &device) const
{
	return HPPBuffer{device, *this};
}

HPPBufferPtr HPPBufferBuilder::build_unique(HPPDevice &device) const
{
	return std::make_unique<HPPBuffer>(device, *this);
}

HPPBuffer HPPBuffer::create_staging_buffer(HPPDevice &device, vk::DeviceSize size, const void *data)
{
	HPPBuffer staging_buffer = HPPBufferBuilder{size}
	                               .with_usage(vk::BufferUsageFlagBits::eTransferSrc)
	                               .with_vma_flags(VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT)
	                               .build(device);
	if (data != nullptr)
	{
		staging_buffer.update(static_cast<const uint8_t *>(data), size);
	}
	return staging_buffer;
}

HPPBuffer::HPPBuffer(vkb::core::HPPDevice        &device,
                     vk::DeviceSize               size_,
                     vk::BufferUsageFlags         buffer_usage,
                     VmaMemoryUsage               memory_usage,
                     VmaAllocationCreateFlags     flags,
                     const std::vector<uint32_t> &queue_family_indices) :
    HPPBuffer(device,
              HPPBufferBuilder{size_}
                  .with_usage(buffer_usage)
                  .with_vma_flags(flags)
                  .with_vma_usage(memory_usage)
                  .with_queue_families(queue_family_indices)
                  .with_implicit_sharing_mode())
{}

HPPBuffer::HPPBuffer(
    vkb::core::HPPDevice   &device,
    HPPBufferBuilder const &builder) :
    Parent{builder.alloc_create_info, nullptr, &device}, size(builder.create_info.size)
{
	get_handle() = create_buffer(builder.create_info.operator const VkBufferCreateInfo &());
	if (!builder.debug_name.empty())
	{
		set_debug_name(builder.debug_name);
	}
}

HPPBuffer::HPPBuffer(HPPBuffer &&other) noexcept :
    HPPAllocated{static_cast<HPPAllocated &&>(other)},
    size(std::exchange(other.size, {}))
{}

HPPBuffer::~HPPBuffer()
{
	destroy_buffer(get_handle());
}

vk::DeviceSize HPPBuffer::get_size() const
{
	return size;
}

uint64_t HPPBuffer::get_device_address() const
{
	return get_device().get_handle().getBufferAddressKHR({get_handle()});
}

}        // namespace core
}        // namespace vkb
