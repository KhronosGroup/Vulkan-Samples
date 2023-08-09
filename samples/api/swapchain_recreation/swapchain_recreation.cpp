/* Copyright (c) 2023, Google
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

#include "swapchain_recreation.h"

#include "common/logging.h"
#include "common/vk_common.h"
#include "glsl_compiler.h"
#include "platform/filesystem.h"

static constexpr uint32_t INVALID_IMAGE_INDEX = std::numeric_limits<uint32_t>::max();

void SwapchainRecreation::get_queue()
{
	queue = &get_device().get_suitable_graphics_queue();

	// Make sure presentation is supported on this queue.  This is practically always the case;
	// if a platform/driver is found where this is not true, all queues supporting
	// VK_QUEUE_GRAPHICS_BIT need to be queried and one that supports presentation picked.
	VkBool32 supports_present = VK_FALSE;
	vkGetPhysicalDeviceSurfaceSupportKHR(get_gpu_handle(), queue->get_family_index(), get_surface(), &supports_present);

	if (!supports_present)
	{
		throw std::runtime_error("Default graphics queue does not support preesnt.");
	}
}

void SwapchainRecreation::query_surface_format()
{
	uint32_t surface_format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(get_gpu_handle(), get_surface(), &surface_format_count, nullptr);
	std::vector<VkSurfaceFormatKHR> supported_surface_formats(surface_format_count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(get_gpu_handle(), get_surface(), &surface_format_count, supported_surface_formats.data());

	// We want to get an SRGB image format that matches our list of preferred format candiates
	// We initialize to the first supported format, which will be the fallback in case none of the preferred formats is available
	surface_format             = supported_surface_formats[0];
	auto preferred_format_list = std::vector<VkFormat>{VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_A8B8G8R8_SRGB_PACK32};

	for (auto &candidate : supported_surface_formats)
	{
		if (std::find(preferred_format_list.begin(), preferred_format_list.end(), candidate.format) != preferred_format_list.end())
		{
			surface_format = candidate;
			break;
		}
	}
}

void SwapchainRecreation::query_present_modes()
{
	uint32_t present_mode_count = 0;
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(get_gpu_handle(), get_surface(), &present_mode_count, nullptr));

	present_modes.resize(present_mode_count);
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(get_gpu_handle(), get_surface(), &present_mode_count, present_modes.data()));

	adjust_desired_present_mode();
}

void SwapchainRecreation::adjust_desired_present_mode()
{
	// The FIFO present mode is guaranteed to be present.
	if (desired_present_mode == VK_PRESENT_MODE_FIFO_KHR)
	{
		return;
	}

	// When switching to MAILBOX, fallback to IMMEDIATE if not available and back to FIFO if
	// neither are available.
	if (desired_present_mode == VK_PRESENT_MODE_MAILBOX_KHR && std::find(present_modes.begin(), present_modes.end(), desired_present_mode) != present_modes.end())
	{
		return;
	}

	desired_present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
	if (std::find(present_modes.begin(), present_modes.end(), desired_present_mode) == present_modes.end())
	{
		LOGW("Neither MAILBOX nor IMMEDIATE are supported, falling back to FIFO");
		desired_present_mode = VK_PRESENT_MODE_FIFO_KHR;
	}
}

void SwapchainRecreation::create_render_pass()
{
	VkAttachmentDescription attachment = {0};
	attachment.format                  = surface_format.format;
	attachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
	attachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment.finalLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_ref = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

	VkSubpassDescription subpass = {0};
	subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments    = &color_ref;

	// Create a dependency from external such that srcStageMask matches WSI semaphore wait stage
	// (pWaitDstStageMask)
	VkSubpassDependency dependency = {0};
	dependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass          = 0;
	dependency.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask       = 0;
	dependency.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency.dependencyFlags     = 0;

	VkRenderPassCreateInfo rp_info = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
	rp_info.attachmentCount        = 1;
	rp_info.pAttachments           = &attachment;
	rp_info.subpassCount           = 1;
	rp_info.pSubpasses             = &subpass;
	rp_info.dependencyCount        = 1;
	rp_info.pDependencies          = &dependency;

	VK_CHECK(vkCreateRenderPass(get_device_handle(), &rp_info, nullptr, &render_pass));
}

/**
 * @brief Initializes the Vulkan swapchain.
 */
