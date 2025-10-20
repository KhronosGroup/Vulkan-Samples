/* Copyright (c) 2025, Holochip Inc.
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

#include "compute_shader_derivatives.h"

#include <cstdio>

#include "common/vk_common.h"
#include "common/vk_initializers.h"
#include "core/util/logging.hpp"

ComputeShaderDerivatives::ComputeShaderDerivatives()
{
	title = "Compute shader derivatives (VK_KHR_compute_shader_derivatives)";

	// Use Vulkan 1.2 instance so SPIR-V 1.4 modules produced by Slang are valid under validation
	set_api_version(VK_API_VERSION_1_2);

	// Needed for feature chaining
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	// Device extension providing the feature
	add_device_extension(VK_KHR_COMPUTE_SHADER_DERIVATIVES_EXTENSION_NAME);
	// Toolchains may still emit SPV_NV_compute_shader_derivatives; enable NV extension if available to satisfy validation
	add_device_extension(VK_NV_COMPUTE_SHADER_DERIVATIVES_EXTENSION_NAME, /*optional*/ true);
}

ComputeShaderDerivatives::~ComputeShaderDerivatives()
{
	if (has_device())
	{
		VkDevice device = get_device().get_handle();

		if (compute_pipeline)
		{
			vkDestroyPipeline(device, compute_pipeline, nullptr);
		}
		if (pipeline_layout)
		{
			vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
		}
		if (descriptor_pool)
		{
			vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
		}
		if (descriptor_set_layout)
		{
			vkDestroyDescriptorSetLayout(device, descriptor_set_layout, nullptr);
		}
		if (result_buffer)
		{
			vkDestroyBuffer(device, result_buffer, nullptr);
		}
		if (result_memory)
		{
			vkFreeMemory(device, result_memory, nullptr);
		}
	}
}

