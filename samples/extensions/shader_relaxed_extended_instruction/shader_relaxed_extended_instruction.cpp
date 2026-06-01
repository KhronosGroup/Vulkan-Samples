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

#include "shader_relaxed_extended_instruction.h"

#include "common/vk_common.h"
#include "common/vk_initializers.h"
#include <algorithm>
#include <cstring>

// Minimal debug utils callback to capture INFO severity messages (e.g., debugPrintfEXT)
static VKAPI_ATTR VkBool32 VKAPI_CALL s_debug_utils_message_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT             messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void                                       *pUserData)
{
	(void) messageSeverity;
	(void) messageType;
	if (pCallbackData && pCallbackData->pMessage && pUserData)
	{
		auto *self = reinterpret_cast<ShaderRelaxedExtendedInstruction *>(pUserData);
		self->append_message(pCallbackData->pMessage);
	}
	return VK_FALSE;
}

ShaderRelaxedExtendedInstruction::ShaderRelaxedExtendedInstruction()
{
	title = "Shader relaxed extended instruction (VK_KHR_shader_relaxed_extended_instruction)";

	// Device extensions used by this demo
	add_device_extension(VK_KHR_SHADER_RELAXED_EXTENDED_INSTRUCTION_EXTENSION_NAME);
	// Non-semantic info is the SPIR-V mechanism for non-semantic extended instruction sets
	add_device_extension(VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME);
}

ShaderRelaxedExtendedInstruction::~ShaderRelaxedExtendedInstruction()
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
	}
	// Destroy the local debug utils messenger if created
	if (debug_utils_messenger != VK_NULL_HANDLE)
	{
		vkDestroyDebugUtilsMessengerEXT(get_instance().get_handle(), debug_utils_messenger, nullptr);
		debug_utils_messenger = VK_NULL_HANDLE;
	}
}

void ShaderRelaxedExtendedInstruction::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	VkClearValue clear_values[2];
	clear_values[0].color        = {{0.02f, 0.02f, 0.03f, 1.0f}};        // subtle dark background
	clear_values[1].depthStencil = {1.0f, 0};

	for (int32_t i = 0; i < static_cast<int32_t>(draw_cmd_buffers.size()); ++i)
	{
		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));

		VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
		render_pass_begin_info.framebuffer              = framebuffers[i];
		render_pass_begin_info.renderPass               = render_pass;
		render_pass_begin_info.clearValueCount          = 2;
		render_pass_begin_info.renderArea.extent.width  = width;
		render_pass_begin_info.renderArea.extent.height = height;
		render_pass_begin_info.pClearValues             = clear_values;

		vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		// Draw only the UI for this sample
		draw_ui(draw_cmd_buffers[i]);

		vkCmdEndRenderPass(draw_cmd_buffers[i]);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void ShaderRelaxedExtendedInstruction::request_gpu_features(vkb::core::PhysicalDeviceC &gpu)
{
	// Require the feature for this sample
	REQUEST_REQUIRED_FEATURE(gpu, VkPhysicalDeviceShaderRelaxedExtendedInstructionFeaturesKHR, shaderRelaxedExtendedInstruction);
}

