/* Copyright (c) 2025, Holochip Inc
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

#include "pipeline_binary.h"

#include "common/vk_common.h"
#include "common/vk_initializers.h"
#include "core/device.h"
#include "core/util/logging.hpp"
#include <cstdio>

PipelineBinary::PipelineBinary()
{
	title = "Pipeline binary (VK_KHR_pipeline_binary)";

	// We need the properties2 instance extension to request chained extension features
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

	// Enable the device extension required by this sample
	add_device_extension(VK_KHR_PIPELINE_BINARY_EXTENSION_NAME);
	add_device_extension(VK_KHR_MAINTENANCE_5_EXTENSION_NAME);
	add_device_extension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
	add_device_extension(VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME);
	add_device_extension(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);
}

PipelineBinary::~PipelineBinary()
{
	if (has_device())
	{
		if (compute_pipeline)
		{
			vkDestroyPipeline(get_device().get_handle(), compute_pipeline, nullptr);
		}
		if (pipeline_layout)
		{
			vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);
		}
		if (pipeline_binary)
		{
			vkDestroyPipelineBinaryKHR(get_device().get_handle(), pipeline_binary, nullptr);
		}
	}
}

void PipelineBinary::request_gpu_features(vkb::core::PhysicalDeviceC &gpu)
{
	// Enable the pipeline binary feature using the framework's feature chaining
	REQUEST_REQUIRED_FEATURE(gpu, VkPhysicalDevicePipelineBinaryFeaturesKHR, pipelineBinaries);
}

bool PipelineBinary::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	// Create a very small compute pipeline used to demonstrate the extension APIs
	create_compute_pipeline();

	// Log support status and device properties related to pipeline binaries
	log_pipeline_binary_support();

	// Demonstrate querying a key and (optionally) getting a pipeline binary
	demo_pipeline_key_and_binary();

	prepared = true;
	return true;
}

void PipelineBinary::render(float /*delta_time*/)
{
	if (!prepared)
	{
		return;
	}

	prepare_frame();

	if (get_render_context().has_swapchain())
	{
		recreate_current_command_buffer();
		VkCommandBuffer cmd = draw_cmd_buffers[current_buffer];

		VkCommandBufferBeginInfo begin_info = vkb::initializers::command_buffer_begin_info();
		VK_CHECK(vkBeginCommandBuffer(cmd, &begin_info));

		VkImageSubresourceRange subresource_range{};
		subresource_range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		subresource_range.baseMipLevel   = 0;
		subresource_range.levelCount     = 1;
		subresource_range.baseArrayLayer = 0;
		subresource_range.layerCount     = 1;

		// Transition the acquired image from UNDEFINED to PRESENT for this frame.
		vkb::image_layout_transition(cmd, swapchain_buffers[current_buffer].image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, subresource_range);

		VK_CHECK(vkEndCommandBuffer(cmd));

		// Wait at TOP_OF_PIPE so the command buffer executes only after the acquire semaphore is signaled.
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

		// Use the graphics queue for this lightweight submission.
		auto graphics_queue = get_device().get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0).get_handle();
		VK_CHECK(vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE));
	}

	submit_frame();
}

