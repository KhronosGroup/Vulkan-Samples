/* Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.
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

#include <core/hpp_command_buffer.h>
#include <rendering/hpp_render_frame.h>

namespace vkb
{
namespace core
{
class HPPDevice;

class HPPCommandPool
{
  public:
	HPPCommandPool(HPPDevice const &                     device,
	               uint32_t                              queue_family_index,
	               vkb::rendering::HPPRenderFrame const *render_frame = nullptr,
	               size_t                                thread_index = 0,
	               HPPCommandBuffer::ResetMode           reset_mode   = HPPCommandBuffer::ResetMode::ResetPool);
	HPPCommandPool(const HPPCommandPool &) = delete;
	HPPCommandPool(HPPCommandPool &&other);
	~HPPCommandPool();

	HPPCommandPool &operator=(const HPPCommandPool &) = delete;
	HPPCommandPool &operator=(HPPCommandPool &&) = delete;

	HPPDevice const &                     get_device() const;
	vk::CommandPool                       get_handle() const;
	uint32_t                              get_queue_family_index() const;
	vkb::rendering::HPPRenderFrame const *get_render_frame() const;
	HPPCommandBuffer::ResetMode           get_reset_mode() const;
	size_t                                get_thread_index() const;
	HPPCommandBuffer const &              request_command_buffer(vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary);
	void                                  reset_pool();

  private:
	void reset_command_buffers();

  private:
	HPPDevice const &                              device;
	vk::CommandPool                                handle             = nullptr;
	vkb::rendering::HPPRenderFrame const *         render_frame       = nullptr;
	size_t                                         thread_index       = 0;
	uint32_t                                       queue_family_index = 0;
	std::vector<std::unique_ptr<HPPCommandBuffer>> primary_command_buffers;
	uint32_t                                       active_primary_command_buffer_count = 0;
	std::vector<std::unique_ptr<HPPCommandBuffer>> secondary_command_buffers;
	uint32_t                                       active_secondary_command_buffer_count = 0;
	HPPCommandBuffer::ResetMode                    reset_mode                            = HPPCommandBuffer::ResetMode::ResetPool;
};
}        // namespace core
}        // namespace vkb
