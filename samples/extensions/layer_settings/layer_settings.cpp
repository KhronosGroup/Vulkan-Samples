/* Copyright (c) 2026, Holochip Inc.
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
#include <format>

LayerSettingsSample::~LayerSettingsSample()
{
	cleanup_scenarios();

	if (debug_messenger_ != VK_NULL_HANDLE && has_instance())
	{
		VkInstance instance = get_instance().get_handle();
		vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger_, nullptr);
		debug_messenger_ = VK_NULL_HANDLE;
	}
}

void LayerSettingsSample::cleanup_wrong_buffer_flags_scenario()
{
	if (!has_device())
	{
		return;
	}

	VkDevice device = get_device().get_handle();

	if (resources_.wrong_usage_buffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(device, resources_.wrong_usage_buffer, nullptr);
		resources_.wrong_usage_buffer = VK_NULL_HANDLE;
	}

	if (resources_.wrong_usage_memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(device, resources_.wrong_usage_memory, nullptr);
		resources_.wrong_usage_memory = VK_NULL_HANDLE;
	}
}

void LayerSettingsSample::cleanup_suboptimal_transitions_scenario()
{
	if (!has_device())
	{
		return;
	}

	VkDevice device = get_device().get_handle();

	if (resources_.test_image_view != VK_NULL_HANDLE)
	{
		vkDestroyImageView(device, resources_.test_image_view, nullptr);
		resources_.test_image_view = VK_NULL_HANDLE;
	}

	if (resources_.test_image != VK_NULL_HANDLE)
	{
		vkDestroyImage(device, resources_.test_image, nullptr);
		resources_.test_image = VK_NULL_HANDLE;
	}

	if (resources_.test_image_memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(device, resources_.test_image_memory, nullptr);
		resources_.test_image_memory = VK_NULL_HANDLE;
	}
}

void LayerSettingsSample::cleanup_small_allocations_scenario()
{
	if (!has_device())
	{
		return;
	}

	VkDevice device = get_device().get_handle();

	// Clean up small buffers (must be destroyed before their memory)
	for (auto buffer : resources_.small_buffers)
	{
		if (buffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(device, buffer, nullptr);
		}
	}
	resources_.small_buffers.clear();

	// Clean up small allocations
	for (auto mem : resources_.small_allocations)
	{
		if (mem != VK_NULL_HANDLE)
		{
			vkFreeMemory(device, mem, nullptr);
		}
	}
	resources_.small_allocations.clear();
}

void LayerSettingsSample::cleanup_scenarios()
{
	cleanup_wrong_buffer_flags_scenario();
	cleanup_suboptimal_transitions_scenario();
	cleanup_small_allocations_scenario();

	if (!has_device())
	{
		return;
	}

	VkDevice device = get_device().get_handle();

	if (resources_.simple_pipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(device, resources_.simple_pipeline, nullptr);
		resources_.simple_pipeline = VK_NULL_HANDLE;
	}

	if (resources_.pipeline_layout != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(device, resources_.pipeline_layout, nullptr);
		resources_.pipeline_layout = VK_NULL_HANDLE;
	}

	if (resources_.vertex_buffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(device, resources_.vertex_buffer, nullptr);
		resources_.vertex_buffer = VK_NULL_HANDLE;
	}

	if (resources_.vertex_buffer_memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(device, resources_.vertex_buffer_memory, nullptr);
		resources_.vertex_buffer_memory = VK_NULL_HANDLE;
	}
}

void LayerSettingsSample::render_scenario_visuals(VkCommandBuffer cmd)
{
	// Simple visual feedback: draw colored rectangles for each active scenario
	// This is a placeholder - in a full implementation, you would render actual geometry
	// For now, we just demonstrate that scenarios are active via the UI
	// The validation messages themselves provide the main feedback
}

void LayerSettingsSample::execute_one_time_commands(std::function<void(VkCommandBuffer)> commands)
{
	VkDevice device = get_device().get_handle();

	// Create a one-time command buffer
	VkCommandPoolCreateInfo pool_info = vkb::initializers::command_pool_create_info();
	pool_info.queueFamilyIndex        = get_device().get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0).get_family_index();
	pool_info.flags                   = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

	VkCommandPool command_pool;
	VK_CHECK(vkCreateCommandPool(device, &pool_info, nullptr, &command_pool));

	VkCommandBufferAllocateInfo alloc_info = vkb::initializers::command_buffer_allocate_info(command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

	VkCommandBuffer command_buffer;
	VK_CHECK(vkAllocateCommandBuffers(device, &alloc_info, &command_buffer));

	// Begin recording
	VkCommandBufferBeginInfo begin_info = vkb::initializers::command_buffer_begin_info();
	begin_info.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VK_CHECK(vkBeginCommandBuffer(command_buffer, &begin_info));

	// Execute user commands
	commands(command_buffer);

	// End recording
	VK_CHECK(vkEndCommandBuffer(command_buffer));

	// Submit and wait
	VkSubmitInfo submit_info       = vkb::initializers::submit_info();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &command_buffer;

	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	VK_CHECK(vkQueueWaitIdle(queue));

	// Cleanup
	vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
	vkDestroyCommandPool(device, command_pool, nullptr);
}

void LayerSettingsSample::setup_wrong_buffer_flags_scenario()
{
	VkDevice device = get_device().get_handle();

	// Create a buffer with TRANSFER_DST usage but we'll use it as a vertex buffer
	// This triggers a Best Practices warning about missing VERTEX_BUFFER_BIT
	VkBufferCreateInfo buffer_info = vkb::initializers::buffer_create_info();
	buffer_info.size               = 1024;
	buffer_info.usage              = VK_BUFFER_USAGE_TRANSFER_DST_BIT;        // Wrong! Should include VERTEX_BUFFER_BIT
	buffer_info.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

	VK_CHECK(vkCreateBuffer(device, &buffer_info, nullptr, &resources_.wrong_usage_buffer));

	VkMemoryRequirements mem_reqs;
	vkGetBufferMemoryRequirements(device, resources_.wrong_usage_buffer, &mem_reqs);

	VkMemoryAllocateInfo alloc_info = vkb::initializers::memory_allocate_info();
	alloc_info.allocationSize       = mem_reqs.size;
	alloc_info.memoryTypeIndex      = get_device().get_gpu().get_memory_type(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VK_CHECK(vkAllocateMemory(device, &alloc_info, nullptr, &resources_.wrong_usage_memory));
	VK_CHECK(vkBindBufferMemory(device, resources_.wrong_usage_buffer, resources_.wrong_usage_memory, 0));

	// Execute the validation-triggering operation ONCE in a one-time command buffer
	// This triggers the VERTEX_BUFFER_BIT error which gets cached
	execute_one_time_commands([this](VkCommandBuffer cmd) {
		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(cmd, 0, 1, &resources_.wrong_usage_buffer, &offset);
	});
}

void LayerSettingsSample::setup_suboptimal_transitions_scenario()
{
	VkDevice device = get_device().get_handle();

	// Create a small test image
	VkImageCreateInfo image_info = vkb::initializers::image_create_info();
	image_info.imageType         = VK_IMAGE_TYPE_2D;
	image_info.format            = VK_FORMAT_R8G8B8A8_UNORM;
	image_info.extent            = {256, 256, 1};
	image_info.mipLevels         = 1;
	image_info.arrayLayers       = 1;
	image_info.samples           = VK_SAMPLE_COUNT_1_BIT;
	image_info.tiling            = VK_IMAGE_TILING_OPTIMAL;
	image_info.usage             = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	image_info.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
	image_info.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;

	VK_CHECK(vkCreateImage(device, &image_info, nullptr, &resources_.test_image));

	VkMemoryRequirements mem_reqs;
	vkGetImageMemoryRequirements(device, resources_.test_image, &mem_reqs);

	VkMemoryAllocateInfo alloc_info = vkb::initializers::memory_allocate_info();
	alloc_info.allocationSize       = mem_reqs.size;
	alloc_info.memoryTypeIndex      = get_device().get_gpu().get_memory_type(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VK_CHECK(vkAllocateMemory(device, &alloc_info, nullptr, &resources_.test_image_memory));
	VK_CHECK(vkBindImageMemory(device, resources_.test_image, resources_.test_image_memory, 0));

	// Create image view
	VkImageViewCreateInfo view_info           = vkb::initializers::image_view_create_info();
	view_info.image                           = resources_.test_image;
	view_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	view_info.format                          = VK_FORMAT_R8G8B8A8_UNORM;
	view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	view_info.subresourceRange.baseMipLevel   = 0;
	view_info.subresourceRange.levelCount     = 1;
	view_info.subresourceRange.baseArrayLayer = 0;
	view_info.subresourceRange.layerCount     = 1;

	VK_CHECK(vkCreateImageView(device, &view_info, nullptr, &resources_.test_image_view));

	// Execute the validation-triggering operation ONCE in a one-time command buffer
	// Perform a suboptimal layout transition: UNDEFINED -> GENERAL
	execute_one_time_commands([this](VkCommandBuffer cmd) {
		VkImageMemoryBarrier img_barrier        = vkb::initializers::image_memory_barrier();
		img_barrier.oldLayout                   = VK_IMAGE_LAYOUT_UNDEFINED;
		img_barrier.newLayout                   = VK_IMAGE_LAYOUT_GENERAL;        // Suboptimal!
		img_barrier.srcAccessMask               = 0;
		img_barrier.dstAccessMask               = VK_ACCESS_SHADER_READ_BIT;
		img_barrier.image                       = resources_.test_image;
		img_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		img_barrier.subresourceRange.levelCount = 1;
		img_barrier.subresourceRange.layerCount = 1;

		vkCmdPipelineBarrier(cmd,
		                     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		                     0,
		                     0, nullptr,
		                     0, nullptr,
		                     1, &img_barrier);
	});
}

void LayerSettingsSample::setup_small_allocations_scenario()
{
	VkDevice device = get_device().get_handle();

	// Create many small buffers with dedicated allocations instead of suballocating
	// This is inefficient and triggers Best Practices warnings about small-dedicated-allocation
	resources_.small_buffers.reserve(50);
	resources_.small_allocations.reserve(50);

	// Create 50 small buffers, each with its own dedicated memory allocation
	// This triggers "small-dedicated-allocation" warnings from Best Practices
	for (size_t i = 0; i < 50; ++i)
	{
		VkBufferCreateInfo buffer_info = vkb::initializers::buffer_create_info();
		buffer_info.size               = 512;        // Small buffer (below threshold)
		buffer_info.usage              = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		buffer_info.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

		VkBuffer buffer = VK_NULL_HANDLE;
		VkResult res    = vkCreateBuffer(device, &buffer_info, nullptr, &buffer);
		if (res != VK_SUCCESS)
		{
			break;
		}

		VkMemoryRequirements mem_reqs;
		vkGetBufferMemoryRequirements(device, buffer, &mem_reqs);

		VkMemoryAllocateInfo alloc_info = vkb::initializers::memory_allocate_info();
		alloc_info.allocationSize       = mem_reqs.size;
		alloc_info.memoryTypeIndex      = get_device().get_gpu().get_memory_type(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		VkDeviceMemory mem = VK_NULL_HANDLE;
		res                = vkAllocateMemory(device, &alloc_info, nullptr, &mem);
		if (res != VK_SUCCESS)
		{
			vkDestroyBuffer(device, buffer, nullptr);
			break;
		}

		// Bind buffer to memory - this triggers the small-dedicated-allocation warning
		res = vkBindBufferMemory(device, buffer, mem, 0);
		if (res != VK_SUCCESS)
		{
			vkDestroyBuffer(device, buffer, nullptr);
			vkFreeMemory(device, mem, nullptr);
			break;
		}

		resources_.small_buffers.push_back(buffer);
		resources_.small_allocations.push_back(mem);
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

		if (debug_messenger_ == VK_NULL_HANDLE)
		{
			VkResult res = vkCreateDebugUtilsMessengerEXT(inst->get_handle(), &info, nullptr, &debug_messenger_);
			if (res != VK_SUCCESS)
			{
				LOGW("Failed to create local debug messenger (res=%d)", int(res));
			}
		}
	}

	// Note: If VK_EXT_debug_utils is not enabled (e.g., disabled via CLI or platform
	// constraints) or creation fails, the messenger will remain null and the sample
	// will continue to run without collecting messages into the UI.
	return inst;
}

LayerSettingsSample::LayerSettingsSample()
{
	title = "Layer settings (VK_EXT_layer_settings)";

	// Request VK_EXT_layer_settings as an optional instance extension so the
	// framework enables it when available and consumes the layer settings below.
	add_instance_extension(VK_EXT_LAYER_SETTINGS_EXTENSION_NAME, /*optional*/ true);

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

	// Initialize scenario state map so UI totals and logic have all keys even before toggling
	scenario_states_.emplace(Scenario::WrongBufferFlags, ScenarioState{});
	scenario_states_.emplace(Scenario::SuboptimalTransitions, ScenarioState{});
	scenario_states_.emplace(Scenario::SmallAllocations, ScenarioState{});
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

	// Scenarios now execute ONCE during setup, not every frame
	// This prevents hitting the validation layer's duplicate message limit
	// and allows message caching to work properly across toggle cycles

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

	// Accept VALIDATION and PERFORMANCE messages (both are relevant for this demo)
	if (!(messageTypes & (VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)))
	{
		return VK_FALSE;
	}

	const char *sev = (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)   ? "ERROR" :
	                  (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) ? "WARNING" :
	                  (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)    ? "INFO" :
	                                                                                        "VERBOSE";

	std::string type_str;
	if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
		type_str += (type_str.empty() ? "GEN" : "|GEN");
	if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
		type_str += (type_str.empty() ? "VAL" : "|VAL");
	if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
		type_str += (type_str.empty() ? "PERF" : "|PERF");

	const char *msg_id = pCallbackData && pCallbackData->pMessageIdName ? pCallbackData->pMessageIdName : "";
	const char *msg    = pCallbackData && pCallbackData->pMessage ? pCallbackData->pMessage : "";

	std::string line = std::format("[{}][{}] {}\n",
	                               sev,
	                               type_str.empty() ? std::string("-") : type_str,
	                               pCallbackData && pCallbackData->pMessage ? pCallbackData->pMessage : "<no message>");

	bool message_cached = false;
	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		if (strstr(msg_id, "vkCmdBindVertexBuffers") || strstr(msg, "VERTEX_BUFFER_BIT"))
		{
			auto &state = self->scenario_states_[Scenario::WrongBufferFlags];
			state.error_count++;
			if (state.enabled)
			{
				state.recent_messages.append(line);
				message_cached = true;
			}
		}
	}
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		if (strstr(msg_id, "small-dedicated-allocation") || strstr(msg, "small-dedicated-allocation"))
		{
			if (strstr(msg_id, "vkBindBufferMemory") || strstr(msg, "vkBindBufferMemory"))
			{
				if (self->scenario_states_[Scenario::SmallAllocations].enabled)
				{
					auto &state = self->scenario_states_[Scenario::SmallAllocations];
					state.warning_count++;
					state.recent_messages.append(line);
					message_cached = true;
				}
				if (self->scenario_states_[Scenario::WrongBufferFlags].enabled)
				{
					auto &state = self->scenario_states_[Scenario::WrongBufferFlags];
					state.warning_count++;
					if (!message_cached)
					{
						state.recent_messages.append(line);
						message_cached = true;
					}
				}
			}
			else if (strstr(msg_id, "vkBindImageMemory") || strstr(msg, "vkBindImageMemory"))
			{
				if (self->scenario_states_[Scenario::SuboptimalTransitions].enabled)
				{
					auto &state = self->scenario_states_[Scenario::SuboptimalTransitions];
					state.warning_count++;
					state.recent_messages.append(line);
					message_cached = true;
				}
			}
		}
		else if (strstr(msg, "GENERAL") || strstr(msg, "layout"))
		{
			if (self->scenario_states_[Scenario::SuboptimalTransitions].enabled)
			{
				auto &state = self->scenario_states_[Scenario::SuboptimalTransitions];
				state.warning_count++;
				state.recent_messages.append(line);
				message_cached = true;
			}
		}
	}

	self->log_text_.append(line);
	if (self->log_text_.size() > 8 * 1024)
	{
		self->log_text_.erase(0, self->log_text_.size() - 8 * 1024);
	}
	return VK_FALSE;
}

