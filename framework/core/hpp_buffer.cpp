/* Copyright (c) 2021, NVIDIA CORPORATION. All rights reserved.
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

#include <core/hpp_buffer.h>

namespace vkb
{
namespace core
{
HPPBuffer::HPPBuffer(vkb::core::HPPDevice &   device,
                     vk::DeviceSize           size,
                     vk::BufferUsageFlags     buffer_usage,
                     VmaMemoryUsage           memory_usage,
                     VmaAllocationCreateFlags flags) :
    Buffer(*reinterpret_cast<vkb::Device *>(&device),
           static_cast<VkDeviceSize>(size),
           static_cast<VkBufferUsageFlags>(buffer_usage),
           memory_usage,
           flags)
{}

void HPPBuffer::flush() const
{
	vkb::core::Buffer::flush();
}

vk::Buffer HPPBuffer::get_handle() const
{
	return static_cast<vk::Buffer>(vkb::core::Buffer::get_handle());
}

vk::DeviceSize HPPBuffer::get_size() const
{
	return static_cast<vk::DeviceSize>(vkb::core::Buffer::get_size());
}

void HPPBuffer::update(uint8_t const *data, size_t size, size_t offset)
{
	vkb::core::Buffer::update(data, size, offset);
}

void HPPBuffer::update(void *data, size_t size, size_t offset)
{
	vkb::core::Buffer::update(data, size, offset);
}

}        // namespace core
}        // namespace vkb
