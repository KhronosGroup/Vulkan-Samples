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

#include "shader_relaxed_extended_instruction.h"

#include "common/vk_common.h"
#include "common/vk_initializers.h"

// Minimal debug utils callback to print INFO severity messages (e.g., debugPrintfEXT)
static VKAPI_ATTR VkBool32 VKAPI_CALL s_debug_utils_message_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT             messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void                                       *pUserData)
{
	(void)messageSeverity;
	(void)messageType;
	(void)pUserData;
	if (pCallbackData && pCallbackData->pMessage)
	{
		LOGI("{}", pCallbackData->pMessage);
	}
	return VK_FALSE;
}

ShaderRelaxedExtendedInstruction::ShaderRelaxedExtendedInstruction()
{
	title = "Shader relaxed extended instruction (VK_KHR_shader_relaxed_extended_instruction)";

 // Instance prerequisite for feature chaining and layer settings (optional)
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	add_instance_extension(VK_EXT_LAYER_SETTINGS_EXTENSION_NAME, /*optional*/ true);

	// Device extensions used by this demo
	add_device_extension(VK_KHR_SHADER_RELAXED_EXTENDED_INSTRUCTION_EXTENSION_NAME);
	// Non-semantic info is the SPIR-V mechanism for non-semantic extended instruction sets
	add_device_extension(VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME);

	// Optionally, enable debug printf so shaders using debugPrintfEXT will print via VVL
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
	// Intentionally empty; this sample records per-frame in render().
}

void ShaderRelaxedExtendedInstruction::request_gpu_features(vkb::core::PhysicalDeviceC &gpu)
{
	// Require the feature for this sample
	REQUEST_REQUIRED_FEATURE(gpu, VkPhysicalDeviceShaderRelaxedExtendedInstructionFeaturesKHR, shaderRelaxedExtendedInstruction);
}

std::unique_ptr<vkb::core::InstanceC> ShaderRelaxedExtendedInstruction::create_instance()
{
	LOGI("ShaderRelaxedExtendedInstruction::create_instance override invoked");
	auto debugprintf_api_version = VK_API_VERSION_1_2;

	// Enumerate layers to find VVL and its version
	uint32_t layer_property_count = 0;
	VK_CHECK(vkEnumerateInstanceLayerProperties(&layer_property_count, nullptr));
	std::vector<VkLayerProperties> layer_properties(layer_property_count);
	VK_CHECK(vkEnumerateInstanceLayerProperties(&layer_property_count, layer_properties.data()));

	const char *validation_layer_name = "VK_LAYER_KHRONOS_validation";
	auto        vvl_properties        = std::ranges::find_if(layer_properties, [validation_layer_name](const VkLayerProperties &p) {
		return strcmp(p.layerName, validation_layer_name) == 0;
	});

	if (vvl_properties != layer_properties.end())
	{
		// Does VVL advertise VK_EXT_layer_settings?
		uint32_t vvl_extension_count = 0;
		VK_CHECK(vkEnumerateInstanceExtensionProperties(validation_layer_name, &vvl_extension_count, nullptr));
		std::vector<VkExtensionProperties> vvl_instance_extensions(vvl_extension_count);
		VK_CHECK(vkEnumerateInstanceExtensionProperties(validation_layer_name, &vvl_extension_count, vvl_instance_extensions.data()));

		bool has_layer_settings = std::ranges::any_of(vvl_instance_extensions, [](const VkExtensionProperties &e) {
			return strcmp(e.extensionName, VK_EXT_LAYER_SETTINGS_EXTENSION_NAME) == 0;
		});

		if (has_layer_settings)
		{
			set_api_version(debugprintf_api_version);
			// Use the base implementation which will chain our pre-added layer settings
			return VulkanSample::create_instance();
		}
	}

	// Fallback: use VK_EXT_validation_features to enable debugPrintf without layer settings
	std::vector<const char *> enabled_extensions;
	enabled_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	for (const char *extension_name : window->get_required_surface_extensions())
	{
		enabled_extensions.push_back(extension_name);
	}
	enabled_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	enabled_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

#if (defined(VKB_ENABLE_PORTABILITY))
	// Check for portability enumeration
	uint32_t available_extension_count = 0;
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &available_extension_count, nullptr));
	std::vector<VkExtensionProperties> available_instance_extensions(available_extension_count);
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &available_extension_count, available_instance_extensions.data()));
	bool portability_enumeration_available = std::ranges::any_of(available_instance_extensions, [](const VkExtensionProperties &e) {
		return strcmp(e.extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0;
	});
	if (portability_enumeration_available)
	{
		enabled_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
	}
#endif

	// Enable validation features to activate debugPrintf
	enabled_extensions.push_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);

	VkApplicationInfo app_info{VK_STRUCTURE_TYPE_APPLICATION_INFO};
	app_info.pApplicationName = "Shader relaxed extended instruction";
	app_info.pEngineName      = "Vulkan Samples";
	app_info.apiVersion       = debugprintf_api_version;

	std::vector<const char *> validation_layers = {validation_layer_name};

	std::vector<VkValidationFeatureEnableEXT> validation_feature_enables = {VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT};
	VkValidationFeaturesEXT                   validation_features{VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};
	validation_features.enabledValidationFeatureCount = static_cast<uint32_t>(validation_feature_enables.size());
	validation_features.pEnabledValidationFeatures    = validation_feature_enables.data();

	VkInstanceCreateInfo instance_create_info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
	instance_create_info.ppEnabledExtensionNames = enabled_extensions.data();
	instance_create_info.enabledExtensionCount   = static_cast<uint32_t>(enabled_extensions.size());
	instance_create_info.pApplicationInfo        = &app_info;
	instance_create_info.ppEnabledLayerNames     = validation_layers.data();
	instance_create_info.enabledLayerCount       = static_cast<uint32_t>(validation_layers.size());
