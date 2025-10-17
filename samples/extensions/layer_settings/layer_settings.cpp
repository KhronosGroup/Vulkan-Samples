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

#include "layer_settings.h"

#include "common/vk_common.h"
#include "common/vk_initializers.h"

#include <array>

LayerSettingsSample::LayerSettingsSample()
{
	title = "Layer settings (VK_EXT_layer_settings)";

	// Configure the Khronos validation layer using layer settings. These settings are
	// consumed by the validation layer at instance creation time.
	//
	// Note: The settings only take effect if the layer is enabled (e.g. by building
	// with validation layers on, or enabling them via the application's options).
	//
	// 1) Enable Best Practices (generic + vendor-specific)
	{
		static const char *enables[] = {
		    "VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT",
		};

		VkLayerSettingEXT layer_setting{};
		layer_setting.pLayerName   = "VK_LAYER_KHRONOS_validation";
		layer_setting.pSettingName = "enables";
		layer_setting.type         = VK_LAYER_SETTING_TYPE_STRING_EXT;
		layer_setting.valueCount   = static_cast<uint32_t>(std::size(enables));
		layer_setting.pValues      = enables;
		add_layer_setting(layer_setting);
	}

	// 2) Optionally enable debug printf so shaders using debugPrintfEXT will print via VVL
	{
		static const char *enables[] = {
		    "VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT",
		};

		VkLayerSettingEXT layer_setting{};
		layer_setting.pLayerName   = "VK_LAYER_KHRONOS_validation";
		layer_setting.pSettingName = "enables";
		layer_setting.type         = VK_LAYER_SETTING_TYPE_STRING_EXT;
		layer_setting.valueCount   = static_cast<uint32_t>(std::size(enables));
		layer_setting.pValues      = enables;
		add_layer_setting(layer_setting);
	}

	// 3) Demonstrate disabling a known verbose message category (example)
	//    This shows how to filter messages with the "disables" setting.
	//    Replace with a concrete message enable/disable as needed in your environment.
	{
		static const char *disables[] = {
		    // Example: Treat performance warnings as disabled (users can customize)
		    "VK_VALIDATION_FEATURE_DISABLE_ALL_EXT"        // users can comment this out; kept as illustration
		};

		VkLayerSettingEXT layer_setting{};
		layer_setting.pLayerName   = "VK_LAYER_KHRONOS_validation";
		layer_setting.pSettingName = "disables";
		layer_setting.type         = VK_LAYER_SETTING_TYPE_STRING_EXT;
		layer_setting.valueCount   = static_cast<uint32_t>(std::size(disables));
		layer_setting.pValues      = disables;
		// Do not add this by default as it disables all validation. Leave as commented example.
		// add_layer_setting(layer_setting);
	}
}

bool LayerSettingsSample::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	// Build once; we record per-frame minimal CBs in render().
	build_command_buffers();

	prepared = true;
	return true;
}

void LayerSettingsSample::record_minimal_present_cmd(VkCommandBuffer cmd, uint32_t image_index)
{
	VkCommandBufferBeginInfo begin_info = vkb::initializers::command_buffer_begin_info();
	VK_CHECK(vkBeginCommandBuffer(cmd, &begin_info));

	// Transition the acquired swapchain image to PRESENT so vkQueuePresentKHR is valid.
	VkImageSubresourceRange subresource_range{};
	subresource_range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource_range.baseMipLevel   = 0;
	subresource_range.levelCount     = 1;
	subresource_range.baseArrayLayer = 0;
	subresource_range.layerCount     = 1;

	vkb::image_layout_transition(cmd,
	                             swapchain_buffers[image_index].image,
	                             VK_IMAGE_LAYOUT_UNDEFINED,
	                             VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	                             subresource_range);

	// Note: UI rendering requires an active render pass. This minimal sample
	// does not start one, so we intentionally skip draw_ui(cmd) to avoid draw calls
	// outside a render pass.
	VK_CHECK(vkEndCommandBuffer(cmd));
}

void LayerSettingsSample::render(float /*delta_time*/)
{
	if (!prepared)
	{
		return;
	}

	prepare_frame();

	// Recreate and record the command buffer for the current swapchain image
	recreate_current_command_buffer();
	auto &cmd = draw_cmd_buffers[current_buffer];
	record_minimal_present_cmd(cmd, current_buffer);

	// Submit: wait on the acquire semaphore and signal render_complete for present
	VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

	VkSubmitInfo submit_info{};
	submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.waitSemaphoreCount   = 1;
	submit_info.pWaitSemaphores      = &semaphores.acquired_image_ready;
	submit_info.pWaitDstStageMask    = &wait_stage;
	submit_info.commandBufferCount   = 1;
	submit_info.pCommandBuffers      = &cmd;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores    = &semaphores.render_complete;

	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

	submit_frame();
}

std::unique_ptr<vkb::Application> create_layer_settings()
{
	return std::make_unique<LayerSettingsSample>();
}