std::unique_ptr<vkb::core::InstanceC> ShaderRelaxedExtendedInstruction::create_instance()
{
	LOGI("ShaderRelaxedExtendedInstruction::create_instance: Demonstrating explicit debugPrintf setup");

	// Use Vulkan 1.3 so validation uses SPIR-V 1.6 semantics, matching our DXC target
	uint32_t debugprintf_api_version = VK_API_VERSION_1_3;

	// Enumerate layers to find VVL and its version
	uint32_t layer_property_count = 0;
	VK_CHECK(vkEnumerateInstanceLayerProperties(&layer_property_count, nullptr));
	std::vector<VkLayerProperties> layer_properties(layer_property_count);
	VK_CHECK(vkEnumerateInstanceLayerProperties(&layer_property_count, layer_properties.data()));

	const char *validation_layer_name = "VK_LAYER_KHRONOS_validation";
	auto        vvl_properties        = std::ranges::find_if(layer_properties, [validation_layer_name](const VkLayerProperties &p) {
        return strcmp(p.layerName, validation_layer_name) == 0;
    });

	const bool validation_layer_available = (vvl_properties != layer_properties.end());
	if (validation_layer_available)
	{
		// Does VVL advertise VK_EXT_layer_settings? (Newer Vulkan SDKs / layers)
		uint32_t vvl_extension_count = 0;
		VK_CHECK(vkEnumerateInstanceExtensionProperties(validation_layer_name, &vvl_extension_count, nullptr));
		std::vector<VkExtensionProperties> vvl_instance_extensions(vvl_extension_count);
		VK_CHECK(vkEnumerateInstanceExtensionProperties(validation_layer_name, &vvl_extension_count, vvl_instance_extensions.data()));

		bool has_layer_settings = std::ranges::any_of(vvl_instance_extensions, [](const VkExtensionProperties &e) {
			return strcmp(e.extensionName, VK_EXT_LAYER_SETTINGS_EXTENSION_NAME) == 0;
		});

		if (has_layer_settings)
		{
			LOGI("Validation layer supports VK_EXT_layer_settings. Using framework hooks for configuration.");
			// The framework's base create_instance will call our request_layer_settings() and request_layers() hooks.
			// We already return VK_API_VERSION_1_3 from get_api_version().
			return ApiVulkanSample::create_instance();
		}
	}

	LOGI("Fallback: Validation layer settings not available. Demonstrating manual VK_EXT_validation_features setup.");

	// Manual setup for educational demonstration
	std::vector<const char *> enabled_extensions;
	enabled_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	for (const char *extension_name : window->get_required_surface_extensions())
	{
		enabled_extensions.push_back(extension_name);
	}
	enabled_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	enabled_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	enabled_extensions.push_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);