void PipelineBinary::create_compute_pipeline()
{
	// Empty pipeline layout (no descriptors / push constants)
	VkPipelineLayoutCreateInfo pipeline_layout_create_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout));

	// Load and cache the compute shader stage once for reuse
	compute_shader_stage = load_shader("pipeline_binary/glsl/binary_demo.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);
	compute_shader       = compute_shader_stage.module;

	// Cache the compute pipeline create info for reuse by the pipeline binary demo
	compute_ci_cache        = {VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
	compute_ci_cache.stage  = compute_shader_stage;
	compute_ci_cache.layout = pipeline_layout;

	// Ensure we have a pipeline cache, as some drivers may expect one
	if (pipeline_cache == VK_NULL_HANDLE)
	{
		create_pipeline_cache();
	}

	VK_CHECK(vkCreateComputePipelines(get_device().get_handle(), pipeline_cache, 1, &compute_ci_cache, nullptr, &compute_pipeline));
}

void PipelineBinary::log_pipeline_binary_support()
{
	VkPhysicalDevicePipelineBinaryFeaturesKHR   features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_BINARY_FEATURES_KHR};
	VkPhysicalDevicePipelineBinaryPropertiesKHR props{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_BINARY_PROPERTIES_KHR};

	VkPhysicalDeviceFeatures2 features2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
	features2.pNext = &features;

	VkPhysicalDeviceProperties2 props2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
	props2.pNext = &props;

	vkGetPhysicalDeviceFeatures2(get_device().get_gpu().get_handle(), &features2);
	vkGetPhysicalDeviceProperties2(get_device().get_gpu().get_handle(), &props2);

	const char *pb = features.pipelineBinaries ? "true" : "false";
	LOGI("VK_KHR_pipeline_binary supported feature: pipelineBinaries = {}", pb);
	char buf[256];
	snprintf(buf, sizeof(buf), "VK_KHR_pipeline_binary supported feature: pipelineBinaries = %s\n", pb);
	log_text_ += buf;

	const char *ic   = props.pipelineBinaryInternalCache ? "true" : "false";
	const char *icc  = props.pipelineBinaryInternalCacheControl ? "true" : "false";
	const char *pic  = props.pipelineBinaryPrefersInternalCache ? "true" : "false";
	const char *pic2 = props.pipelineBinaryPrecompiledInternalCache ? "true" : "false";
	const char *cd   = props.pipelineBinaryCompressedData ? "true" : "false";
	LOGI("VK_KHR_pipeline_binary properties: internalCache={}, internalCacheControl={}, prefersInternalCache={}, precompiledInternalCache={}, compressedData={}", ic, icc, pic, pic2, cd);
	snprintf(buf, sizeof(buf), "VK_KHR_pipeline_binary properties: internalCache=%s, internalCacheControl=%s, prefersInternalCache=%s, precompiledInternalCache=%s, compressedData=%s\n", ic, icc, pic, pic2, cd);
	log_text_ += buf;
}