void ComputeShaderDerivatives::create_output_buffer_and_descriptors()
{
	auto device = get_device().get_handle();

	// Create host-visible buffer to store 16 float4 entries
	VkBufferCreateInfo buf_ci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	buf_ci.size        = result_size;
	buf_ci.usage       = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	buf_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VK_CHECK(vkCreateBuffer(device, &buf_ci, nullptr, &result_buffer));

	VkMemoryRequirements mem_req{};
	vkGetBufferMemoryRequirements(device, result_buffer, &mem_req);

	VkMemoryAllocateInfo alloc_info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	alloc_info.allocationSize  = mem_req.size;
	alloc_info.memoryTypeIndex = get_device().get_gpu().get_memory_type(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	VK_CHECK(vkAllocateMemory(device, &alloc_info, nullptr, &result_memory));
	VK_CHECK(vkBindBufferMemory(device, result_buffer, result_memory, 0));

	// Create descriptor pool for one storage buffer descriptor
	VkDescriptorPoolSize pool_size{};
	pool_size.type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	pool_size.descriptorCount = 1;

	VkDescriptorPoolCreateInfo pool_ci{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
	pool_ci.maxSets       = 1;
	pool_ci.poolSizeCount = 1;
	pool_ci.pPoolSizes    = &pool_size;
	VK_CHECK(vkCreateDescriptorPool(device, &pool_ci, nullptr, &descriptor_pool));
}

void ComputeShaderDerivatives::request_gpu_features(vkb::core::PhysicalDeviceC &gpu)
{
	// Require quads derivative group (the sample shader uses layout(derivative_group_quadsNV/derivative_group_quads_khr))
	REQUEST_REQUIRED_FEATURE(gpu, VkPhysicalDeviceComputeShaderDerivativesFeaturesKHR, computeDerivativeGroupQuads);
	// Users may switch to the linear mode by changing the shader qualifier
}

void ComputeShaderDerivatives::create_compute_pipeline()
{
	auto device = get_device().get_handle();

	// Descriptor set layout: binding 0 as storage buffer for results
	VkDescriptorSetLayoutBinding binding{};
	binding.binding            = 0;
	binding.descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	binding.descriptorCount    = 1;
	binding.stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
	binding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo set_layout_ci{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
	set_layout_ci.bindingCount = 1;
	set_layout_ci.pBindings    = &binding;
	VK_CHECK(vkCreateDescriptorSetLayout(device, &set_layout_ci, nullptr, &descriptor_set_layout));

	// Pipeline layout uses the descriptor set layout at set 0
	VkPipelineLayoutCreateInfo layout_ci{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
	layout_ci.setLayoutCount = 1;
	layout_ci.pSetLayouts    = &descriptor_set_layout;
	VK_CHECK(vkCreatePipelineLayout(device, &layout_ci, nullptr, &pipeline_layout));

	// Allocate and update descriptor set now that layout exists
	VkDescriptorSetAllocateInfo alloc_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
	alloc_info.descriptorPool     = descriptor_pool;
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts        = &descriptor_set_layout;
	VK_CHECK(vkAllocateDescriptorSets(device, &alloc_info, &descriptor_set));

	VkDescriptorBufferInfo buffer_info{};
	buffer_info.buffer = result_buffer;
	buffer_info.offset = 0;
	buffer_info.range  = result_size;

	VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
	write.dstSet          = descriptor_set;
	write.dstBinding      = 0;
	write.dstArrayElement = 0;
	write.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	write.descriptorCount = 1;
	write.pBufferInfo     = &buffer_info;
	vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

	// Load compute shader explicitly from slang path to ensure SPV_KHR_compute_shader_derivatives is used
	VkPipelineShaderStageCreateInfo stage = load_shader("compute_shader_derivatives/slang/derivatives.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);

	VkComputePipelineCreateInfo compute_ci{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
	compute_ci.stage  = stage;
	compute_ci.layout = pipeline_layout;
	VK_CHECK(vkCreateComputePipelines(device, pipeline_cache, 1, &compute_ci, nullptr, &compute_pipeline));
}

bool ComputeShaderDerivatives::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	// Create buffer + descriptors first, then the pipeline/layout that reference the set layout
	create_output_buffer_and_descriptors();
	create_compute_pipeline();

	prepared = true;
	return true;
}

void ComputeShaderDerivatives::build_command_buffers()
{
	// Not used; this sample records per-frame in render()
}

void ComputeShaderDerivatives::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}

	// Acquire swapchain image and signal acquired_image_ready
	prepare_frame();

	// Recreate and record the current frame's command buffer
	recreate_current_command_buffer();
	VkCommandBuffer cmd = draw_cmd_buffers[current_buffer];

	VkCommandBufferBeginInfo begin_info = vkb::initializers::command_buffer_begin_info();
	VK_CHECK(vkBeginCommandBuffer(cmd, &begin_info));

	// Dispatch a single workgroup; shader has local_size 4x4
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);
	vkCmdDispatch(cmd, 1, 1, 1);

	// Transition the acquired swapchain image to PRESENT so presentation is valid
	VkImageSubresourceRange range{};
	range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	range.baseMipLevel   = 0;
	range.levelCount     = 1;
	range.baseArrayLayer = 0;
	range.layerCount     = 1;
	vkb::image_layout_transition(cmd,
	                             swapchain_buffers[current_buffer].image,
	                             VK_IMAGE_LAYOUT_UNDEFINED,
	                             VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	                             range);

	VK_CHECK(vkEndCommandBuffer(cmd));

	// Submit: wait on acquire semaphore, signal render_complete for present
	VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkSubmitInfo         submit_info{};
	submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.waitSemaphoreCount   = 1;
	submit_info.pWaitSemaphores      = &semaphores.acquired_image_ready;
	submit_info.pWaitDstStageMask    = &wait_stage;
	submit_info.commandBufferCount   = 1;
	submit_info.pCommandBuffers      = &cmd;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores    = &semaphores.render_complete;

	{
		auto   &queue = get_device().get_queue_by_present(0);
		VkQueue q     = static_cast<VkQueue>(queue.get_handle());
		VK_CHECK(vkQueueSubmit(q, 1, &submit_info, VK_NULL_HANDLE));

		// One-time readback and collect results after the compute dispatch completes
		if (!printed_once)
		{
			VK_CHECK(vkQueueWaitIdle(q));
			void *mapped = nullptr;
			VK_CHECK(vkMapMemory(get_device().get_handle(), result_memory, 0, result_size, 0, &mapped));
			float *data = static_cast<float *>(mapped);
			log_text_.clear();
			// Each entry is float4: v, ddx, ddy, pad
			for (uint32_t y = 0; y < 4; ++y)
			{
				for (uint32_t x = 0; x < 4; ++x)
				{
					uint32_t idx = y * 4 + x;
					float    v   = data[idx * 4 + 0];
					float    ddx = data[idx * 4 + 1];
					float    ddy = data[idx * 4 + 2];
					// Store as human-readable line for GUI and also keep LOGI for debug environments
					char buf[160];
					snprintf(buf, sizeof(buf), "tid=(%u,%u) v=%.6f ddx=%.6f ddy=%.6f", x, y, v, ddx, ddy);
					log_text_ += buf;
					log_text_ += '\n';
					LOGI("compute-derivatives CPU: {}", buf);
				}
			}
			vkUnmapMemory(get_device().get_handle(), result_memory);
			printed_once = true;
		}
	}

	// Present (waits on render_complete)
	submit_frame();
}

void ComputeShaderDerivatives::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Compute shader derivatives"))
	{
		if (log_text_.empty())
		{
			drawer.text("Results will be shown here after first dispatch.");
		}
		else
		{
			drawer.text("%s", log_text_.c_str());
		}
	}
}

std::unique_ptr<vkb::Application> create_compute_shader_derivatives()
{
	return std::make_unique<ComputeShaderDerivatives>();
}
