/* Copyright (c) 2026, NVIDIA CORPORATION
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

#include "swapchain_present_timing.h"

#include "common/vk_common.h"
#include "core/util/logging.hpp"
#include "filesystem/legacy.h"

/**
 * @brief Get a graphics queue suitable for presentation.
 */
void SwapchainPresentTiming::get_queue()
{
	queue = &get_device().get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);

	// Make sure presentation is supported on this queue.  This is practically always the case;
	// if a platform/driver is found where this is not true, all queues supporting
	// VK_QUEUE_GRAPHICS_BIT need to be queried and one that supports presentation picked.
	VkBool32 supports_present = VK_FALSE;
	vkGetPhysicalDeviceSurfaceSupportKHR(get_gpu_handle(), queue->get_family_index(), get_surface(), &supports_present);

	if (!supports_present)
	{
		throw std::runtime_error("Default graphics queue does not support present.");
	}
}

/**
 * @brief Selects the swapchain surface format
 */
void SwapchainPresentTiming::select_surface_format()
{
	surface_format = vkb::select_surface_format(get_gpu_handle(), get_surface());
}

/**
 * @brief Queries surface capabilities and present timing support
 */
void SwapchainPresentTiming::query_surface_capabilities()
{
	VkSurfaceCapabilities2KHR             surface_capabilities{VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR};
	VkPresentTimingSurfaceCapabilitiesEXT present_timing_capabilities{VK_STRUCTURE_TYPE_PRESENT_TIMING_SURFACE_CAPABILITIES_EXT};
	VkPhysicalDeviceSurfaceInfo2KHR       surface_info{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR};

	surface_info.surface = get_surface();

	surface_capabilities.pNext = &present_timing_capabilities;

	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilities2KHR(get_gpu_handle(), &surface_info, &surface_capabilities));

	if (surface_capabilities.surfaceCapabilities.currentExtent.width == 0xFFFFFFFF)
	{
		swapchain_extents = VkExtent2D{400, 300};
	}
	else
	{
		swapchain_extents = surface_capabilities.surfaceCapabilities.currentExtent;
	}

	// Do triple-buffering when possible.  This is clamped to the min and max image count limits.
	desired_swapchain_images = std::max(surface_capabilities.surfaceCapabilities.minImageCount, 3u);
	if (surface_capabilities.surfaceCapabilities.maxImageCount > 0)
	{
		desired_swapchain_images = std::min(desired_swapchain_images, surface_capabilities.surfaceCapabilities.maxImageCount);
	}

	// Find a supported composite type.
	composite = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	if (surface_capabilities.surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
	{
		composite = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	}
	else if (surface_capabilities.surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
	{
		composite = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
	}
	else if (surface_capabilities.surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
	{
		composite = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
	}
	else if (surface_capabilities.surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
	{
		composite = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
	}

	// Determine which present stages and presentAt method to use.
	// We interpret the "display" present stage as the closest possible to the FIRST_PIXEL_VISIBLE stage.
	// Ignore VK_PRESENT_STAGE_QUEUE_OPERATIONS_END_BIT_EXT as it might be too far from display to be useful.
	can_present_at_absolute_time = present_timing_capabilities.presentAtAbsoluteTimeSupported;
	display_present_stage        = 0;

	if (present_timing_capabilities.presentStageQueries & VK_PRESENT_STAGE_IMAGE_FIRST_PIXEL_VISIBLE_BIT_EXT)
	{
		display_present_stage = VK_PRESENT_STAGE_IMAGE_FIRST_PIXEL_VISIBLE_BIT_EXT;
	}
	else if (present_timing_capabilities.presentStageQueries & VK_PRESENT_STAGE_IMAGE_FIRST_PIXEL_OUT_BIT_EXT)
	{
		display_present_stage = VK_PRESENT_STAGE_IMAGE_FIRST_PIXEL_OUT_BIT_EXT;
	}
	else if (present_timing_capabilities.presentStageQueries & VK_PRESENT_STAGE_REQUEST_DEQUEUED_BIT_EXT)
	{
		display_present_stage = VK_PRESENT_STAGE_REQUEST_DEQUEUED_BIT_EXT;
	}

	// We'll calculate the animation time based on the display_present_stage's time measurements, so
	// don't try to use present timing if we don't have one.
	if (display_present_stage &&
	    (present_timing_capabilities.presentAtRelativeTimeSupported ||
	     present_timing_capabilities.presentAtAbsoluteTimeSupported))
	{
		can_use_present_timing = true;
	}
	else
	{
		can_use_present_timing = false;
	}
}

/**
 * @brief Selects the swapchain-local time domain used for present timing.
 */
void SwapchainPresentTiming::select_swapchain_time_domain()
{
	VkSwapchainTimeDomainPropertiesEXT time_domain_properties{VK_STRUCTURE_TYPE_SWAPCHAIN_TIME_DOMAIN_PROPERTIES_EXT};
	time_domain_properties.pTimeDomains = nullptr;

	VK_CHECK(vkGetSwapchainTimeDomainPropertiesEXT(get_device_handle(), swapchain, &time_domain_properties, &time_domains_counter));

	std::vector<VkTimeDomainKHR> domains(time_domain_properties.timeDomainCount);
	std::vector<uint64_t>        domain_ids(time_domain_properties.timeDomainCount);

	time_domain_properties.pTimeDomains   = domains.data();
	time_domain_properties.pTimeDomainIds = domain_ids.data();
	VK_CHECK(vkGetSwapchainTimeDomainPropertiesEXT(get_device_handle(), swapchain, &time_domain_properties, &time_domains_counter));

	// Associate a VkTimeDomainKHR with its time domain ID returned by the swapchain.
	auto find_time_domain = [time_domain_properties](VkTimeDomainKHR time_domain) -> SwapchainTimeDomain {
		for (uint32_t i = 0; i < time_domain_properties.timeDomainCount; i++)
		{
			if (time_domain_properties.pTimeDomains[i] == time_domain)
			{
				return SwapchainTimeDomain{time_domain_properties.pTimeDomains[i], time_domain_properties.pTimeDomainIds[i]};
			}
		}
		return SwapchainTimeDomain{VK_TIME_DOMAIN_MAX_ENUM_KHR, 0};
	};

	// Any swapchain-local time domain will do.
	time_domain = find_time_domain(VK_TIME_DOMAIN_SWAPCHAIN_LOCAL_EXT);
	if (time_domain.time_domain == VK_TIME_DOMAIN_MAX_ENUM_KHR)
	{
		time_domain = find_time_domain(VK_TIME_DOMAIN_PRESENT_STAGE_LOCAL_EXT);
	}
}

void SwapchainPresentTiming::query_swapchain_timing_properties()
{
	timing_properties.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_TIMING_PROPERTIES_EXT;
	timing_properties.pNext = nullptr;
	VkResult result         = vkGetSwapchainTimingPropertiesEXT(get_device_handle(), swapchain, &timing_properties, &timing_properties_counter);

	if (result != VK_SUCCESS)
	{
		timing_properties.refreshDuration = 0;
		timing_properties.refreshInterval = 0;
	}

	LOGI("New timing properties: refreshDuration={} refreshInterval={}",
	     timing_properties.refreshDuration, timing_properties.refreshInterval);
}

/**
 * @brief Sets the target present duration from timing properties (or default 60 Hz).
 */
void SwapchainPresentTiming::select_target_present_duration()
{
	static constexpr uint64_t default_refresh_duration_ns = 16'666'666;

	if (timing_properties.refreshDuration > 0)
	{
		// Match the presentation engine's refresh rate.
		target_present_duration = timing_properties.refreshDuration;
	}
	else
	{
		// Default to 60Hz if timing_properties are not available yet.
		target_present_duration = default_refresh_duration_ns;
	}

	// Here, timing_history could be analyzed to detect latency, skipped frames,
	// micro-stutters, or even pick a new present duration that is optimal
	// for the current workload.
}

/**
 * @brief Invalidates the current timing statistics after recreating a swapchain or changing time domain.
 */
void SwapchainPresentTiming::invalidate_timing_history()
{
	timing_history_size     = 0;
	display_time_present_id = 0;
	display_time_ns         = 0;
	display_time_ns_base    = 0;
}

/**
 * @brief Retrieves past presentation timing from the driver and updates timing history.
 */
void SwapchainPresentTiming::query_past_presentation_timing()
{
	VkPastPresentationTimingPropertiesEXT past_presentation_properties{VK_STRUCTURE_TYPE_PAST_PRESENTATION_TIMING_PROPERTIES_EXT};
	VkPastPresentationTimingInfoEXT       past_presentation_info{VK_STRUCTURE_TYPE_PAST_PRESENTATION_TIMING_INFO_EXT};

	past_presentation_info.swapchain = swapchain;

	past_presentation_properties.pPresentationTimings    = past_presentation_timings.data();
	past_presentation_properties.presentationTimingCount = past_presentation_timings.size();

	VK_CHECK(vkGetPastPresentationTimingEXT(get_device_handle(), &past_presentation_info, &past_presentation_properties));

	for (uint32_t i = 0; i < past_presentation_properties.presentationTimingCount; i++)
	{
		VkPastPresentationTimingEXT &timing = past_presentation_timings[i];
		size_t                       index  = timing_history_size % history_buffer_size;

		timing_history[index].present_id  = timing.presentId;
		timing_history[index].target_time = timing.targetTime;

		for (uint32_t j = 0; j < timing.presentStageCount; j++)
		{
			if (timing.pPresentStages[j].stage == VK_PRESENT_STAGE_QUEUE_OPERATIONS_END_BIT_EXT)
			{
				timing_history[index].present_ready_time = timing.pPresentStages[j].time;
			}
			if (timing.pPresentStages[j].stage == VK_PRESENT_STAGE_REQUEST_DEQUEUED_BIT_EXT)
			{
				timing_history[index].present_dequeued_time = timing.pPresentStages[j].time;
			}
			if (timing.pPresentStages[j].stage == display_present_stage)
			{
				timing_history[index].present_display_time = timing.pPresentStages[j].time;
			}
		}

		timing_history_size++;

		// Save the last known display time and present id, which we'll use as a
		// reference to predict the next presentation time.
		//
		// Since we did not allow out of order or incomplete results, these are
		// guaranteed to be in increasing order.
		if (timing_history[index].present_display_time)
		{
			display_time_present_id = timing.presentId;
			display_time_ns         = timing_history[index].present_display_time;

			// We save the first display time as a base to increase the
			// floating point accuracy when converting the time to
			// seconds for the animation.
			if (display_time_ns_base == 0)
			{
				display_time_ns_base = display_time_ns;
			}
		}

		// Reset presentStageCount for the next vkGetPastPresentationTimingEXT call.
		timing.presentStageCount = max_present_stages;
	}

	if (past_presentation_properties.timingPropertiesCounter != timing_properties_counter)
	{
		query_swapchain_timing_properties();
		select_target_present_duration();
		timing_properties_counter = past_presentation_properties.timingPropertiesCounter;
	}

	if (past_presentation_properties.timeDomainsCounter != time_domains_counter)
	{
		const uint64_t current_time_domain_id = time_domain.time_domain_id;

		select_swapchain_time_domain();

		// If we changed time domain, invalidate our present timing history.
		if (current_time_domain_id != time_domain.time_domain_id)
		{
			invalidate_timing_history();
		}

		time_domains_counter = past_presentation_properties.timeDomainsCounter;
	}
}

/**
 * @brief Creates the graphics pipeline which is a basic vertex shader passthrough and a frament shader drawing a circle.
 */
void SwapchainPresentTiming::create_pipeline()
{
	auto vert_module = vkb::load_shader("swapchain_present_timing/" + get_shader_folder() + "/fullscreen_triangle.vert.spv", get_device_handle(), VK_SHADER_STAGE_VERTEX_BIT);
	auto frag_module = vkb::load_shader("swapchain_present_timing/" + get_shader_folder() + "/circle.frag.spv", get_device_handle(), VK_SHADER_STAGE_FRAGMENT_BIT);

	VkPushConstantRange push_constant_range{};
	push_constant_range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	push_constant_range.offset     = 0;
	push_constant_range.size       = sizeof(PushConstants);

	VkPipelineLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
	layout_info.pushConstantRangeCount = 1;
	layout_info.pPushConstantRanges    = &push_constant_range;

	VK_CHECK(vkCreatePipelineLayout(get_device_handle(), &layout_info, nullptr, &pipeline_layout));

	VkPipelineShaderStageCreateInfo stages[2] = {};

	stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = vert_module;
	stages[0].pName  = "main";

	stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = frag_module;
	stages[1].pName  = "main";

	VkPipelineVertexInputStateCreateInfo vertex_input_state{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
	input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineViewportStateCreateInfo viewport_state{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
	viewport_state.viewportCount = 1;
	viewport_state.scissorCount  = 1;

	VkPipelineRasterizationStateCreateInfo rasterization_state{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
	rasterization_state.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisample_state{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
	multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState blend_attachment{};
	blend_attachment.blendEnable         = VK_TRUE;
	blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blend_attachment.colorBlendOp        = VK_BLEND_OP_ADD;
	blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	blend_attachment.alphaBlendOp        = VK_BLEND_OP_ADD;
	blend_attachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo color_blend_state{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
	color_blend_state.attachmentCount = 1;
	color_blend_state.pAttachments    = &blend_attachment;

	VkDynamicState                   dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamic_state{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
	dynamic_state.dynamicStateCount = 2;
	dynamic_state.pDynamicStates    = dynamic_states;

	VkPipelineRenderingCreateInfo rendering_info{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
	rendering_info.colorAttachmentCount    = 1;
	rendering_info.pColorAttachmentFormats = &surface_format.format;

	VkGraphicsPipelineCreateInfo pipeline_info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
	pipeline_info.pNext               = &rendering_info;
	pipeline_info.stageCount          = 2;
	pipeline_info.pStages             = stages;
	pipeline_info.pVertexInputState   = &vertex_input_state;
	pipeline_info.pInputAssemblyState = &input_assembly_state;
	pipeline_info.pViewportState      = &viewport_state;
	pipeline_info.pRasterizationState = &rasterization_state;
	pipeline_info.pMultisampleState   = &multisample_state;
	pipeline_info.pColorBlendState    = &color_blend_state;
	pipeline_info.pDynamicState       = &dynamic_state;
	pipeline_info.layout              = pipeline_layout;

	VK_CHECK(vkCreateGraphicsPipelines(get_device_handle(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline));

	vkDestroyShaderModule(get_device_handle(), vert_module, nullptr);
	vkDestroyShaderModule(get_device_handle(), frag_module, nullptr);
}

/**
 * @brief Allocates per-frame resources (fence, acquire semaphore, command pool, command buffer).
 *
 * @param frame The PerFrame structure to initialize.
 */
void SwapchainPresentTiming::create_frame_resources(PerFrame &frame)
{
	VkFenceCreateInfo fence_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	VK_CHECK(vkCreateFence(get_device_handle(), &fence_info, nullptr, &frame.render_fence));

	VkSemaphoreCreateInfo sem_info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
	VK_CHECK(vkCreateSemaphore(get_device_handle(), &sem_info, nullptr, &frame.acquire_semaphore));

	VkCommandPoolCreateInfo cmd_pool_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
	cmd_pool_info.flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	cmd_pool_info.queueFamilyIndex = queue->get_family_index();
	VK_CHECK(vkCreateCommandPool(get_device_handle(), &cmd_pool_info, nullptr, &frame.command_pool));

	VkCommandBufferAllocateInfo cmd_buf_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
	cmd_buf_info.commandPool        = frame.command_pool;
	cmd_buf_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmd_buf_info.commandBufferCount = 1;
	VK_CHECK(vkAllocateCommandBuffers(get_device_handle(), &cmd_buf_info, &frame.command_buffer));
}

void SwapchainPresentTiming::destroy_frame_resources(PerFrame &frame)
{
	vkDestroyFence(get_device_handle(), frame.render_fence, nullptr);
	vkDestroySemaphore(get_device_handle(), frame.acquire_semaphore, nullptr);
	vkDestroyCommandPool(get_device_handle(), frame.command_pool, nullptr);
}

void SwapchainPresentTiming::init_swapchain()
{
	VkSwapchainKHR old_swapchain = swapchain;

	VkSwapchainCreateInfoKHR info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
	info.flags            = VK_SWAPCHAIN_CREATE_PRESENT_TIMING_BIT_EXT | VK_SWAPCHAIN_CREATE_PRESENT_ID_2_BIT_KHR;
	info.surface          = get_surface();
	info.minImageCount    = desired_swapchain_images;
	info.imageFormat      = surface_format.format;
	info.imageColorSpace  = surface_format.colorSpace;
	info.imageExtent      = swapchain_extents;
	info.imageArrayLayers = 1;
	info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.preTransform     = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	info.compositeAlpha   = composite;
	info.presentMode      = present_mode;
	info.clipped          = true;
	info.oldSwapchain     = old_swapchain;

	LOGI("Creating new swapchain");
	VK_CHECK(vkCreateSwapchainKHR(get_device_handle(), &info, nullptr, &swapchain));

	if (old_swapchain != VK_NULL_HANDLE)
	{
		get_device().wait_idle();
		vkDestroySwapchainKHR(get_device_handle(), old_swapchain, nullptr);
	}

	uint32_t image_count = 0;
	VK_CHECK(vkGetSwapchainImagesKHR(get_device_handle(), swapchain, &image_count, nullptr));

	std::vector<VkImage> images(image_count);
	VK_CHECK(vkGetSwapchainImagesKHR(get_device_handle(), swapchain, &image_count, images.data()));

	for (auto &img : swapchain_images)
	{
		if (img.image_view != VK_NULL_HANDLE)
		{
			vkDestroyImageView(get_device_handle(), img.image_view, nullptr);
		}
		if (img.render_semaphore != VK_NULL_HANDLE)
		{
			vkDestroySemaphore(get_device_handle(), img.render_semaphore, nullptr);
		}
	}

	swapchain_images.clear();
	swapchain_images.resize(image_count);

	for (uint32_t index = 0; index < image_count; ++index)
	{
		init_swapchain_image(swapchain_images[index], images[index]);
	}

	// Initialize present timing utilities.
	//
	// First, query the swapchain time domain properties to select an appropriate time domain.
	select_swapchain_time_domain();

	// Query the swapchain timing properties to get the refresh cycle duration. Note
	// this value may not be available on all platforms yet before presenting a few times.
	query_swapchain_timing_properties();

	// Clear the frame timing history in case we are reusing a swapchain.
	invalidate_timing_history();

	// Finally, compute a target present duration based on the current timing properties.
	select_target_present_duration();

	// Set a queue size for the swapchain to hold the present timing data. We'll poll results
	// every frame, but give it a decent buffer for results to be consistently available.
	VK_CHECK(vkSetSwapchainPresentTimingQueueSizeEXT(get_device_handle(), swapchain, history_buffer_size));
}

void SwapchainPresentTiming::init_swapchain_image(PerSwapchainImage &perImage, VkImage image)
{
	perImage.image = image;

	VkImageViewCreateInfo view_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};

	view_info.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
	view_info.format                      = surface_format.format;
	view_info.image                       = image;
	view_info.subresourceRange.levelCount = 1;
	view_info.subresourceRange.layerCount = 1;
	view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	view_info.components.r                = VK_COMPONENT_SWIZZLE_R;
	view_info.components.g                = VK_COMPONENT_SWIZZLE_G;
	view_info.components.b                = VK_COMPONENT_SWIZZLE_B;
	view_info.components.a                = VK_COMPONENT_SWIZZLE_A;

	VK_CHECK(vkCreateImageView(get_device_handle(), &view_info, nullptr, &perImage.image_view));

	VkSemaphoreCreateInfo sem_info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
	VK_CHECK(vkCreateSemaphore(get_device_handle(), &sem_info, nullptr, &perImage.render_semaphore));
}

bool SwapchainPresentTiming::recreate_swapchain()
{
	VkSurfaceCapabilitiesKHR surface_properties;
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(get_gpu_handle(), get_surface(), &surface_properties));

	// Only rebuild the swapchain if the dimensions have changed
	if (surface_properties.currentExtent.width == swapchain_extents.width &&
	    surface_properties.currentExtent.height == swapchain_extents.height)
	{
		return false;
	}

	if (surface_properties.currentExtent.width == 0xFFFFFFFF)
	{
		swapchain_extents = VkExtent2D{400, 300};
	}
	else
	{
		swapchain_extents = surface_properties.currentExtent;
	}

	init_swapchain();
	return true;
}

void SwapchainPresentTiming::setup_frame()
{
	frame_index     = (frame_index + 1) % frame_resources.size();
	PerFrame &frame = frame_resources[frame_index];

	// Wait for frame N-2 to finish before starting recording of frame N so we can reuse resources.
	vkWaitForFences(get_device_handle(), 1, &frame.render_fence, true, UINT64_MAX);
	vkResetFences(get_device_handle(), 1, &frame.render_fence);

	vkResetCommandPool(get_device_handle(), frame.command_pool, 0);

	// Collect presentation timings to update the display time
	query_past_presentation_timing();
}

/**
 * @brief Records and submits rendering commands for the given swapchain image.
 *
 * @param index Swapchain image index to render into.
 * @param delta_time Elapsed time since last frame (used when present timing is disabled).
 *
 * @return Target present time to pass to present_image (0 if present timing disabled).
 */
uint64_t SwapchainPresentTiming::render(uint32_t index, float delta_time)
{
	uint64_t  present_time   = 0;
	float     animation_time = 0.f;
	PerFrame &frame          = frame_resources[frame_index];

	// Compute the animation time by adding the appropriate multiple of our target
	// present duration to the last known display time.
	//
	// `present_time` will be returned and used to set the present timing info.
	if (can_use_present_timing)
	{
		const uint64_t id_delta    = present_id - display_time_present_id;
		const uint64_t time_offset = target_present_duration * id_delta;

		// Subtract display_time_ns_base for floating-point accuracy only; do not use
		// this base-adjusted value as an absolute targetTime for the presentation engine.
		animation_time = (float) ((display_time_ns - display_time_ns_base + time_offset) / (uint64_t) 1000000) / 1000.f;

		if (can_present_at_absolute_time)
		{
			present_time = display_time_ns ? display_time_ns + time_offset : 0;
		}
		else
		{
			present_time = target_present_duration;
		}
	}
	else
	{
		animation_time = cpu_time;
	}

	VkCommandBufferBeginInfo begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VK_CHECK(vkBeginCommandBuffer(frame.command_buffer, &begin_info));

	VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
	barrier.srcAccessMask                   = 0;
	barrier.dstAccessMask                   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	barrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout                       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
	barrier.image                           = swapchain_images[index].image;
	barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel   = 0;
	barrier.subresourceRange.levelCount     = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount     = 1;

	vkCmdPipelineBarrier(frame.command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	VkClearValue clear_values[1] = {};
	clear_values[0].color        = {{0.01f, 0.01f, 0.01f, 1.0f}};

	VkRenderingAttachmentInfo attachment_info{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
	attachment_info.imageView   = swapchain_images[index].image_view;
	attachment_info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachment_info.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment_info.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
	attachment_info.clearValue  = clear_values[0];

	VkRenderingInfo rendering_info{VK_STRUCTURE_TYPE_RENDERING_INFO};
	rendering_info.renderArea.extent    = swapchain_extents;
	rendering_info.layerCount           = 1;
	rendering_info.colorAttachmentCount = 1;
	rendering_info.pColorAttachments    = &attachment_info;

	vkCmdBeginRendering(frame.command_buffer, &rendering_info);
	vkCmdBindPipeline(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	VkViewport viewport{};
	viewport.x        = 0.0f;
	viewport.y        = 0.0f;
	viewport.width    = (float) swapchain_extents.width;
	viewport.height   = (float) swapchain_extents.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = swapchain_extents;

	vkCmdSetViewport(frame.command_buffer, 0, 1, &viewport);
	vkCmdSetScissor(frame.command_buffer, 0, 1, &scissor);

	// Linearly interpolate horizontal position over 1 second, so any stutter should
	// be easily perceptible
	const float pos_x = (animation_time - floorf(animation_time)) * 3.0f - 1.5f;

	PushConstants push_constants;
	push_constants.resolution = {(float) swapchain_extents.width, (float) swapchain_extents.height};
	push_constants.position   = {pos_x, 0.f, 0.0f};
	push_constants.color      = can_use_present_timing ? glm::vec3{0.2f, 0.7f, 0.3f} : glm::vec3{0.6f, 0.4f, 0.2f};
	vkCmdPushConstants(frame.command_buffer, pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &push_constants);
	vkCmdDraw(frame.command_buffer, 3, 1, 0, 0);

	vkCmdEndRendering(frame.command_buffer);

	barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	barrier.dstAccessMask = 0;
	barrier.oldLayout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barrier.newLayout     = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	vkCmdPipelineBarrier(frame.command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	VK_CHECK(vkEndCommandBuffer(frame.command_buffer));

	VkPipelineStageFlags wait_stage{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

	VkSubmitInfo info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
	info.commandBufferCount   = 1;
	info.pCommandBuffers      = &frame.command_buffer;
	info.waitSemaphoreCount   = 1;
	info.pWaitSemaphores      = &frame.acquire_semaphore;
	info.pWaitDstStageMask    = &wait_stage;
	info.signalSemaphoreCount = 1;
	info.pSignalSemaphores    = &swapchain_images[index].render_semaphore;

	VK_CHECK(vkQueueSubmit(queue->get_handle(), 1, &info, frame.render_fence));

	return present_time;
}

static VkResult ignore_suboptimal_due_to_rotation(VkResult result)
{
	// Because preTransform is not respected in this sample, VK_SUBOPTIMAL_KHR is returned if
	// the device is rotated.  Handling preTransform optimally is out of scope for this sample,
	// so VK_SUBOPTIMAL_KHR is ignored in that case.
	//
	// Note that on Android VK_SUBOPTIMAL_KHR is only returned when there is a mismatch between
	// the device rotation and the specified preTransform.
#if defined(ANDROID)
	if (result == VK_SUBOPTIMAL_KHR)
	{
		result = VK_SUCCESS;
	}
#endif
	return result;
}

VkResult SwapchainPresentTiming::acquire_next_image(uint32_t *index)
{
	PerFrame &frame  = frame_resources[frame_index];
	VkResult  result = vkAcquireNextImageKHR(get_device_handle(), swapchain, UINT64_MAX, frame.acquire_semaphore, VK_NULL_HANDLE, index);

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		return result;
	}

	return ignore_suboptimal_due_to_rotation(result);
}

/**
 * @brief Fills VkPresentTimingInfoEXT with target time, time domain, and present stage queries.
 *
 * @param timing_info Timing info structure that gets pNext-chained for vkQueuePresentKHR
 * @param present_time Target present time or duration, depending on device/surface support.
 */
void SwapchainPresentTiming::set_present_timing_info(VkPresentTimingInfoEXT &timing_info, uint64_t present_time)
{
	timing_info.targetTime                   = present_time;
	timing_info.flags                        = VK_PRESENT_TIMING_INFO_PRESENT_AT_NEAREST_REFRESH_CYCLE_BIT_EXT;
	timing_info.timeDomainId                 = time_domain.time_domain_id;
	timing_info.presentStageQueries          = display_present_stage | VK_PRESENT_STAGE_QUEUE_OPERATIONS_END_BIT_EXT | VK_PRESENT_STAGE_REQUEST_DEQUEUED_BIT_EXT;
	timing_info.targetTimeDomainPresentStage = display_present_stage;

	if (!can_present_at_absolute_time)
	{
		timing_info.flags |= VK_PRESENT_TIMING_INFO_PRESENT_AT_RELATIVE_TIME_BIT_EXT;
	}
}

/**
 * @brief Presents an image to the swapchain.
 *
 * @param index The swapchain image index previously obtained from @ref acquire_next_image.
 *
 * @returns Vulkan result of vkQueuePresentKHR, ignoring VK_SUBOPTIMAL_KHR
 */
VkResult SwapchainPresentTiming::present_image(uint32_t index, uint64_t present_time)
{
	VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
	present.swapchainCount     = 1;
	present.pSwapchains        = &swapchain;
	present.pImageIndices      = &index;
	present.waitSemaphoreCount = 1;
	present.pWaitSemaphores    = &swapchain_images[index].render_semaphore;

	VkPresentId2KHR         present_id_info{VK_STRUCTURE_TYPE_PRESENT_ID_2_KHR};
	VkPresentTimingsInfoEXT timings_info{VK_STRUCTURE_TYPE_PRESENT_TIMINGS_INFO_EXT};
	VkPresentTimingInfoEXT  timing_info{VK_STRUCTURE_TYPE_PRESENT_TIMING_INFO_EXT};

	present_id_info.swapchainCount = 1;
	present_id_info.pPresentIds    = &present_id;
	present.pNext                  = &present_id_info;

	if (can_use_present_timing)
	{
		set_present_timing_info(timing_info, present_time);
		timings_info.swapchainCount = 1;
		timings_info.pTimingInfos   = &timing_info;
		present_id_info.pNext       = &timings_info;
	}

	VkResult result = vkQueuePresentKHR(queue->get_handle(), &present);
	if (result == VK_ERROR_PRESENT_TIMING_QUEUE_FULL_EXT)
	{
		LOGI("Present timing queue is full, presenting {} without requesting timing information", present_id);
		timing_info.presentStageQueries = 0;
		result                          = vkQueuePresentKHR(queue->get_handle(), &present);
	}

	present_id++;

	return ignore_suboptimal_due_to_rotation(result);
}

VkPhysicalDevice SwapchainPresentTiming::get_gpu_handle()
{
	return get_device().get_gpu().get_handle();
}

VkDevice SwapchainPresentTiming::get_device_handle()
{
	if (!has_device())
	{
		return VK_NULL_HANDLE;
	}
	return get_device().get_handle();
}

uint32_t SwapchainPresentTiming::get_api_version() const
{
	return VK_API_VERSION_1_3;
}

void SwapchainPresentTiming::request_instance_extensions(std::unordered_map<std::string, vkb::RequestMode> &requested_extensions) const
{
	vkb::VulkanSampleC::request_instance_extensions(requested_extensions);
	requested_extensions[VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME] = vkb::RequestMode::Required;
}

SwapchainPresentTiming::SwapchainPresentTiming()
{
	// VK_EXT_present_timing dependencies
	add_device_extension(VK_KHR_PRESENT_ID_2_EXTENSION_NAME);
	add_device_extension(VK_EXT_PRESENT_TIMING_EXTENSION_NAME);
	add_device_extension(VK_KHR_CALIBRATED_TIMESTAMPS_EXTENSION_NAME);

	for (auto &timing : past_presentation_timings)
	{
		timing.sType             = VK_STRUCTURE_TYPE_PAST_PRESENTATION_TIMING_EXT;
		timing.pNext             = nullptr;
		timing.pPresentStages    = new VkPresentStageTimeEXT[max_present_stages];
		timing.presentStageCount = max_present_stages;
	}
}

SwapchainPresentTiming::~SwapchainPresentTiming()
{
	if (get_device_handle() == VK_NULL_HANDLE)
	{
		return;
	}

	// Wait for device to be idle and clean up everything.
	vkDeviceWaitIdle(get_device_handle());

	for (auto &timing : past_presentation_timings)
	{
		delete[] timing.pPresentStages;
	}

	for (PerFrame &frame : frame_resources)
	{
		destroy_frame_resources(frame);
	}

	if (pipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(get_device_handle(), pipeline, nullptr);
	}

	if (pipeline_layout != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(get_device_handle(), pipeline_layout, nullptr);
	}

	for (auto &img : swapchain_images)
	{
		if (img.image_view != VK_NULL_HANDLE)
		{
			vkDestroyImageView(get_device_handle(), img.image_view, nullptr);
		}
		if (img.render_semaphore != VK_NULL_HANDLE)
		{
			vkDestroySemaphore(get_device_handle(), img.render_semaphore, nullptr);
		}
	}

	if (swapchain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(get_device_handle(), swapchain, nullptr);
	}
}

void SwapchainPresentTiming::request_gpu_features(vkb::core::PhysicalDeviceC &gpu)
{
	REQUEST_REQUIRED_FEATURE(gpu, VkPhysicalDevicePresentTimingFeaturesEXT, presentTiming);
	REQUEST_REQUIRED_FEATURE(gpu, VkPhysicalDevicePresentId2FeaturesKHR, presentId2);

	// Not technically required but makes the reduces the amount of boilerplate
	REQUEST_REQUIRED_FEATURE(gpu, VkPhysicalDeviceDynamicRenderingFeatures, dynamicRendering);

	bool presentAtAbsoluteTime = REQUEST_OPTIONAL_FEATURE(gpu, VkPhysicalDevicePresentTimingFeaturesEXT, presentAtAbsoluteTime);
	bool presentAtRelativeTime = REQUEST_OPTIONAL_FEATURE(gpu, VkPhysicalDevicePresentTimingFeaturesEXT, presentAtRelativeTime);

	if (!presentAtAbsoluteTime && !presentAtRelativeTime)
	{
		throw std::runtime_error("Requested required feature <VkPhysicalDevicePresentTimingFeaturesEXT::presentAt*> is not supported");
	}
}

void SwapchainPresentTiming::create_render_context()
{
	get_queue();
	select_surface_format();
	query_surface_capabilities();
	create_pipeline();
	init_swapchain();
}

/**
 * @brief Prepares per-frame resources for all frames in the ring.
 */
void SwapchainPresentTiming::prepare_render_context()
{
	for (PerFrame &frame : frame_resources)
	{
		create_frame_resources(frame);
	}
}

/**
 * @brief Main update loop: acquire image, setup frame, render, present; handles swapchain recreation on out-of-date.
 * @param delta_time Time since last frame.
 */
void SwapchainPresentTiming::update(float delta_time)
{
	uint64_t next_present_time = 0;

	fps_timer += delta_time;
	cpu_time += delta_time;

	if (fps_timer > 1.0f)
	{
		LOGI("FPS: {} - target {}", static_cast<float>(present_id - fps_last_logged_frame_number) / fps_timer,
		     target_present_duration ? 1'000'000'000 / target_present_duration : 0);
		fps_timer -= 1.0f;
		fps_last_logged_frame_number = present_id;
	}

	setup_frame();

	uint32_t index;
	VkResult res = acquire_next_image(&index);

	if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreate_swapchain();
		res = acquire_next_image(&index);
	}
	if (res != VK_SUBOPTIMAL_KHR)
	{
		VK_CHECK(res);
	}

	next_present_time = render(index, delta_time);

	res = present_image(index, next_present_time);

	if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreate_swapchain();
	}
	else
	{
		VK_CHECK(res);
	}
}

bool SwapchainPresentTiming::resize(const uint32_t, const uint32_t)
{
	if (get_device_handle() == VK_NULL_HANDLE)
	{
		return false;
	}

	return recreate_swapchain();
}

void SwapchainPresentTiming::input_event(const vkb::InputEvent &input_event)
{
	if (input_event.get_source() != vkb::EventSource::Keyboard)
	{
		return;
	}

	const auto &key_button = static_cast<const vkb::KeyInputEvent &>(input_event);
	if (key_button.get_action() != vkb::KeyAction::Up)
	{
		return;
	}

	switch (key_button.get_code())
	{
		case vkb::KeyCode::P:
			can_use_present_timing = !can_use_present_timing;
			LOGI("can_use_present_timing: {}", can_use_present_timing);
			break;
		default:
			break;
	}
}

std::unique_ptr<vkb::Application> create_swapchain_present_timing()
{
	return std::make_unique<SwapchainPresentTiming>();
}