void PipelineBinary::demo_pipeline_key_and_binary()
{
	// Reuse the cached compute pipeline create info we prepared up front
	VkComputePipelineCreateInfo *compute_ci_ptr = &compute_ci_cache;

	// Wrap our pipeline create info in the KHR generic create info struct
	VkPipelineCreateInfoKHR pipeline_create_info_khr{VK_STRUCTURE_TYPE_PIPELINE_CREATE_INFO_KHR};
	pipeline_create_info_khr.pNext = compute_ci_ptr;

	// Query a pipeline key for these creation parameters
	VkPipelineBinaryKeyKHR key{VK_STRUCTURE_TYPE_PIPELINE_BINARY_KEY_KHR};
	VkResult               res = vkGetPipelineKeyKHR(get_device().get_handle(), &pipeline_create_info_khr, &key);
	if (res != VK_SUCCESS)
	{
		LOGW("vkGetPipelineKeyKHR failed ({}); skipping binary capture", static_cast<int>(res));
		char buf[128];
		snprintf(buf, sizeof(buf), "vkGetPipelineKeyKHR failed (%d); skipping binary capture\n", static_cast<int>(res));
		log_text_ += buf;
		return;
	}

	LOGI("Got pipeline key ({} bytes)", key.keySize);
	{
		char buf[128];
		snprintf(buf, sizeof(buf), "Got pipeline key (%u bytes)\n", static_cast<unsigned>(key.keySize));
		log_text_ += buf;
	}

	// Create a pipeline binary handle from the pipeline creation parameters only
	VkPipelineBinaryHandlesInfoKHR handles{VK_STRUCTURE_TYPE_PIPELINE_BINARY_HANDLES_INFO_KHR};
	handles.pipelineBinaryCount = 1;
	handles.pPipelineBinaries   = &pipeline_binary;

	VkPipelineBinaryCreateInfoKHR create_info{VK_STRUCTURE_TYPE_PIPELINE_BINARY_CREATE_INFO_KHR};
	create_info.pipeline            = VK_NULL_HANDLE;                   // Using pPipelineCreateInfo path; no capture flag required on a pipeline object
	create_info.pPipelineCreateInfo = &pipeline_create_info_khr;        // Only one of the three must be non-NULL
	create_info.pNext               = nullptr;
	create_info.pKeysAndDataInfo    = nullptr;

	res = vkCreatePipelineBinariesKHR(get_device().get_handle(), &create_info, nullptr, &handles);
	if (res != VK_SUCCESS || pipeline_binary == VK_NULL_HANDLE)
	{
		LOGW("vkCreatePipelineBinariesKHR failed ({}); driver may not support capturing binaries in this context", static_cast<int>(res));
		char buf[180];
		snprintf(buf, sizeof(buf), "vkCreatePipelineBinariesKHR failed (%d); driver may not support capturing binaries in this context\n", static_cast<int>(res));
		log_text_ += buf;
		return;
	}

	// Query the size first (spec requires a valid pPipelineBinaryKey pointer)
	size_t                      binary_size = 0;
	VkPipelineBinaryDataInfoKHR binary_info{VK_STRUCTURE_TYPE_PIPELINE_BINARY_DATA_INFO_KHR};
	binary_info.pipelineBinary = pipeline_binary;

	VkPipelineBinaryKeyKHR size_query_key{VK_STRUCTURE_TYPE_PIPELINE_BINARY_KEY_KHR};
	res = vkGetPipelineBinaryDataKHR(get_device().get_handle(), &binary_info, &size_query_key, &binary_size, nullptr);
	if (res != VK_SUCCESS || binary_size == 0)
	{
		LOGW("vkGetPipelineBinaryDataKHR size query failed ({}); skipping data fetch", static_cast<int>(res));
		char buf[160];
		snprintf(buf, sizeof(buf), "vkGetPipelineBinaryDataKHR size query failed (%d); skipping data fetch\n", static_cast<int>(res));
		log_text_ += buf;
		return;
	}

	std::vector<uint8_t>   data(binary_size);
	VkPipelineBinaryKeyKHR key2{VK_STRUCTURE_TYPE_PIPELINE_BINARY_KEY_KHR};
	res = vkGetPipelineBinaryDataKHR(get_device().get_handle(), &binary_info, &key2, &binary_size, data.data());
	if (res == VK_SUCCESS)
	{
		LOGI("Retrieved pipeline binary of {} bytes; key size {} bytes", static_cast<uint32_t>(binary_size), key2.keySize);
		char buf[160];
		snprintf(buf, sizeof(buf), "Retrieved pipeline binary of %u bytes; key size %u bytes\n", static_cast<unsigned>(binary_size), static_cast<unsigned>(key2.keySize));
		log_text_ += buf;
		// Print a short signature so we can see it changes between runs/devices
		if (binary_size >= 4)
		{
			char sig[96];
			snprintf(sig, sizeof(sig), "Binary signature: %u %u %u %u ...\n", data[0], data[1], data[2], data[3]);
			log_text_ += sig;
			LOGD("Binary signature: {} {} {} {} ...", data[0], data[1], data[2], data[3]);
		}
	}
	else
	{
		LOGW("vkGetPipelineBinaryDataKHR failed ({}); data not available", static_cast<int>(res));
		char buf[128];
		snprintf(buf, sizeof(buf), "vkGetPipelineBinaryDataKHR failed (%d); data not available\n", static_cast<int>(res));
		log_text_ += buf;
	}
}

void PipelineBinary::build_command_buffers()
{
	VkCommandBufferBeginInfo begin_info = vkb::initializers::command_buffer_begin_info();

	for (size_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &begin_info));

		VkImageSubresourceRange subresource_range{};
		subresource_range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		subresource_range.baseMipLevel   = 0;
		subresource_range.levelCount     = 1;
		subresource_range.baseArrayLayer = 0;
		subresource_range.layerCount     = 1;
		vkb::image_layout_transition(draw_cmd_buffers[i], swapchain_buffers[i].image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, subresource_range);

		vkEndCommandBuffer(draw_cmd_buffers[i]);
	}
}

void PipelineBinary::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Pipeline binary"))
	{
		if (!log_text_.empty())
		{
			drawer.text("%s", log_text_.c_str());
		}
		else
		{
			drawer.text("Collecting pipeline binary info...");
		}
	}
}

std::unique_ptr<vkb::Application> create_pipeline_binary()
{
	return std::make_unique<PipelineBinary>();
}