void LayerSettingsSample::on_update_ui_overlay(vkb::Drawer &drawer)
{
	// Reduce font scale for better readability
	ImGui::SetWindowFontScale(0.75f);

	// Improve checkbox visibility with better colors
	ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.26f, 0.59f, 0.98f, 1.0f));             // Bright blue checkmark
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.16f, 0.16f, 0.17f, 1.0f));               // Dark gray background
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.26f, 0.26f, 0.27f, 1.0f));        // Lighter on hover
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.36f, 0.36f, 0.37f, 1.0f));         // Even lighter when active

	if (drawer.header("VK_EXT_layer_settings Demo"))
	{
		drawer.text("Interactive demonstration of Best Practices validation.");
		drawer.text("Toggle scenarios below to trigger validation warnings:");
		drawer.text("");

		// Scenario toggles
		if (drawer.checkbox("Wrong Buffer Flags", &scenario_states_[Scenario::WrongBufferFlags].enabled))
		{
			auto &state = scenario_states_[Scenario::WrongBufferFlags];
			if (state.enabled)
			{
				// Only setup if not already set up (first enable or after cleanup)
				if (resources_.wrong_usage_buffer == VK_NULL_HANDLE)
				{
					setup_wrong_buffer_flags_scenario();
				}
				// Restore cached messages to log
				if (!state.recent_messages.empty())
				{
					log_text_.append(state.recent_messages);
				}
			}
			else
			{
				// Cleanup resources when disabled, but preserve cached messages
				cleanup_wrong_buffer_flags_scenario();
				// Don't clear state.recent_messages - keep them for next enable
			}
		}
		ImGui::SameLine();
		drawer.text("  Warnings: %u | Errors: %u",
		            scenario_states_[Scenario::WrongBufferFlags].warning_count,
		            scenario_states_[Scenario::WrongBufferFlags].error_count);

		if (drawer.checkbox("Suboptimal Transitions", &scenario_states_[Scenario::SuboptimalTransitions].enabled))
		{
			auto &state = scenario_states_[Scenario::SuboptimalTransitions];
			if (state.enabled)
			{
				// Only setup if not already set up (first enable or after cleanup)
				if (resources_.test_image == VK_NULL_HANDLE)
				{
					setup_suboptimal_transitions_scenario();
				}
				// Restore cached messages to log
				if (!state.recent_messages.empty())
				{
					log_text_.append(state.recent_messages);
				}
			}
			else
			{
				// Cleanup resources when disabled, but preserve cached messages
				cleanup_suboptimal_transitions_scenario();
				// Don't clear state.recent_messages - keep them for next enable
			}
		}
		ImGui::SameLine();
		drawer.text("  Warnings: %u | Errors: %u",
		            scenario_states_[Scenario::SuboptimalTransitions].warning_count,
		            scenario_states_[Scenario::SuboptimalTransitions].error_count);

		if (drawer.checkbox("Many Small Allocations", &scenario_states_[Scenario::SmallAllocations].enabled))
		{
			auto &state = scenario_states_[Scenario::SmallAllocations];
			if (state.enabled)
			{
				// Only setup if not already set up (first enable or after cleanup)
				if (resources_.small_buffers.empty())
				{
					setup_small_allocations_scenario();
				}
				// Restore cached messages to log
				if (!state.recent_messages.empty())
				{
					log_text_.append(state.recent_messages);
				}
			}
			else
			{
				// Cleanup resources when disabled, but preserve cached messages
				cleanup_small_allocations_scenario();
				// Don't clear state.recent_messages - keep them for next enable
			}
		}
		ImGui::SameLine();
		drawer.text("  Warnings: %u | Errors: %u",
		            scenario_states_[Scenario::SmallAllocations].warning_count,
		            scenario_states_[Scenario::SmallAllocations].error_count);

		drawer.text("");

		// Check if all scenarios are disabled and clear log if so
		bool any_enabled = false;
		for (const auto &kv : scenario_states_)
		{
			const auto &state = kv.second;
			if (state.enabled)
			{
				any_enabled = true;
				break;
			}
		}
		if (!any_enabled)
		{
			log_text_.clear();
		}

		// Total statistics
		uint32_t total_warnings = 0;
		uint32_t total_errors   = 0;
		for (const auto &kv : scenario_states_)
		{
			const auto &state = kv.second;
			total_warnings += state.warning_count;
			total_errors += state.error_count;
		}
		drawer.text("Total Warnings: %u | Total Errors: %u", total_warnings, total_errors);

		drawer.text("");
		drawer.text("Recent Validation Messages:");

		// Use a scrollable text box for better message display
		if (!log_text_.empty())
		{
			ImGui::InputTextMultiline("##messages", const_cast<char *>(log_text_.c_str()), log_text_.size() + 1,
			                          ImVec2(-1.0f, 200.0f), ImGuiInputTextFlags_ReadOnly);
		}
		else
		{
			drawer.text("(No messages yet - enable scenarios above)");
		}
	}

	// Restore style colors
	ImGui::PopStyleColor(4);

	// Restore font scale
	ImGui::SetWindowFontScale(1.0f);
}

std::unique_ptr<vkb::Application> create_layer_settings()
{
	return std::make_unique<LayerSettingsSample>();
}
