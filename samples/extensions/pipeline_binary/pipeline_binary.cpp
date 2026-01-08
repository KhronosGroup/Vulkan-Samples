/* Copyright (c) 2026, Holochip Inc
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
#include <chrono>
#include <cstdio>
#include <fmt/format.h>
#include <fstream>

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

	// Check if a binary file exists from a previous run
	check_binary_file_exists();

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

		// Begin render pass to clear the screen and render the GUI
		VkClearValue clear_values[2];
		clear_values[0].color        = {{0.1f, 0.1f, 0.1f, 1.0f}};
		clear_values[1].depthStencil = {1.0f, 0};

		VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
		render_pass_begin_info.renderPass               = render_pass;
		render_pass_begin_info.framebuffer              = framebuffers[current_buffer];
		render_pass_begin_info.renderArea.extent.width  = width;
		render_pass_begin_info.renderArea.extent.height = height;
		render_pass_begin_info.clearValueCount          = 2;
		render_pass_begin_info.pClearValues             = clear_values;

		vkCmdBeginRenderPass(cmd, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		// Set viewport and scissor
		VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		vkCmdSetViewport(cmd, 0, 1, &viewport);

		VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(cmd, 0, 1, &scissor);

		// Draw the GUI overlay
		draw_ui(cmd);

		vkCmdEndRenderPass(cmd);

		VK_CHECK(vkEndCommandBuffer(cmd));

		// Wait at COLOR_ATTACHMENT_OUTPUT so rendering happens after the acquire semaphore is signaled.
		VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
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

	std::string message = fmt::format("VK_KHR_pipeline_binary support: pipelineBinaries = {}", !!features.pipelineBinaries);
	LOGI(message);
	log_text_ += message + "\n";

	message = fmt::format(
	    "VK_KHR_pipeline_binary properties: internalCache={}, internalCacheControl={}, prefersInternalCache={}, precompiledInternalCache={}, compressedData={}",
	    !!props.pipelineBinaryInternalCache,
	    !!props.pipelineBinaryInternalCacheControl,
	    !!props.pipelineBinaryPrefersInternalCache,
	    !!props.pipelineBinaryPrecompiledInternalCache,
	    !!props.pipelineBinaryCompressedData);
	LOGI(message);
	log_text_ += message + "\n";
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
		std::string message = fmt::format("vkGetPipelineKeyKHR failed ({}); skipping binary capture", vk::to_string(static_cast<vk::Result>(res)));
		LOGW(message);
		log_text_ += message + "\n";
		return;
	}

	{
		std::string message = fmt::format("Got pipeline key ({} bytes)", key.keySize);
		LOGI(message);
		log_text_ += message + "\n";
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
		std::string message = fmt::format("vkCreatePipelineBinariesKHR failed ({}); driver may not support capturing binaries in this context",
		                                  vk::to_string(static_cast<vk::Result>(res)));
		LOGW(message);
		log_text_ += message + "\n";
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
		std::string message = fmt::format("vkGetPipelineBinaryDataKHR size query failed ({}); skipping data fetch", vk::to_string(static_cast<vk::Result>(res)));
		LOGW(message);
		log_text_ += message + "\n";
		return;
	}

	binary_data_.resize(binary_size);
	binary_key_ = {VK_STRUCTURE_TYPE_PIPELINE_BINARY_KEY_KHR};
	res         = vkGetPipelineBinaryDataKHR(get_device().get_handle(), &binary_info, &binary_key_, &binary_size, binary_data_.data());
	if (res == VK_SUCCESS)
	{
		binary_size_      = binary_size;
		binary_available_ = true;
		LOGI("Retrieved pipeline binary of {} bytes; key size {} bytes", static_cast<uint32_t>(binary_size), binary_key_.keySize);
		char buf[160];
		snprintf(buf, sizeof(buf), "Retrieved pipeline binary of %u bytes; key size %u bytes\n", static_cast<unsigned>(binary_size), static_cast<unsigned>(binary_key_.keySize));
		log_text_ += buf;
		// Print a short signature so we can see it changes between runs/devices
		if (binary_size >= 4)
		{
			std::string message = fmt::format("Binary signature: {} {} {} {} ...", binary_data_[0], binary_data_[1], binary_data_[2], binary_data_[3]);
			LOGD(message);
			log_text_ += message + "\n";
		}
	}
	else
	{
		binary_available_ = false;
		LOGW("vkGetPipelineBinaryDataKHR failed ({}); data not available", static_cast<int>(res));
		std::string message = std::format("vkGetPipelineBinaryDataKHR failed ({}); data not available", vk::to_string(static_cast<vk::Result>(res)));
		LOGW(message);
		log_text_ += message + "\n";
	}
}

void PipelineBinary::recreate_pipeline_from_scratch()
{
	auto start = std::chrono::high_resolution_clock::now();

	// Destroy existing pipeline
	if (compute_pipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(get_device().get_handle(), compute_pipeline, nullptr);
		compute_pipeline = VK_NULL_HANDLE;
	}

	// Recreate pipeline from scratch
	VK_CHECK(vkCreateComputePipelines(get_device().get_handle(), pipeline_cache, 1, &compute_ci_cache, nullptr, &compute_pipeline));

	auto end             = std::chrono::high_resolution_clock::now();
	last_create_time_ms_ = std::chrono::duration<float, std::milli>(end - start).count();
	creation_count_++;

	status_message_ = "Pipeline recreated from scratch";
	LOGI("Pipeline recreated from scratch in {:.3f} ms", last_create_time_ms_);
}

void PipelineBinary::recreate_pipeline_from_binary()
{
	if (!binary_available_)
	{
		status_message_ = "Error: No binary available";
		LOGW("Cannot recreate from binary: no binary available");
		return;
	}

	auto start = std::chrono::high_resolution_clock::now();

	// Destroy existing pipeline
	if (compute_pipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(get_device().get_handle(), compute_pipeline, nullptr);
		compute_pipeline = VK_NULL_HANDLE;
	}

	// Create pipeline from binary data
	VkPipelineBinaryDataKHR binary_data_info{};
	binary_data_info.dataSize = binary_size_;
	binary_data_info.pData    = binary_data_.data();

	VkPipelineBinaryKeysAndDataKHR keys_and_data{};
	keys_and_data.binaryCount         = 1;
	keys_and_data.pPipelineBinaryKeys = &binary_key_;
	keys_and_data.pPipelineBinaryData = &binary_data_info;

	VkPipelineBinaryCreateInfoKHR create_info{VK_STRUCTURE_TYPE_PIPELINE_BINARY_CREATE_INFO_KHR};
	create_info.pKeysAndDataInfo = &keys_and_data;

	VkPipelineBinaryKHR            temp_binary = VK_NULL_HANDLE;
	VkPipelineBinaryHandlesInfoKHR handles{VK_STRUCTURE_TYPE_PIPELINE_BINARY_HANDLES_INFO_KHR};
	handles.pipelineBinaryCount = 1;
	handles.pPipelineBinaries   = &temp_binary;

	VkResult res = vkCreatePipelineBinariesKHR(get_device().get_handle(), &create_info, nullptr, &handles);
	if (res != VK_SUCCESS || temp_binary == VK_NULL_HANDLE)
	{
		status_message_ = "Error: Failed to create binary from data";
		LOGW("Failed to create pipeline binary from data: {}", static_cast<int>(res));
		return;
	}

	// Create pipeline using the binary
	VkPipelineBinaryInfoKHR binary_info{VK_STRUCTURE_TYPE_PIPELINE_BINARY_INFO_KHR};
	binary_info.binaryCount       = 1;
	binary_info.pPipelineBinaries = &temp_binary;

	VkComputePipelineCreateInfo ci = compute_ci_cache;
	ci.pNext                       = &binary_info;

	res = vkCreateComputePipelines(get_device().get_handle(), pipeline_cache, 1, &ci, nullptr, &compute_pipeline);

	vkDestroyPipelineBinaryKHR(get_device().get_handle(), temp_binary, nullptr);

	auto end                    = std::chrono::high_resolution_clock::now();
	last_binary_create_time_ms_ = std::chrono::duration<float, std::milli>(end - start).count();
	binary_creation_count_++;

	if (res == VK_SUCCESS)
	{
		status_message_ = "Pipeline recreated from binary";
		LOGI("Pipeline recreated from binary in {:.3f} ms", last_binary_create_time_ms_);
	}
	else
	{
		status_message_ = "Error: Failed to create pipeline from binary";
		LOGW("Failed to create pipeline from binary: {}", static_cast<int>(res));
	}
}

void PipelineBinary::save_binary_to_file()
{
	if (!binary_available_)
	{
		status_message_ = "Error: No binary to save";
		LOGW("Cannot save binary: no binary available");
		return;
	}

	std::ofstream file(binary_file_path_, std::ios::binary);
	if (!file)
	{
		status_message_ = "Error: Failed to open file for writing";
		LOGW("Failed to open file for writing: {}", binary_file_path_);
		return;
	}

	// Write key size and key data
	file.write(reinterpret_cast<const char *>(&binary_key_.keySize), sizeof(binary_key_.keySize));
	file.write(reinterpret_cast<const char *>(binary_key_.key), binary_key_.keySize);

	// Write binary size and binary data
	file.write(reinterpret_cast<const char *>(&binary_size_), sizeof(binary_size_));
	file.write(reinterpret_cast<const char *>(binary_data_.data()), binary_size_);

	file.close();

	binary_file_exists_ = true;
	status_message_     = "Binary saved to " + binary_file_path_;
	LOGI("Binary saved to {}", binary_file_path_);
}

void PipelineBinary::load_binary_from_file()
{
	std::ifstream file(binary_file_path_, std::ios::binary);
	if (!file)
	{
		status_message_ = "Error: Failed to open file for reading";
		LOGW("Failed to open file for reading: {}", binary_file_path_);
		return;
	}

	// Read key size and key data
	uint32_t key_size = 0;
	file.read(reinterpret_cast<char *>(&key_size), sizeof(key_size));
	if (key_size > VK_MAX_PIPELINE_BINARY_KEY_SIZE_KHR)
	{
		status_message_ = "Error: Invalid key size in file";
		LOGW("Invalid key size in file: {}", key_size);
		file.close();
		return;
	}

	binary_key_         = {VK_STRUCTURE_TYPE_PIPELINE_BINARY_KEY_KHR};
	binary_key_.keySize = key_size;
	file.read(reinterpret_cast<char *>(binary_key_.key), key_size);

	// Read binary size and binary data
	file.read(reinterpret_cast<char *>(&binary_size_), sizeof(binary_size_));
	binary_data_.resize(binary_size_);
	file.read(reinterpret_cast<char *>(binary_data_.data()), binary_size_);

	file.close();

	binary_available_ = true;
	status_message_   = "Binary loaded from " + binary_file_path_;
	LOGI("Binary loaded from {} ({} bytes)", binary_file_path_, binary_size_);
}

bool PipelineBinary::check_binary_file_exists()
{
	std::ifstream file(binary_file_path_);
	binary_file_exists_ = file.good();
	return binary_file_exists_;
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
	if (drawer.header("Pipeline Binary Info"))
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

	if (drawer.header("Interactive Demo"))
	{
		// Status message
		if (!status_message_.empty())
		{
			drawer.text("Status: %s", status_message_.c_str());
		}

		drawer.text("");        // Spacing

		// Pipeline recreation buttons
		if (drawer.button("Recreate Pipeline (from scratch)"))
		{
			recreate_pipeline_from_scratch();
		}

		if (binary_available_)
		{
			if (drawer.button("Recreate Pipeline (from binary)"))
			{
				recreate_pipeline_from_binary();
			}
		}
		else
		{
			drawer.text("(Binary not available for recreation)");
		}

		drawer.text("");        // Spacing

		// File operations
		if (binary_available_)
		{
			if (drawer.button("Save Binary to File"))
			{
				save_binary_to_file();
			}
		}
		else
		{
			drawer.text("(No binary to save)");
		}

		if (binary_file_exists_)
		{
			if (drawer.button("Load Binary from File"))
			{
				load_binary_from_file();
			}
			drawer.text("File: %s", binary_file_path_.c_str());
		}
		else
		{
			drawer.text("(No saved binary file found)");
		}
	}

	if (drawer.header("Performance Statistics"))
	{
		// Display timing information
		if (creation_count_ > 0)
		{
			drawer.text("Last creation from scratch: %.3f ms", last_create_time_ms_);
			drawer.text("Total recreations from scratch: %d", creation_count_);
		}
		else
		{
			drawer.text("No recreations from scratch yet");
		}

		drawer.text("");        // Spacing

		if (binary_creation_count_ > 0)
		{
			drawer.text("Last creation from binary: %.3f ms", last_binary_create_time_ms_);
			drawer.text("Total recreations from binary: %d", binary_creation_count_);

			// Calculate and display speedup if both methods have been used
			if (creation_count_ > 0 && last_create_time_ms_ > 0.0f && last_binary_create_time_ms_ > 0.0f)
			{
				float speedup = last_create_time_ms_ / last_binary_create_time_ms_;
				drawer.text("Speedup: %.2fx faster", speedup);
			}
		}
		else
		{
			drawer.text("No recreations from binary yet");
		}

		drawer.text("");        // Spacing

		// Binary information
		if (binary_available_)
		{
			drawer.text("Binary size: %zu bytes", binary_size_);
			drawer.text("Key size: %u bytes", binary_key_.keySize);
		}
	}
}

std::unique_ptr<vkb::Application> create_pipeline_binary()
{
	return std::make_unique<PipelineBinary>();
}
