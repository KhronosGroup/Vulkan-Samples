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

#pragma once

#include <api_vulkan_sample.h>
#include <core/hpp_device.h>

/**
 * @brief wrapper class for use with vulkan.hpp in the samples of this framework
 *
 * See vkb::VulkanSample for documentation
 */
class HPPApiVulkanSample : public ApiVulkanSample
{
  public:
	bool prepare(vkb::Platform &platform) override;

	vk::CommandBuffer                 get_command_buffer(size_t index) const;
	size_t                            get_command_buffers_count() const;
	vk::Device                        get_device() const;
	vk::ClearColorValue const &       get_default_clear_color() const;
	vk::DescriptorPool                get_descriptor_pool() const;
	vk::Framebuffer                   get_framebuffer(size_t index) const;
	vk::Instance                      get_instance() const;
	vk::PhysicalDevice                get_physical_device() const;
	vk::Queue                         get_queue() const;
	vk::PipelineShaderStageCreateInfo load_shader(const std::string &file, vk::ShaderStageFlagBits stage);
	vk::SubmitInfo const &            set_submit_info(vk::CommandBuffer const &command_buffer);

	vkb::core::HPPDevice &get_device_wrapper();
};