#if (defined(VKB_ENABLE_PORTABILITY))
	// Framework portability check
	uint32_t                           instance_extension_count;
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, nullptr));
	std::vector<VkExtensionProperties> instance_extensions(instance_extension_count);
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, instance_extensions.data()));
	for (auto &ext : instance_extensions)
	{
		if (strcmp(ext.extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0)
		{
			enabled_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
			break;
		}
	}
#endif

	VkApplicationInfo app_info{VK_STRUCTURE_TYPE_APPLICATION_INFO};
	app_info.pApplicationName = "Shader relaxed extended instruction";
	app_info.pEngineName      = "Vulkan Samples";
	app_info.apiVersion       = debugprintf_api_version;

	std::vector<const char *> enabled_layers;
	if (validation_layer_available)
	{
		enabled_layers.push_back(validation_layer_name);
	}

	// This is the key "teaching" part: manually configuring the pNext chain for debugPrintf
	std::vector<VkValidationFeatureEnableEXT> validation_feature_enables = {VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT};
	VkValidationFeaturesEXT                   validation_features{VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};
	validation_features.enabledValidationFeatureCount = static_cast<uint32_t>(validation_feature_enables.size());
	validation_features.pEnabledValidationFeatures    = validation_feature_enables.data();

	VkInstanceCreateInfo instance_create_info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
	instance_create_info.pApplicationInfo        = &app_info;
	instance_create_info.enabledExtensionCount   = static_cast<uint32_t>(enabled_extensions.size());
	instance_create_info.ppEnabledExtensionNames = enabled_extensions.data();
	instance_create_info.enabledLayerCount       = static_cast<uint32_t>(enabled_layers.size());
	instance_create_info.ppEnabledLayerNames     = enabled_layers.empty() ? nullptr : enabled_layers.data();
	instance_create_info.pNext                   = &validation_features;

#if (defined(VKB_ENABLE_PORTABILITY))
	if (std::ranges::find_if(enabled_extensions, [](const char *e) { return strcmp(e, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0; }) != enabled_extensions.end())
	{
		instance_create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
	}
#endif

	VkInstance instance = VK_NULL_HANDLE;
	VK_CHECK(vkCreateInstance(&instance_create_info, nullptr, &instance));

	volkLoadInstance(instance);

	return std::make_unique<vkb::core::InstanceC>(instance, enabled_extensions);
}

uint32_t ShaderRelaxedExtendedInstruction::get_api_version() const
{
	// Use Vulkan 1.3 so validation uses SPIR-V 1.6 semantics, matching our DXC target
	return VK_API_VERSION_1_3;
}

void ShaderRelaxedExtendedInstruction::request_instance_extensions(std::unordered_map<std::string, vkb::RequestMode> &requested_extensions) const
{
	ApiVulkanSample::request_instance_extensions(requested_extensions);
	// Instance prerequisite for feature chaining
	requested_extensions[VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME] = vkb::RequestMode::Required;
	// Request layer settings extension optionally to enable debugPrintf via new framework hooks
	requested_extensions[VK_EXT_LAYER_SETTINGS_EXTENSION_NAME] = vkb::RequestMode::Optional;
	// Request validation features extension optionally for fallback path
	requested_extensions[VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME] = vkb::RequestMode::Optional;
}

void ShaderRelaxedExtendedInstruction::request_layers(std::unordered_map<std::string, vkb::RequestMode> &requested_layers) const
{
	ApiVulkanSample::request_layers(requested_layers);
	// This sample relies on the Vulkan Validation Layers (VVL) to capture and display debugPrintfEXT output
	requested_layers["VK_LAYER_KHRONOS_validation"] = vkb::RequestMode::Required;
}

void ShaderRelaxedExtendedInstruction::request_layer_settings(std::vector<VkLayerSettingEXT> &requested_layer_settings, vkb::StructureChainBuilderC<VkInstanceCreateInfo> &scb) const
{
	ApiVulkanSample::request_layer_settings(requested_layer_settings, scb);
	// Enable debug printf so shaders using debugPrintfEXT will print via VVL
	requested_layer_settings.push_back({"VK_LAYER_KHRONOS_validation", "printf_enable", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &scb.add_chain_data<VkBool32>(VK_TRUE)});
}

void ShaderRelaxedExtendedInstruction::request_validation_feature_enables(std::vector<VkValidationFeatureEnableEXT> &requested_validation_feature_enables) const
{
	ApiVulkanSample::request_validation_feature_enables(requested_validation_feature_enables);
	// Fallback: enable debugPrintf via validation features if VK_EXT_layer_settings is not supported
	requested_validation_feature_enables.push_back(VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT);
}

bool ShaderRelaxedExtendedInstruction::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	// Create an INFO-severity debug utils messenger so debugPrintf messages are visible
	{
		VkDebugUtilsMessengerCreateInfoEXT ci{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
		ci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
		ci.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
		ci.pfnUserCallback = s_debug_utils_message_callback;
		ci.pUserData       = this;
		VK_CHECK(vkCreateDebugUtilsMessengerEXT(get_instance().get_handle(), &ci, nullptr, &debug_utils_messenger));
	}

	// Create a minimal compute pipeline using a shader that emits a non-semantic
	// extended instruction (debugPrintfEXT) to demonstrate interaction with
	// VK_KHR_shader_non_semantic_info and the SPIR-V relaxed extended instruction rules.
	{
		VkPushConstantRange pc_range{};
		pc_range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		pc_range.offset     = 0;
		pc_range.size       = sizeof(uint32_t);

		VkPipelineLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
		layout_info.pushConstantRangeCount = 1;
		layout_info.pPushConstantRanges    = &pc_range;
		VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &layout_info, nullptr, &pipeline_layout));

		VkPipelineShaderStageCreateInfo stage = load_shader("shader_relaxed_extended_instruction/hlsl/relaxed_demo.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);

		VkComputePipelineCreateInfo compute_ci{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
		compute_ci.stage  = stage;
		compute_ci.layout = pipeline_layout;
		VK_CHECK(vkCreateComputePipelines(get_device().get_handle(), pipeline_cache, 1, &compute_ci, nullptr, &compute_pipeline));
	}
	// Record the UI render pass command buffers now so they are valid for submission
	build_command_buffers();

	prepared = true;
	return true;
}

void ShaderRelaxedExtendedInstruction::record_minimal_present_cmd(VkCommandBuffer) const
{}

void ShaderRelaxedExtendedInstruction::render(float)
{
	if (!prepared)
	{
		return;
	}

	// Acquire next image
	prepare_frame();

	// Conditionally run the compute dispatch if the UI value changed or if explicitly requested
	const bool value_changed = (ui_value_ != last_dispatched_value_);
	if ((value_changed || request_dispatch_once_) && compute_pipeline != VK_NULL_HANDLE)
	{
		request_dispatch_once_ = false;
		last_dispatched_value_ = ui_value_;
		auto push_value        = ui_value_;

		with_command_buffer([&](VkCommandBuffer command_buffer) {
			vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline);
			vkCmdPushConstants(command_buffer, pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t), &push_value);
			vkCmdDispatch(command_buffer, 1, 1, 1);
		});
	}

	// Submit pre-recorded UI render commands
	auto                &cmd         = draw_cmd_buffers[current_buffer];
	VkPipelineStageFlags wait_stages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo         submit_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
	submit_info.waitSemaphoreCount   = 1;
	submit_info.pWaitSemaphores      = &semaphores.acquired_image_ready;
	submit_info.pWaitDstStageMask    = &wait_stages;
	submit_info.commandBufferCount   = 1;
	submit_info.pCommandBuffers      = &cmd;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores    = &semaphores.render_complete;

	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

	// Present (waits on render_complete internally)
	submit_frame();
}

