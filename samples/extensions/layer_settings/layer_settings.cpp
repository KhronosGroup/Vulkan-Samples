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

LayerSettingsSample::~LayerSettingsSample()
{
	if (debug_messenger_ != VK_NULL_HANDLE && has_instance())
	{
		VkInstance instance = get_instance().get_handle();
		auto       fpDestroy = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
		    vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
		if (fpDestroy)
		{
			fpDestroy(instance, debug_messenger_, nullptr);
		}
		debug_messenger_ = VK_NULL_HANDLE;
	}
}

std::unique_ptr<vkb::core::InstanceC> LayerSettingsSample::create_instance()
{
	// Create the instance via base implementation first.
	auto inst = ApiVulkanSample::create_instance();

	// Create our debug messenger as early as possible so we catch messages emitted
	// during device creation and initial resource setup.
	if (inst && inst->is_enabled(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
	{
		VkDebugUtilsMessengerCreateInfoEXT info{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
		info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		                  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		                  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		info.pfnUserCallback = &LayerSettingsSample::debug_callback;
		info.pUserData       = this;

		VkInstance instance = inst->get_handle();
		auto       fpCreate = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
		    vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
		if (fpCreate && debug_messenger_ == VK_NULL_HANDLE)
		{
			VkResult res = fpCreate(instance, &info, nullptr, &debug_messenger_);
			if (res != VK_SUCCESS)
			{
				LOGW("Failed to create local debug messenger (res=%d)", int(res));
			}
		}
	}

	return inst;
}

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

	// Install a local DebugUtils messenger to collect validation messages into the UI.
	// If it was already created during create_instance(), skip to avoid duplication.
	if (debug_messenger_ == VK_NULL_HANDLE && get_instance().is_enabled(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
	{
		VkDebugUtilsMessengerCreateInfoEXT info{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
		info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		                  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		                  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		info.pfnUserCallback = &LayerSettingsSample::debug_callback;
		info.pUserData       = this;

		auto instance = get_instance().get_handle();
		auto fpCreate = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
		    vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
		if (fpCreate)
		{
			VK_CHECK(fpCreate(instance, &info, nullptr, &debug_messenger_));
		}
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

	// Intentionally trigger a Best Practices warning with a non-optimal barrier.
	// Using TOP_OF_PIPE -> BOTTOM_OF_PIPE with empty access masks is legal but discouraged.
	VkMemoryBarrier bp_barrier = vkb::initializers::memory_barrier();
	bp_barrier.srcAccessMask   = 0;
	bp_barrier.dstAccessMask   = 0;
	vkCmdPipelineBarrier(cmd,
	                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	                    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
	                    0,
	                    1, &bp_barrier,
	                    0, nullptr,
	                    0, nullptr);

	// Minimal render pass only to draw the UI overlay with collected validation messages.
	VkRenderPassBeginInfo render_pass_begin = vkb::initializers::render_pass_begin_info();
	render_pass_begin.renderPass            = render_pass;
	render_pass_begin.renderArea.offset     = {0, 0};
	render_pass_begin.renderArea.extent     = {width, height};
	VkClearValue clears[2]                  = {};
	clears[0].color.float32[0]              = 0.0f;
	clears[0].color.float32[1]              = 0.0f;
	clears[0].color.float32[2]              = 0.0f;
	clears[0].color.float32[3]              = 1.0f;
	clears[1].depthStencil.depth            = 1.0f;
	clears[1].depthStencil.stencil          = 0;
	render_pass_begin.clearValueCount       = 2;
	render_pass_begin.pClearValues          = clears;
	render_pass_begin.framebuffer           = framebuffers[image_index];

	vkCmdBeginRenderPass(cmd, &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE);

	// Draw the UI which will call on_update_ui_overlay().
	draw_ui(cmd);

	vkCmdEndRenderPass(cmd);

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

VKAPI_ATTR VkBool32 VKAPI_CALL LayerSettingsSample::debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void                                       *pUserData)
{
	auto *self = reinterpret_cast<LayerSettingsSample *>(pUserData);
	if (!self)
	{
		return VK_FALSE;
	}
	if (!(messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT))
	{
		// don't need to see messages in GUI that aren't related to this demonstration.
		return VK_FALSE;
	}

	const char *sev = (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)   ? "ERROR" :
	                 (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) ? "WARNING" :
	                 (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)    ? "INFO" : "VERBOSE";

	char type_buf[32] = {};
	int  ofs          = 0;
	if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
		ofs += snprintf(type_buf + ofs, sizeof(type_buf) - ofs, "%sGEN", ofs ? "|" : "");
	if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
		ofs += snprintf(type_buf + ofs, sizeof(type_buf) - ofs, "%sVAL", ofs ? "|" : "");
	if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
		ofs += snprintf(type_buf + ofs, sizeof(type_buf) - ofs, "%sPERF", ofs ? "|" : "");

	char line[2048];
	snprintf(line, sizeof(line), "[%s][%s] %s (%s)\n", sev, type_buf[0] ? type_buf : "-",
	         pCallbackData && pCallbackData->pMessage ? pCallbackData->pMessage : "<no message>",
	         pCallbackData && pCallbackData->pMessageIdName ? pCallbackData->pMessageIdName : "no-id");

	self->log_text_.append(line);
	// Avoid unbounded growth: keep last ~64KB
	if (self->log_text_.size() > 64 * 1024)
	{
		self->log_text_.erase(0, self->log_text_.size() - 64 * 1024);
	}
	return VK_FALSE; // do not abort calls
}

void LayerSettingsSample::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("VK_EXT_layer_settings"))
	{
		drawer.text("This sample enables Validation Best Practices and captures messages.");
		drawer.text("A deliberate non-optimal barrier is recorded each frame to trigger BP.");
		if (!log_text_.empty())
		{
			drawer.text("%s", log_text_.c_str());
		}
		else
		{
			drawer.text("Waiting for validation output...");
		}
	}
}

std::unique_ptr<vkb::Application> create_layer_settings()
{
	return std::make_unique<LayerSettingsSample>();
}