#if (defined(VKB_ENABLE_PORTABILITY))
	if (portability_enumeration_available)
	{
		instance_create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
	}
#endif
	instance_create_info.pNext = &validation_features;

	VkInstance vulkan_instance = VK_NULL_HANDLE;
	VK_CHECK(vkCreateInstance(&instance_create_info, nullptr, &vulkan_instance));

	volkLoadInstance(vulkan_instance);

	return std::make_unique<vkb::core::InstanceC>(vulkan_instance, enabled_extensions);
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
		VK_CHECK(vkCreateDebugUtilsMessengerEXT(get_instance().get_handle(), &ci, nullptr, &debug_utils_messenger));
	}

	// Create a minimal compute pipeline using a shader that emits a non-semantic
	// extended instruction (debugPrintfEXT) to demonstrate interaction with
	// VK_KHR_shader_non_semantic_info and the SPIR-V relaxed extended instruction rules.
	{
		VkPipelineLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
		VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &layout_info, nullptr, &pipeline_layout));

		VkPipelineShaderStageCreateInfo stage = load_shader("shader_relaxed_extended_instruction/glsl/relaxed_demo.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);

		VkComputePipelineCreateInfo compute_ci{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
		compute_ci.stage  = stage;
		compute_ci.layout = pipeline_layout;
		VK_CHECK(vkCreateComputePipelines(get_device().get_handle(), pipeline_cache, 1, &compute_ci, nullptr, &compute_pipeline));
	}

	prepared = true;
	return true;
}

void ShaderRelaxedExtendedInstruction::record_minimal_present_cmd(VkCommandBuffer cmd) const
{
	VkCommandBufferBeginInfo begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	VK_CHECK(vkBeginCommandBuffer(cmd, &begin_info));

	// Demonstrate a non-semantic extended instruction by dispatching the compute shader
	// which calls debugPrintfEXT. If validation is configured to capture debug printf,
	// you should see a message per dispatch.
	if (compute_pipeline != VK_NULL_HANDLE)
	{
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline);
		vkCmdDispatch(cmd, 1, 1, 1);
	}

	// Transition the acquired swapchain image to PRESENT so validation is happy
	VkImageSubresourceRange subresource_range{};
	subresource_range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource_range.baseMipLevel   = 0;
	subresource_range.levelCount     = 1;
	subresource_range.baseArrayLayer = 0;
	subresource_range.layerCount     = 1;

	vkb::image_layout_transition(cmd,
	                            swapchain_buffers[current_buffer].image,
	                            VK_IMAGE_LAYOUT_UNDEFINED,
	                            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	                            subresource_range);

	VK_CHECK(vkEndCommandBuffer(cmd));
}

void ShaderRelaxedExtendedInstruction::render(float)
{
	if (!prepared)
	{
		return;
	}

	// Acquire next image
	prepare_frame();

	// Recreate and record a minimal command buffer for this frame
	recreate_current_command_buffer();
	auto &cmd = draw_cmd_buffers[current_buffer];
	record_minimal_present_cmd(cmd);

	// Submit with acquire->present synchronization
	VkPipelineStageFlags wait_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
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