std::unique_ptr<vkb::Application> create_shader_relaxed_extended_instruction()
{
	return std::make_unique<ShaderRelaxedExtendedInstruction>();
}

void ShaderRelaxedExtendedInstruction::append_message(const char *msg)
{
	if (!msg)
		return;
	last_messages_.emplace_back(msg);
	while (last_messages_.size() > kMaxMessages_)
	{
		last_messages_.pop_front();
	}
}

void ShaderRelaxedExtendedInstruction::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("VK_KHR_shader_relaxed_extended_instruction"))
	{
		const bool has_ext  = get_device().is_extension_enabled(VK_KHR_SHADER_RELAXED_EXTENDED_INSTRUCTION_EXTENSION_NAME);
		const bool has_info = get_device().is_extension_enabled(VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME);
		drawer.text("Device extensions: relaxed_extended_instruction=%s, non_semantic_info=%s", has_ext ? "ON" : "OFF", has_info ? "ON" : "OFF");
		drawer.text("This feature enables SPIR-V modules that use relaxed forward-refs in extended instruction sets (e.g., DebugPrintf).\nUseful when tools emit richer debug info that would otherwise be rejected.");

		int32_t value_ui = static_cast<int32_t>(ui_value_);
		if (drawer.slider_int("Value", &value_ui, 0, 1000))
		{
			ui_value_ = static_cast<uint32_t>(value_ui);
		}
		if (drawer.button("Dispatch once"))
		{
			request_dispatch_once_ = true;
		}

		drawer.text("Last messages (max %d):", static_cast<int>(kMaxMessages_));
		if (last_messages_.empty())
		{
			drawer.text("<no messages yet>");
		}
		else
		{
			for (auto it = last_messages_.rbegin(); it != last_messages_.rend(); ++it)
			{
				drawer.text("%s", it->c_str());
			}
		}
	}
}