void SwapchainRecreation::init_swapchain()
{
	VkSurfaceCapabilitiesKHR surface_properties;
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(get_gpu_handle(), get_surface(), &surface_properties));

	if (surface_properties.currentExtent.width == 0xFFFFFFFF)
	{
		swapchain_extents = VkExtent2D{400, 300};
	}
	else
	{
		swapchain_extents = surface_properties.currentExtent;
	}

	// Do tripple-buffering when possible.  This is clamped to the min and max image count limits.
	uint32_t desired_swapchain_images = std::max(surface_properties.minImageCount, 3u);
	if (surface_properties.maxImageCount > 0)
	{
		desired_swapchain_images = std::min(desired_swapchain_images, surface_properties.maxImageCount);
	}

	// Find a supported composite type.
	VkCompositeAlphaFlagBitsKHR composite = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	if (surface_properties.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
	{
		composite = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	}
	else if (surface_properties.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
	{
		composite = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
	}
	else if (surface_properties.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
	{
		composite = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
	}
	else if (surface_properties.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
	{
		composite = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
	}

	VkSwapchainKHR old_swapchain = swapchain;

	VkSwapchainCreateInfoKHR info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
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
	info.presentMode      = desired_present_mode;
	info.clipped          = true;
	info.oldSwapchain     = old_swapchain;

	// Note: the above info sets preTransform to `VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR`.  This
	// is **not optimal** on devices that support rotation and will lead to measurable
	// performance loss.  It is strongly recommended that `surface_properties.currentTransform`
	// be used instead.  However, the application is required to handle preTransform elsewhere
	// accordingly.

	VK_CHECK(vkCreateSwapchainKHR(get_device_handle(), &info, nullptr, &swapchain));

	current_present_mode = desired_present_mode;

	// Schedule destruction of the old swapchain resources once this frame's submission is finished.
	submit_history[submit_history_index].swapchain_garbage.push_back(std::move(swapchain_objects));

	// Schedule destruction of the old swapchain itself once its last presentation is finished.
	if (old_swapchain != VK_NULL_HANDLE)
	{
		schedule_old_swapchain_for_destruction(old_swapchain);
	}

	// Get the swapchain images.
	uint32_t image_count;
	VK_CHECK(vkGetSwapchainImagesKHR(get_device_handle(), swapchain, &image_count, nullptr));

	swapchain_objects.images.resize(image_count, VK_NULL_HANDLE);
	swapchain_objects.views.resize(image_count, VK_NULL_HANDLE);
	swapchain_objects.framebuffers.resize(image_count, VK_NULL_HANDLE);
	VK_CHECK(vkGetSwapchainImagesKHR(get_device_handle(), swapchain, &image_count, swapchain_objects.images.data()));

	for (uint32_t index = 0; index < image_count; ++index)
	{
		init_swapchain_image(index);
	}
}

/**
 * @brief Called to initialize resources for a swapchain image.
 */
void SwapchainRecreation::init_swapchain_image(uint32_t index)
{
	assert(swapchain_objects.views[index] == VK_NULL_HANDLE);

	VkImageViewCreateInfo view_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
	view_info.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
	view_info.format                      = surface_format.format;
	view_info.image                       = swapchain_objects.images[index];
	view_info.subresourceRange.levelCount = 1;
	view_info.subresourceRange.layerCount = 1;
	view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	view_info.components.r                = VK_COMPONENT_SWIZZLE_R;
	view_info.components.g                = VK_COMPONENT_SWIZZLE_G;
	view_info.components.b                = VK_COMPONENT_SWIZZLE_B;
	view_info.components.a                = VK_COMPONENT_SWIZZLE_A;

	VK_CHECK(vkCreateImageView(get_device_handle(), &view_info, nullptr, &swapchain_objects.views[index]));

	VkFramebufferCreateInfo fb_info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
	fb_info.renderPass      = render_pass;
	fb_info.attachmentCount = 1;
	fb_info.pAttachments    = &swapchain_objects.views[index];
	fb_info.width           = swapchain_extents.width;
	fb_info.height          = swapchain_extents.height;
	fb_info.layers          = 1;

	VK_CHECK(vkCreateFramebuffer(get_device_handle(), &fb_info, nullptr, &swapchain_objects.framebuffers[index]));
}

/**
 * @brief When a swapchain is retired, the resources associated with its images are scheduled to be
 * cleaned up as soon as the last submission using those images is complete.  This function is
 * called at such a moment.
 *
 * The swapchain itself is not destroyed until known safe.
 */
void SwapchainRecreation::cleanup_swapchain_objects(SwapchainObjects &garbage)
{
	for (VkImageView view : garbage.views)
	{
		vkDestroyImageView(get_device_handle(), view, nullptr);
	}
	for (VkFramebuffer framebuffer : garbage.framebuffers)
	{
		vkDestroyFramebuffer(get_device_handle(), framebuffer, nullptr);
	}

	garbage = {};
}

bool SwapchainRecreation::recreate_swapchain()
{
	VkSurfaceCapabilitiesKHR surface_properties;
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(get_gpu_handle(), get_surface(), &surface_properties));

	// Only rebuild the swapchain if the dimensions have changed
	if (surface_properties.currentExtent.width == swapchain_extents.width &&
	    surface_properties.currentExtent.height == swapchain_extents.height &&
	    desired_present_mode == current_present_mode)
	{
		return false;
	}

	init_swapchain();
	return true;
}

void SwapchainRecreation::setup_frame()
{
	// For the frame we need:
	// - A fence for the submission
	// - A semaphore for image acquire
	// - A semaphore for image present

	// But first, pace the CPU.  Wait for frame N-2 to finish before starting recording of frame N.
	submit_history_index = (submit_history_index + 1) % submit_history.size();
	PerFrame &frame      = submit_history[submit_history_index];
	if (frame.submit_fence != VK_NULL_HANDLE)
	{
		vkWaitForFences(get_device_handle(), 1, &frame.submit_fence, true, UINT64_MAX);

		// Reset/recycle resources, they are no longer in use.
		recycle_fence(frame.submit_fence);
		recycle_semaphore(frame.acquire_semaphore);
		vkResetCommandPool(get_device_handle(), frame.command_pool, 0);

		// Destroy any garbage that's associated with this submission.
		for (SwapchainObjects &garbage : frame.swapchain_garbage)
		{
			cleanup_swapchain_objects(garbage);
		}
		frame.swapchain_garbage.clear();

		// Note that while the submission fence, the semaphore it waited on and the command
		// pool its command was allocated from are guaranteed to have finished execution,
		// there is no guarantee that the present semaphore is not in use.
		//
		// This is because the fence wait above ensures that the submission _before_ present
		// is finished, but makes no guarantees as to the state of the present operation
		// that follows.  The present semaphore is queued for garbage collection when
		// possible after present, and is not kept as part of the submit history.
		assert(frame.present_semaphore == VK_NULL_HANDLE);
	}

	frame.submit_fence      = get_fence();
	frame.acquire_semaphore = get_semaphore();
	frame.present_semaphore = get_semaphore();

	if (frame.command_pool == VK_NULL_HANDLE)
	{
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
}

void SwapchainRecreation::render(uint32_t index)
{
	PerFrame &frame = submit_history[submit_history_index];

	VkCommandBufferBeginInfo begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(frame.command_buffer, &begin_info);

	// Render the following with basic vkCmdClearAttachments calls:
	// - A gray rectangle that scales with the size of the extent
	// - A fixed size square with changing color based on FPS

	VkClearValue black;
	black.color = {{0, 0, 0, 1.0f}};

	VkClearValue gray;
	gray.color = {{0.5f, 0.5f, 0.5f, 1.0f}};

	VkClearValue colorful;
	colorful.color = {{
	    static_cast<float>(frame_number % 256) / 255.0f,
	    static_cast<float>((frame_number + 63) % 256) / 255.0f,
	    static_cast<float>((frame_number + 128) % 256) / 255.0f,
	    1.0f,
	}};

	VkRenderPassBeginInfo rp_begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
	rp_begin.renderPass        = render_pass;
	rp_begin.framebuffer       = swapchain_objects.framebuffers[index];
	rp_begin.renderArea.extent = swapchain_extents;
	rp_begin.clearValueCount   = 1;
	rp_begin.pClearValues      = &black;
	vkCmdBeginRenderPass(frame.command_buffer, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

	VkClearAttachment gray_clear;
	gray_clear.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
	gray_clear.colorAttachment = 0;
	gray_clear.clearValue      = gray;

	VkClearAttachment colorful_clear = gray_clear;
	colorful_clear.clearValue        = colorful;

	const uint32_t half_width  = swapchain_extents.width / 2;
	const uint32_t half_height = swapchain_extents.height / 2;

	VkClearRect gray_rect;
	gray_rect.rect.offset    = {static_cast<int32_t>(half_width) / 2, static_cast<int32_t>(half_height)};
	gray_rect.rect.extent    = {std::max(half_width, 1u), std::max(half_height / 2, 1u)};
	gray_rect.baseArrayLayer = 0;
	gray_rect.layerCount     = 1;

	constexpr int32_t  colorful_rect_x      = 250;
	constexpr int32_t  colorful_rect_y      = 150;
	constexpr uint32_t colorful_rect_width  = 300;
	constexpr uint32_t colorful_rect_height = 350;

	VkClearRect colorful_rect = gray_rect;
	colorful_rect.rect.offset = {colorful_rect_x, colorful_rect_y};
	colorful_rect.rect.extent = {colorful_rect_width, colorful_rect_height};

	// Draw two rectangles via vkCmdClearAttachments.  The gray rectangle scales with the
	// window, but the colorful one has fixed size, and it's skipped if the window is too small.
	vkCmdClearAttachments(frame.command_buffer, 1, &gray_clear, 1, &gray_rect);
	if (colorful_rect_x + colorful_rect_width <= swapchain_extents.width &&
	    colorful_rect_y + colorful_rect_height <= swapchain_extents.height)
	{
		vkCmdClearAttachments(frame.command_buffer, 1, &colorful_clear, 1, &colorful_rect);
	}

	vkCmdEndRenderPass(frame.command_buffer);
	VK_CHECK(vkEndCommandBuffer(frame.command_buffer));

	// Make a submission. Wait on the acquire semaphore and signal the present semaphore.
	VkPipelineStageFlags wait_stage{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

	VkSubmitInfo info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
	info.commandBufferCount   = 1;
	info.pCommandBuffers      = &frame.command_buffer;
	info.waitSemaphoreCount   = 1;
	info.pWaitSemaphores      = &frame.acquire_semaphore;
	info.pWaitDstStageMask    = &wait_stage;
	info.signalSemaphoreCount = 1;
	info.pSignalSemaphores    = &frame.present_semaphore;
	VK_CHECK(vkQueueSubmit(queue->get_handle(), 1, &info, frame.submit_fence));
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

/**
 * @brief Acquires an image from the swapchain.
 * @param[out] index The swapchain index for the acquired image.
 * @returns Vulkan result code
 */
VkResult SwapchainRecreation::acquire_next_image(uint32_t *index)
{
	PerFrame &frame         = submit_history[submit_history_index];
	VkFence   acquire_fence = get_fence();

	VkResult result = vkAcquireNextImageKHR(get_device_handle(), swapchain, UINT64_MAX, frame.acquire_semaphore, acquire_fence, index);

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		// If failed, fence is untouched, recycle it.
		//
		// The semaphore is also untouched, but it may be used in the retry of
		// vkAcquireNextImageKHR.  It is nevertheless cleaned up after cpu throttling
		// automatically.
		recycle_fence(acquire_fence);
		return result;
	}

	associate_fence_with_present_history(*index, acquire_fence);

	return ignore_suboptimal_due_to_rotation(result);
}

/**
 * @brief Presents an image to the swapchain.
 * @param index The swapchain index previously obtained from @ref acquire_next_image.
 * @returns Vulkan result code
 */
VkResult SwapchainRecreation::present_image(uint32_t index)
{
	PerFrame &frame = submit_history[submit_history_index];

	VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
	present.swapchainCount     = 1;
	present.pSwapchains        = &swapchain;
	present.pImageIndices      = &index;
	present.waitSemaphoreCount = 1;
	present.pWaitSemaphores    = &frame.present_semaphore;

	VkResult result = vkQueuePresentKHR(queue->get_handle(), &present);

	add_present_to_history(index);
	cleanup_present_history();

	return ignore_suboptimal_due_to_rotation(result);
}

void SwapchainRecreation::add_present_to_history(uint32_t index)
{
	PerFrame &frame = submit_history[submit_history_index];

	present_history.emplace_back();
	present_history.back().present_semaphore = frame.present_semaphore;
	present_history.back().old_swapchains    = std::move(old_swapchains);

	frame.present_semaphore = VK_NULL_HANDLE;

	// The fence needed to know when the semaphore can be recycled will be one that is passed to
	// vkAcquireNextImageKHR that returns the same image index.  That is why the image index
	// needs to be tracked in this case.
	present_history.back().image_index = index;
}

void SwapchainRecreation::cleanup_present_history()
{
	while (!present_history.empty())
	{
		PresentOperationInfo &present_info = present_history.front();

		// If there is no fence associated with the history, it can't be cleaned up yet.
		if (present_info.cleanup_fence == VK_NULL_HANDLE)
		{
			// Can't have an old present operation without a fence that doesn't have an
			// image index used to later associate a fence with it.
			assert(present_info.image_index != INVALID_IMAGE_INDEX);
			break;
		}

		// Otherwise check to see if the fence is signaled.
		VkResult result = vkGetFenceStatus(get_device_handle(), present_info.cleanup_fence);
		if (result == VK_NOT_READY)
		{
			// Not yet
			break;
		}
		VK_CHECK(result);

		cleanup_present_info(present_info);
		present_history.pop_front();
	}

	// The present history can grow indefinitely if a present operation is done on an index
	// that's never acquired in the future.  In that case, there's no fence associated with that
	// present operation.  Move the offending entry to last, so the resources associated with
	// the rest of the present operations can be duly freed.
	if (present_history.size() > swapchain_objects.images.size() * 2 && present_history.front().cleanup_fence == VK_NULL_HANDLE)
	{
		PresentOperationInfo present_info = std::move(present_history.front());
		present_history.pop_front();

		// We can't be stuck on a presentation to an old swapchain without a fence.
		assert(present_info.image_index != INVALID_IMAGE_INDEX);

		// Move clean up data to the next (now first) present operation, if any.  Note that
		// there cannot be any clean up data on the rest of the present operations, because
		// the first present already gathers every old swapchain to clean up.
		assert(std::all_of(present_history.begin(), present_history.end(), [](const PresentOperationInfo &op) {
			return op.old_swapchains.empty();
		}));
		present_history.front().old_swapchains = std::move(present_info.old_swapchains);

		// Put the present operation at the end of the queue, so it's revisited after the
		// rest of the present operations are cleaned up.
		present_history.push_back(std::move(present_info));
	}
}

void SwapchainRecreation::cleanup_present_info(PresentOperationInfo &present_info)
{
	// Called when it's safe to destroy resources associated with a present operation.
	if (present_info.cleanup_fence != VK_NULL_HANDLE)
	{
		recycle_fence(present_info.cleanup_fence);
	}

	// On the first acquire of the image, a fence is used but there is no present semaphore to
	// clean up.  That fence is placed in the present history just for clean up purposes.
	if (present_info.present_semaphore != VK_NULL_HANDLE)
	{
		recycle_semaphore(present_info.present_semaphore);
	}

	// Destroy old swapchains
	for (SwapchainCleanupData &old_swapchain : present_info.old_swapchains)
	{
		cleanup_old_swapchain(old_swapchain);
	}

	present_info = {};
}

void SwapchainRecreation::cleanup_old_swapchain(SwapchainCleanupData &old_swapchain)
{
	if (old_swapchain.swapchain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(get_device_handle(), old_swapchain.swapchain, nullptr);
	}

	for (VkSemaphore semaphore : old_swapchain.semaphores)
	{
		recycle_semaphore(semaphore);
	}

	old_swapchain = {};
}

void SwapchainRecreation::associate_fence_with_present_history(uint32_t index, VkFence acquire_fence)
{
	// The history looks like this:
	//
	// <entries for old swapchains, imageIndex == UINT32_MAX> <entries for this swapchain>
	//
	// Walk the list backwards and find the entry for the given image index.  That's the last
	// present with that image.  Associate the fence with that present operation.
	for (size_t history_index = 0; history_index < present_history.size(); ++history_index)
	{
		PresentOperationInfo &present_info =
		    present_history[present_history.size() - history_index - 1];
		if (present_info.image_index == INVALID_IMAGE_INDEX)
		{
			// No previous presentation with this index.
			break;
		}

		if (present_info.image_index == index)
		{
			assert(present_info.cleanup_fence == VK_NULL_HANDLE);
			present_info.cleanup_fence = acquire_fence;
			return;
		}
	}

	// If no previous presentation with this index, add an empty entry just so the fence can be
	// cleaned up.
	present_history.emplace_back();
	present_history.back().cleanup_fence = acquire_fence;
	present_history.back().image_index   = index;
}

void SwapchainRecreation::schedule_old_swapchain_for_destruction(VkSwapchainKHR old_swapchain)
{
	// If no presentation is done on the swapchain, destroy it right away.
	if (!present_history.empty() && present_history.back().image_index == INVALID_IMAGE_INDEX)
	{
		vkDestroySwapchainKHR(get_device_handle(), old_swapchain, nullptr);
		return;
	}

	SwapchainCleanupData cleanup;
	cleanup.swapchain = old_swapchain;

	// Place any present operation that's not associated with a fence into old_swapchains.  That
	// gets scheduled for destruction when the semaphore of the first image of the next
	// swapchain can be recycled.
	std::vector<PresentOperationInfo> history_to_keep;
	while (!present_history.empty())
	{
		PresentOperationInfo &present_info = present_history.back();

		// If this is about an older swapchain, let it be.
		if (present_info.image_index == INVALID_IMAGE_INDEX)
		{
			assert(present_info.cleanup_fence != VK_NULL_HANDLE);
			break;
		}

		// Reset the index, so it's not processed in the future.
		present_info.image_index = INVALID_IMAGE_INDEX;

		if (present_info.cleanup_fence != VK_NULL_HANDLE)
		{
			// If there is already a fence associated with it, let it be cleaned up once
			// the fence is signaled.
			history_to_keep.push_back(std::move(present_info));
		}
		else
		{
			assert(present_info.present_semaphore != VK_NULL_HANDLE);

			// Otherwise accumulate it in cleanup data.
			cleanup.semaphores.push_back(present_info.present_semaphore);

			// Accumulate any previous swapchains that are pending destruction too.
			for (SwapchainCleanupData &swapchain : present_info.old_swapchains)
			{
				old_swapchains.emplace_back(swapchain);
			}
			present_info.old_swapchains.clear();
		}

		present_history.pop_back();
	}
	std::move(history_to_keep.begin(), history_to_keep.end(), std::back_inserter(present_history));

	if (cleanup.swapchain != VK_NULL_HANDLE || !cleanup.semaphores.empty())
	{
		old_swapchains.emplace_back(std::move(cleanup));
	}
}

VkSemaphore SwapchainRecreation::get_semaphore()
{
	// If there is a free semaphore, return it
	if (!semaphore_pool.empty())
	{
		VkSemaphore semaphore = semaphore_pool.back();
		semaphore_pool.pop_back();
		return semaphore;
	}

	VkSemaphore           semaphore = VK_NULL_HANDLE;
	VkSemaphoreCreateInfo create_info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

	VK_CHECK(vkCreateSemaphore(get_device_handle(), &create_info, nullptr, &semaphore));

	return semaphore;
}

void SwapchainRecreation::recycle_semaphore(VkSemaphore semaphore)
{
	semaphore_pool.push_back(semaphore);
}

VkFence SwapchainRecreation::get_fence()
{
	// If there is a free fence, return it
	if (!fence_pool.empty())
	{
		VkFence fence = fence_pool.back();
		fence_pool.pop_back();
		return fence;
	}

	VkFence           fence = VK_NULL_HANDLE;
	VkFenceCreateInfo create_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};

	VK_CHECK(vkCreateFence(get_device_handle(), &create_info, nullptr, &fence));

	return fence;
}

void SwapchainRecreation::recycle_fence(VkFence fence)
{
	fence_pool.push_back(fence);

	VK_CHECK(vkResetFences(get_device_handle(), 1, &fence));
}

VkPhysicalDevice SwapchainRecreation::get_gpu_handle()
{
	return get_device().get_gpu().get_handle();
}

VkDevice SwapchainRecreation::get_device_handle()
{
	return get_device().get_handle();
}

SwapchainRecreation::SwapchainRecreation()
{
}

SwapchainRecreation::~SwapchainRecreation()
{
	// Wait for device to be idle and clean up everything.
	vkDeviceWaitIdle(get_device_handle());

	for (PerFrame &frame : submit_history)
	{
		recycle_fence(frame.submit_fence);
		recycle_semaphore(frame.acquire_semaphore);
		vkDestroyCommandPool(get_device_handle(), frame.command_pool, nullptr);
		for (SwapchainObjects &garbage : frame.swapchain_garbage)
		{
			cleanup_swapchain_objects(garbage);
		}
		frame.swapchain_garbage.clear();
		assert(frame.present_semaphore == VK_NULL_HANDLE);
	}

	for (PresentOperationInfo &present_info : present_history)
	{
		if (present_info.cleanup_fence != VK_NULL_HANDLE)
		{
			vkWaitForFences(get_device_handle(), 1, &present_info.cleanup_fence, true, UINT64_MAX);
		}
		cleanup_present_info(present_info);
	}

	LOGI("Old swapchain count at destruction: {}", old_swapchains.size());

	for (SwapchainCleanupData &old_swapchain : old_swapchains)
	{
		cleanup_old_swapchain(old_swapchain);
	}

	LOGI("Semaphore pool size at destruction: {}", semaphore_pool.size());
	LOGI("Fence pool size at destruction: {}", fence_pool.size());

	for (VkSemaphore semaphore : semaphore_pool)
	{
		vkDestroySemaphore(get_device_handle(), semaphore, nullptr);
	}

	for (VkFence fence : fence_pool)
	{
		vkDestroyFence(get_device_handle(), fence, nullptr);
	}

	cleanup_swapchain_objects(swapchain_objects);
	if (swapchain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(get_device_handle(), swapchain, nullptr);
	}

	if (render_pass != VK_NULL_HANDLE)
	{
		vkDestroyRenderPass(get_device_handle(), render_pass, nullptr);
	}
}

bool SwapchainRecreation::prepare(const vkb::ApplicationOptions &options)
{
	if (!VulkanSample::prepare(options))
	{
		return false;
	}

	LOGI("USAGE:");
	LOGI(" - Press v to enable v-sync");
	LOGI(" - Press n to disable v-sync");

	return true;
}

void SwapchainRecreation::create_render_context()
{
	get_queue();
	query_surface_format();
	create_render_pass();
	init_swapchain();
}

void SwapchainRecreation::prepare_render_context()
{
	// Nothing to do
}

void SwapchainRecreation::update(float delta_time)
{
	fps_timer += delta_time;
	if (fps_timer > 1.0f)
	{
		LOGI("FPS: {}", static_cast<float>(frame_number - fps_last_logged_frame_number) / fps_timer);
		fps_timer -= 1.0f;
		fps_last_logged_frame_number = frame_number;
	}

	++frame_number;

	setup_frame();

	if (desired_present_mode != current_present_mode)
	{
		recreate_swapchain();
	}

	uint32_t index;
	auto     res = acquire_next_image(&index);

	// Handle outdated error in acquire.
	if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreate_swapchain();
		res = acquire_next_image(&index);
	}
	if (res != VK_SUBOPTIMAL_KHR)
	{
		VK_CHECK(res);
	}

	render(index);
	res = present_image(index);

	// Handle Outdated error in present.
	if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreate_swapchain();
	}
	else
	{
		VK_CHECK(res);
	}
}

bool SwapchainRecreation::resize(const uint32_t, const uint32_t)
{
	if (get_device_handle() == VK_NULL_HANDLE)
	{
		return false;
	}

	return recreate_swapchain();
}

void SwapchainRecreation::input_event(const vkb::InputEvent &input_event)
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
		case vkb::KeyCode::V:
			// Note: events are being double-sent, avoid double logging with this check
			// as a workaround.
			if (current_present_mode != VK_PRESENT_MODE_FIFO_KHR)
			{
				LOGI("Enabling V-Sync");
				desired_present_mode = VK_PRESENT_MODE_FIFO_KHR;
			}
			break;
		case vkb::KeyCode::N:
			if (current_present_mode == VK_PRESENT_MODE_FIFO_KHR)
			{
				LOGI("Disabling V-Sync");
				desired_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
			}
			break;
		default:
			break;
	}

	query_present_modes();
}

std::unique_ptr<vkb::Application> create_swapchain_recreation()
{
	return std::make_unique<SwapchainRecreation>();
}
