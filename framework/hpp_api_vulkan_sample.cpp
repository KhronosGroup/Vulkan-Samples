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

#include <hpp_api_vulkan_sample.h>

// Instantiate the default dispatcher
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

bool HPPApiVulkanSample::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

	// initialize function pointers for C++-bindings
	static vk::DynamicLoader dl;
	VULKAN_HPP_DEFAULT_DISPATCHER.init(dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));
	VULKAN_HPP_DEFAULT_DISPATCHER.init(get_instance());
	VULKAN_HPP_DEFAULT_DISPATCHER.init(get_device());

	return true;
}

vk::CommandBuffer HPPApiVulkanSample::get_command_buffer(size_t index) const
{
	return static_cast<vk::CommandBuffer>(draw_cmd_buffers[index]);
}

size_t HPPApiVulkanSample::get_command_buffers_count() const
{
	return draw_cmd_buffers.size();
}

vk::Device HPPApiVulkanSample::get_device() const
{
	return static_cast<vk::Device>(device->get_handle());
}

vk::ClearColorValue const &HPPApiVulkanSample::get_default_clear_color() const
{
	return *reinterpret_cast<vk::ClearColorValue const *>(&default_clear_color);
}

vk::DescriptorPool HPPApiVulkanSample::get_descriptor_pool() const
{
	return static_cast<vk::DescriptorPool>(descriptor_pool);
}

vk::Framebuffer HPPApiVulkanSample::get_framebuffer(size_t index) const
{
	return static_cast<vk::Framebuffer>(framebuffers[index]);
}

vk::Instance HPPApiVulkanSample::get_instance() const
{
	return static_cast<vk::Instance>(instance->get_handle());
}

vk::PhysicalDevice HPPApiVulkanSample::get_physical_device() const
{
	return static_cast<vk::PhysicalDevice>(device->get_gpu().get_handle());
}

vk::Queue HPPApiVulkanSample::get_queue() const
{
	return static_cast<vk::Queue>(queue);
}

vk::PipelineShaderStageCreateInfo HPPApiVulkanSample::load_shader(const std::string &file, vk::ShaderStageFlagBits stage)
{
	return ApiVulkanSample::load_shader(file, static_cast<VkShaderStageFlagBits>(stage));
}

vk::SubmitInfo const &HPPApiVulkanSample::set_submit_info(vk::CommandBuffer const &command_buffer)
{
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = reinterpret_cast<VkCommandBuffer const *>(&command_buffer);
	return *reinterpret_cast<vk::SubmitInfo const *>(&submit_info);
}

vkb::core::HPPDevice &HPPApiVulkanSample::get_device_wrapper()
{
	return reinterpret_cast<vkb::core::HPPDevice &>(*device);
}
