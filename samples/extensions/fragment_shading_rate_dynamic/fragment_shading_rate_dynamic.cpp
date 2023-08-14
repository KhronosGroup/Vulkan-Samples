/* Copyright (c) 2021-2023, Holochip
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

#include "fragment_shading_rate_dynamic.h"

FragmentShadingRateDynamic::FragmentShadingRateDynamic() :
    pipeline_layout(VK_NULL_HANDLE), pipelines(), descriptor_set_layout(), subpass_extent()
{
	title = "Dynamic fragment shading rate";

	(void) ubo_scene.skysphere_modelview;        // this is used in the shader

	// Enable instance and device extensions required to use VK_KHR_fragment_shading_rate
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	add_device_extension(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);
	add_device_extension(VK_KHR_MULTIVIEW_EXTENSION_NAME);
	add_device_extension(VK_KHR_MAINTENANCE2_EXTENSION_NAME);
	add_device_extension(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME);
}

FragmentShadingRateDynamic::~FragmentShadingRateDynamic()
{
	if (device)
	{
		vkDestroyPipeline(get_device().get_handle(), pipelines.sphere, nullptr);
		vkDestroyPipeline(get_device().get_handle(), pipelines.skysphere, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, nullptr);
		vkDestroySampler(get_device().get_handle(), textures.skysphere.sampler, nullptr);
		vkDestroySampler(get_device().get_handle(), textures.scene.sampler, nullptr);

		vkDestroyRenderPass(get_device().get_handle(), fragment_render_pass, nullptr);
		for (auto &&framebuffer : fragment_framebuffers)
		{
			vkDestroyFramebuffer(device->get_handle(), framebuffer, nullptr);
		}
		fragment_framebuffers.clear();

		vkDestroyFence(device->get_handle(), compute_fence, VK_NULL_HANDLE);
		uniform_buffers.scene.reset();
		invalidate_shading_rate_attachment();
		vkDestroyDescriptorPool(device->get_handle(), compute.descriptor_pool, nullptr);
		compute_buffers.clear();
		frequency_information_params.reset();
		vkDestroyCommandPool(get_device().get_handle(), command_pool, VK_NULL_HANDLE);
	}
}

void FragmentShadingRateDynamic::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	// Enable the shading rate attachment feature required by this sample
	// These are passed to device creation via a pNext structure chain
	auto &requested_extension_features = gpu.request_extension_features<VkPhysicalDeviceFragmentShadingRateFeaturesKHR>(
	    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR);
	requested_extension_features.attachmentFragmentShadingRate = VK_TRUE;
	requested_extension_features.pipelineFragmentShadingRate   = VK_TRUE;
	requested_extension_features.primitiveFragmentShadingRate  = VK_FALSE;

	// Enable anisotropic filtering if supported
	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = VK_TRUE;
	}
}

void FragmentShadingRateDynamic::create_shading_rate_attachment()
{
	// Deallocate any existing memory so that it can be reused
	compute_buffers.clear();
	compute_buffers.resize(draw_cmd_buffers.size());

	subpass_extent = VkExtent2D{
	    static_cast<uint32_t>(ceil(static_cast<float>(width) / static_cast<float>(subpass_extent_ratio))),
	    static_cast<uint32_t>(ceil(static_cast<float>(height) / static_cast<float>(subpass_extent_ratio)))};

	for (auto &&compute_buffer : compute_buffers)
	{
		auto &shading_rate_image              = compute_buffer.shading_rate_image;
		auto &shading_rate_image_view         = compute_buffer.shading_rate_image_view;
		auto &frequency_content_image         = compute_buffer.frequency_content_image;
		auto &frequency_content_image_view    = compute_buffer.frequency_content_image_view;
		auto &shading_rate_image_compute      = compute_buffer.shading_rate_image_compute;
		auto &shading_rate_image_compute_view = compute_buffer.shading_rate_image_compute_view;

		const VkFormat     requested_format = VK_FORMAT_R8_UINT;
		VkFormatProperties format_properties;
		vkGetPhysicalDeviceFormatProperties(device->get_gpu().get_handle(), requested_format, &format_properties);
		if (!(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR))
		{
			throw std::runtime_error("Shading rate attachment image does not support required format feature flag.");
		}

		// The shading rate image will be smaller than the frame width and height,
		// which we label here for clarity
		const uint32_t frame_width = width, frame_height = height;
		VkExtent3D     image_extent{};
		image_extent.width  = static_cast<uint32_t>(ceil(static_cast<float>(frame_width) /
		                                                 static_cast<float>(physical_device_fragment_shading_rate_properties.maxFragmentShadingRateAttachmentTexelSize.width)));
		image_extent.height = static_cast<uint32_t>(ceil(static_cast<float>(frame_height) /
		                                                 static_cast<float>(physical_device_fragment_shading_rate_properties.maxFragmentShadingRateAttachmentTexelSize.height)));
		image_extent.depth  = 1;

		auto create_shading_rate = [&](VkImageUsageFlags image_usage, VkFormat format) {
			return std::make_unique<vkb::core::Image>(*device,
			                                          image_extent,
			                                          format,
			                                          image_usage,
			                                          VMA_MEMORY_USAGE_GPU_ONLY,
			                                          VK_SAMPLE_COUNT_1_BIT);
		};

		shading_rate_image         = create_shading_rate(VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR |
		                                                     static_cast<VkImageUsageFlags>(VK_BUFFER_USAGE_TRANSFER_DST_BIT),
		                                                 VK_FORMAT_R8_UINT);
		shading_rate_image_compute = create_shading_rate(
		    VK_IMAGE_USAGE_STORAGE_BIT | static_cast<VkImageUsageFlags>(VK_BUFFER_USAGE_TRANSFER_SRC_BIT),
		    VK_FORMAT_R8_UINT);

		uint32_t fragment_shading_rate_count = 0;
		vkGetPhysicalDeviceFragmentShadingRatesKHR(get_device().get_gpu().get_handle(), &fragment_shading_rate_count,
		                                           nullptr);
		if (fragment_shading_rate_count > 0)
		{
			fragment_shading_rates.resize(fragment_shading_rate_count);
			for (VkPhysicalDeviceFragmentShadingRateKHR &fragment_shading_rate : fragment_shading_rates)
			{
				fragment_shading_rate.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_KHR;
			}
			vkGetPhysicalDeviceFragmentShadingRatesKHR(get_device().get_gpu().get_handle(),
			                                           &fragment_shading_rate_count, fragment_shading_rates.data());
		}

		// initialize to the lowest shading rate, equal to (min_shading_rate >> 1) | (min_shading_rate << 1));
		const auto           min_shading_rate = fragment_shading_rates.front().fragmentSize;
		std::vector<uint8_t> temp_buffer(frame_height * frame_width,
		                                 (min_shading_rate.height >> 1) | ((min_shading_rate.width << 1) & 12));
		auto                 staging_buffer = std::make_unique<vkb::core::Buffer>(*device, temp_buffer.size() * sizeof(temp_buffer[0]),
                                                                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                                  VMA_MEMORY_USAGE_CPU_TO_GPU);
		staging_buffer->update(temp_buffer);

		auto cmd = device->create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		vkb::image_layout_transition(cmd, shading_rate_image->get_handle(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		VkBufferImageCopy buffer_copy_region           = {};
		buffer_copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		buffer_copy_region.imageSubresource.layerCount = 1;
		buffer_copy_region.imageExtent                 = image_extent;
		vkCmdCopyBufferToImage(cmd, staging_buffer->get_handle(), shading_rate_image->get_handle(),
		                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_copy_region);

		vkb::image_layout_transition(
		    cmd, shading_rate_image->get_handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR);

		VK_CHECK(vkEndCommandBuffer(cmd));

		auto submit               = vkb::initializers::submit_info();
		submit.commandBufferCount = 1;
		submit.pCommandBuffers    = &cmd;

		auto fence = device->request_fence();
		VK_CHECK(vkQueueSubmit(queue, 1, &submit, fence));
		VK_CHECK(vkWaitForFences(device->get_handle(), 1, &fence, VK_TRUE, UINT64_MAX));

		shading_rate_image_view         = std::make_unique<vkb::core::ImageView>(*shading_rate_image, VK_IMAGE_VIEW_TYPE_2D,
                                                                         VK_FORMAT_R8_UINT);
		shading_rate_image_compute_view = std::make_unique<vkb::core::ImageView>(*shading_rate_image_compute,
		                                                                         VK_IMAGE_VIEW_TYPE_2D,
		                                                                         VK_FORMAT_R8_UINT);

		// Create an attachment to store the frequency content of the rendered image during the render pass
		VkExtent3D frequency_image_extent{};
		frequency_image_extent.width  = this->width;
		frequency_image_extent.height = this->height;
		frequency_image_extent.depth  = 1;
		frequency_content_image       = std::make_unique<vkb::core::Image>(*device,
                                                                     frequency_image_extent,
                                                                     VK_FORMAT_R8G8_UINT,
                                                                     VK_IMAGE_USAGE_STORAGE_BIT |
                                                                         VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                                                         VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                                                                     VMA_MEMORY_USAGE_GPU_ONLY);
		frequency_content_image_view  = std::make_unique<vkb::core::ImageView>(*frequency_content_image,
                                                                              VK_IMAGE_VIEW_TYPE_2D,
                                                                              VK_FORMAT_R8G8_UINT);

		{
			auto &_cmd = device->request_command_buffer();
			_cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			auto _memory_barrier            = vkb::ImageMemoryBarrier();
			_memory_barrier.dst_access_mask = 0;
			_memory_barrier.src_access_mask = 0;
			_memory_barrier.old_layout      = VK_IMAGE_LAYOUT_UNDEFINED;
			_memory_barrier.new_layout      = VK_IMAGE_LAYOUT_GENERAL;
			_cmd.image_memory_barrier(*shading_rate_image_compute_view, _memory_barrier);
			_cmd.image_memory_barrier(*frequency_content_image_view, _memory_barrier);
			_cmd.end();

			auto &queue  = device->get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);
			auto  _fence = device->request_fence();
			queue.submit(_cmd, _fence);
			VK_CHECK(vkWaitForFences(device->get_handle(), 1, &_fence, VK_TRUE, UINT64_MAX));
		}
	}
}

void FragmentShadingRateDynamic::invalidate_shading_rate_attachment()
{
	device->wait_idle();

	// invalidate compute pipeline
	vkDestroyPipeline(device->get_handle(), compute.pipeline, VK_NULL_HANDLE);
	vkDestroyPipelineLayout(device->get_handle(), compute.pipeline_layout, VK_NULL_HANDLE);
	vkDestroyDescriptorSetLayout(device->get_handle(), compute.descriptor_set_layout, VK_NULL_HANDLE);
}

void FragmentShadingRateDynamic::setup_render_pass()
{
	std::array<VkRenderPass *, 2> render_passes{&render_pass, &fragment_render_pass};

	for (auto use_fragment_shading_rate : {false, true})
	{
		// Query the fragment shading rate properties of the current implementation, we will need them later on
		physical_device_fragment_shading_rate_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR;
		VkPhysicalDeviceProperties2KHR device_properties{};
		device_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		device_properties.pNext = &physical_device_fragment_shading_rate_properties;
		vkGetPhysicalDeviceProperties2KHR(get_device().get_gpu().get_handle(), &device_properties);

		// In contrast to the static fragment shading rate example, include
		// an attachment for the output of the frequency content of the rendered image
		std::vector<VkAttachmentDescription2KHR> attachments(3);
		// Color attachment
		attachments[0].sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
		attachments[0].format         = render_context->get_format();
		attachments[0].samples        = VK_SAMPLE_COUNT_1_BIT;
		attachments[0].loadOp         = use_fragment_shading_rate ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout  = use_fragment_shading_rate ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		attachments[0].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		// Depth attachment
		attachments[1].sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
		attachments[1].format         = depth_format;
		attachments[1].samples        = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp         = use_fragment_shading_rate ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		attachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].stencilLoadOp  = use_fragment_shading_rate ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].initialLayout  = use_fragment_shading_rate ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachments[1].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		// Fragment shading rate attachment
		attachments[2].sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
		attachments[2].format         = VK_FORMAT_R8_UINT;
		attachments[2].samples        = VK_SAMPLE_COUNT_1_BIT;
		attachments[2].loadOp         = VK_ATTACHMENT_LOAD_OP_LOAD;
		attachments[2].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[2].stencilLoadOp  = use_fragment_shading_rate ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[2].initialLayout  = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
		attachments[2].finalLayout    = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
		// Frequency content attachment
		if (use_fragment_shading_rate)
		{
			attachments.emplace_back(VkAttachmentDescription2KHR{});
			attachments[3].sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
			attachments[3].format         = VK_FORMAT_R8G8_UINT;
			attachments[3].samples        = VK_SAMPLE_COUNT_1_BIT;
			attachments[3].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[3].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[3].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[3].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
			attachments[3].finalLayout    = VK_IMAGE_LAYOUT_GENERAL;        // will be used by compute shader
		}

		VkAttachmentReference2KHR depth_reference = {VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2};
		depth_reference.attachment                = 1;
		depth_reference.layout                    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depth_reference.aspectMask                = VK_IMAGE_ASPECT_DEPTH_BIT;

		// Set up the attachment reference for the shading rate image attachment in slot 2
		VkAttachmentReference2 fragment_shading_rate_reference = {VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2};
		fragment_shading_rate_reference.attachment             = 2;
		fragment_shading_rate_reference.layout                 = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;

		// Set up the attachment info for the shading rate image, which will be added to the sub pass via structure chaining (in pNext)
		VkFragmentShadingRateAttachmentInfoKHR fragment_shading_rate_attachment_info = {
		    VK_STRUCTURE_TYPE_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR};
		fragment_shading_rate_attachment_info.pFragmentShadingRateAttachment        = &fragment_shading_rate_reference;
		fragment_shading_rate_attachment_info.shadingRateAttachmentTexelSize.width  = physical_device_fragment_shading_rate_properties.maxFragmentShadingRateAttachmentTexelSize.width;
		fragment_shading_rate_attachment_info.shadingRateAttachmentTexelSize.height = physical_device_fragment_shading_rate_properties.maxFragmentShadingRateAttachmentTexelSize.height;

		VkAttachmentReference2KHR color_reference = {VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2};
		color_reference.attachment                = 0;
		color_reference.layout                    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		color_reference.aspectMask                = VK_IMAGE_ASPECT_COLOR_BIT;

		// Setup attachment for frequency information
		VkAttachmentReference2 frequency_reference = {VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2};
		frequency_reference.attachment             = 3;
		frequency_reference.layout                 = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		frequency_reference.aspectMask             = VK_IMAGE_ASPECT_COLOR_BIT;

		std::vector<VkAttachmentReference2> color_references = {color_reference};
		if (use_fragment_shading_rate)
		{
			color_references.emplace_back(frequency_reference);
		}

		VkSubpassDescription2KHR sub_pass = {VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2};
		if (use_fragment_shading_rate)
		{
			// This sub pass will draw the 3D scene and generate the fragment shading rate
			// The color attachments includes both the (RGB) color output and the fragment shading rate image
			sub_pass.sType                   = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
			sub_pass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
			sub_pass.colorAttachmentCount    = static_cast<uint32_t>(color_references.size());
			sub_pass.pColorAttachments       = color_references.data();
			sub_pass.pDepthStencilAttachment = &depth_reference;
			sub_pass.inputAttachmentCount    = 0;
			sub_pass.pInputAttachments       = nullptr;
			sub_pass.preserveAttachmentCount = 0;
			sub_pass.pPreserveAttachments    = nullptr;
			sub_pass.pResolveAttachments     = nullptr;
			sub_pass.pNext                   = &fragment_shading_rate_attachment_info;
		}
		else
		{
			sub_pass.sType                   = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
			sub_pass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
			sub_pass.colorAttachmentCount    = 1;
			sub_pass.pColorAttachments       = &color_reference;
			sub_pass.pDepthStencilAttachment = &depth_reference;
			sub_pass.inputAttachmentCount    = 0;
			sub_pass.pInputAttachments       = nullptr;
			sub_pass.preserveAttachmentCount = 0;
			sub_pass.pPreserveAttachments    = nullptr;
			sub_pass.pResolveAttachments     = nullptr;
			sub_pass.pNext                   = nullptr;
		}

		// Sub-pass dependencies for layout transitions
		std::vector<VkSubpassDependency2KHR> dependencies = {};

		if (use_fragment_shading_rate)
		{
			dependencies.resize(2, {VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2});
			dependencies[0].sType           = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
			dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass      = 0;
			dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[0].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			dependencies[1].sType           = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
			dependencies[1].srcSubpass      = 0;
			dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
			dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
		}
		VkRenderPassCreateInfo2KHR render_pass_create_info = {};
		render_pass_create_info.sType                      = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
		render_pass_create_info.attachmentCount            = static_cast<uint32_t>(attachments.size());
		render_pass_create_info.pAttachments               = attachments.data();
		render_pass_create_info.subpassCount               = 1;
		render_pass_create_info.pSubpasses                 = &sub_pass;
		render_pass_create_info.dependencyCount            = static_cast<uint32_t>(dependencies.size());
		render_pass_create_info.pDependencies              = !dependencies.empty() ? dependencies.data() : VK_NULL_HANDLE;

		VK_CHECK(vkCreateRenderPass2KHR(device->get_handle(), &render_pass_create_info, nullptr,
		                                render_passes[static_cast<size_t>(use_fragment_shading_rate)]));
	}
}

void FragmentShadingRateDynamic::setup_framebuffer()
{
	// Create ths shading rate image attachment if not defined (first run and resize)
	auto check_dimension = [this](const vkb::core::ImageView *view) {
		if (!view)
		{
			return false;
		}
		auto extent = view->get_image().get_extent();
		return extent.width == this->width && extent.height == this->height;
	};
	if (compute_buffers.empty() || !check_dimension(compute_buffers[0].frequency_content_image_view.get()))
	{
		create_shading_rate_attachment();
	}

	assert(render_pass && fragment_render_pass);
	std::array<VkRenderPass *, 2>                                     render_passes{&render_pass, &fragment_render_pass};
	std::array<std::reference_wrapper<std::vector<VkFramebuffer>>, 2> framebuffer_refs{framebuffers,
	                                                                                   fragment_framebuffers};

	for (auto use_fragment_shading_rate : {false, true})
	{
		// Delete existing frame buffers
		std::vector<VkFramebuffer> &_framebuffers = framebuffer_refs[use_fragment_shading_rate];
		if (!_framebuffers.empty())
		{
			for (auto &framebuffer : _framebuffers)
			{
				if (framebuffer != VK_NULL_HANDLE)
				{
					vkDestroyFramebuffer(device->get_handle(), framebuffer, nullptr);
				}
			}
		}

		// Create frame buffers for every swap chain image
		_framebuffers.resize(render_context->get_render_frames().size());
		for (uint32_t i = 0; i < _framebuffers.size(); i++)
		{
			std::vector<VkImageView> attachments(3, {});
			// Depth/Stencil attachment is the same for all frame buffers
			attachments[1] = depth_stencil.view;
			// Fragment shading rate attachment
			attachments[2] = compute_buffers[i].shading_rate_image_view->get_handle();
			if (use_fragment_shading_rate)
			{
				attachments.emplace_back(compute_buffers[i].frequency_content_image_view->get_handle());
			}

			VkFramebufferCreateInfo framebuffer_create_info = {};
			framebuffer_create_info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebuffer_create_info.pNext                   = nullptr;
			framebuffer_create_info.renderPass              = *render_passes[use_fragment_shading_rate];
			framebuffer_create_info.attachmentCount         = static_cast<uint32_t>(attachments.size());
			framebuffer_create_info.pAttachments            = attachments.data();
			framebuffer_create_info.width                   = get_render_context().get_surface_extent().width;
			framebuffer_create_info.height                  = get_render_context().get_surface_extent().height;
			framebuffer_create_info.layers                  = 1;

			attachments[0] = swapchain_buffers[i].view;
			VK_CHECK(vkCreateFramebuffer(device->get_handle(), &framebuffer_create_info, nullptr, &_framebuffers[i]));
		}
	}
}

void FragmentShadingRateDynamic::build_command_buffers()
{
	setup_descriptor_sets();
	if (small_command_buffers.size() < draw_cmd_buffers.size())
	{
		const size_t old_size = small_command_buffers.size();
		small_command_buffers.resize(draw_cmd_buffers.size(), VK_NULL_HANDLE);
		auto allocate = vkb::initializers::command_buffer_allocate_info(command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		                                                                draw_cmd_buffers.size() - old_size);
		VK_CHECK(vkAllocateCommandBuffers(get_device().get_handle(), &allocate, &small_command_buffers[old_size]));
	}

	struct RenderTarget
	{
		VkCommandBuffer _command_buffer;
		VkFramebuffer   _fragment_framebuffer;
		VkFramebuffer   _framebuffer;
		VkDescriptorSet _descriptor_set;
		VkRenderPass    _render_pass;
		VkExtent2D      image_extent;
		bool            enable_ui;
		bool            enable_fragment_shading_rate;
	};

	auto build_command_buffer = [&](RenderTarget render_target) {
		VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();
		VK_CHECK(vkBeginCommandBuffer(render_target._command_buffer, &command_buffer_begin_info));

		std::array<VkClearValue, 4> clear_values{};
		clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
		clear_values[1].depthStencil = {0.0f, 0};
		clear_values[2].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
		clear_values[3].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};

		// Final composition
		VkRenderPassBeginInfo render_pass_begin_info = vkb::initializers::render_pass_begin_info();
		render_pass_begin_info.framebuffer           = render_target._fragment_framebuffer;
		render_pass_begin_info.renderPass            = render_target._render_pass;
		render_pass_begin_info.clearValueCount       = static_cast<uint32_t>(clear_values.size());
		render_pass_begin_info.pClearValues          = clear_values.data();
		render_pass_begin_info.renderArea.extent     = render_target.image_extent;

		vkCmdBeginRenderPass(render_target._command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		const uint32_t _width = render_target.image_extent.width, _height = render_target.image_extent.height;
		VkViewport     viewport = vkb::initializers::viewport(static_cast<float>(_width), static_cast<float>(_height), 0.0f, 1.0f);
		vkCmdSetViewport(render_target._command_buffer, 0, 1, &viewport);

		VkRect2D scissor = vkb::initializers::rect2D(static_cast<int32_t>(_width), static_cast<int32_t>(_height), 0, 0);
		vkCmdSetScissor(render_target._command_buffer, 0, 1, &scissor);

		VkDescriptorSet descriptor_set = render_target._descriptor_set;
		vkCmdBindDescriptorSets(render_target._command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
		                        &descriptor_set, 0, nullptr);

		// Set the fragment shading rate state for the current pipeline
		VkExtent2D                         fragment_size = {1, 1};
		VkFragmentShadingRateCombinerOpKHR combiner_ops[2];
		// The combiners determine how the different shading rate values for the pipeline, primitives and attachment are combined
		if (enable_attachment_shading_rate && render_target.enable_fragment_shading_rate)
		{
			// If shading rate from attachment is enabled, we set the combiner, so that the values from the attachment are used
			// Combiner for pipeline (A) and primitive (B) - Not used in this sample
			combiner_ops[0] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR;
			// Combiner for pipeline (A) and attachment (B), replace the pipeline default value (fragment_size) with the fragment sizes stored in the attachment
			combiner_ops[1] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_KHR;
		}
		else
		{
			// If shading rate from attachment is disabled, we keep the value set via the dynamic state
			combiner_ops[0] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR;
			combiner_ops[1] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR;
		}
		vkCmdSetFragmentShadingRateKHR(render_target._command_buffer, &fragment_size, combiner_ops);

		if (display_sky_sphere)
		{
			vkCmdBindPipeline(render_target._command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.skysphere);
			push_const_block.object_type = 0;
			vkCmdPushConstants(render_target._command_buffer, pipeline_layout,
			                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push_const_block),
			                   &push_const_block);
			vkCmdBindDescriptorSets(render_target._command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0,
			                        1, &descriptor_set, 0, nullptr);
			draw_model(models.skysphere, render_target._command_buffer);
		}

		vkCmdBindPipeline(render_target._command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.sphere);
		vkCmdBindDescriptorSets(render_target._command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
		                        &descriptor_set, 0, nullptr);
		std::vector<glm::vec3> mesh_offsets = {
		    glm::vec3(-2.5f, 0.0f, 0.0f),
		    glm::vec3(0.0f, 0.0f, 0.0f),
		    glm::vec3(2.5f, 0.0f, 0.0f),
		};
		for (uint32_t j = 0; j < 3; j++)
		{
			push_const_block.object_type = 1;
			push_const_block.offset      = glm::vec4(mesh_offsets[j], 0.0f);
			vkCmdPushConstants(render_target._command_buffer, pipeline_layout,
			                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push_const_block),
			                   &push_const_block);
			draw_model(models.scene, render_target._command_buffer);
		}

		vkCmdEndRenderPass(render_target._command_buffer);

		if (render_target.enable_ui)
		{
			render_pass_begin_info.clearValueCount = 0;
			render_pass_begin_info.pClearValues    = VK_NULL_HANDLE;
			render_pass_begin_info.renderPass      = render_pass;
			render_pass_begin_info.framebuffer     = render_target._framebuffer;
			vkCmdBeginRenderPass(render_target._command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

			draw_ui(render_target._command_buffer);

			vkCmdEndRenderPass(render_target._command_buffer);
		}

		VK_CHECK(vkEndCommandBuffer(render_target._command_buffer));
	};

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		assert(subpass_extent.width > 0 && subpass_extent.width <= width && subpass_extent.height > 0 &&
		       subpass_extent.height <= height);
		RenderTarget small_target{
		    small_command_buffers[i], fragment_framebuffers[i], framebuffers[i], render_descriptor_sets[i],
		    fragment_render_pass, subpass_extent, false, false};
		RenderTarget full_target{
		    draw_cmd_buffers[i], fragment_framebuffers[i], framebuffers[i], render_descriptor_sets[i], fragment_render_pass, {width, height}, true, true};
		build_command_buffer(small_target);
		build_command_buffer(full_target);
	}
}

void FragmentShadingRateDynamic::load_assets()
{
	models.skysphere   = load_model("scenes/geosphere.gltf");
	textures.skysphere = load_texture("textures/skysphere_rgba.ktx", vkb::sg::Image::Color);
	models.scene       = load_model("scenes/textured_unit_cube.gltf");
	textures.scene     = load_texture("textures/vulkan_logo_full.ktx", vkb::sg::Image::Color);
}

void FragmentShadingRateDynamic::setup_descriptor_pool()
{
	const auto N = static_cast<uint32_t>(draw_cmd_buffers.size());

	if (descriptor_pool)
	{
		vkDestroyDescriptorPool(get_device().get_handle(), descriptor_pool, VK_NULL_HANDLE);
		descriptor_pool = VK_NULL_HANDLE;
	}

	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4 * N),
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6 * N),
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 4 * N)};
	uint32_t                   num_descriptor_sets = std::max(static_cast<uint32_t>(4), N);
	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(static_cast<uint32_t>(pool_sizes.size()), pool_sizes.data(),
	                                                   num_descriptor_sets);
	VK_CHECK(
	    vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void FragmentShadingRateDynamic::setup_descriptor_set_layout()
{
	// Scene rendering descriptors
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	                                                     VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
	                                                     0),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	                                                     VK_SHADER_STAGE_FRAGMENT_BIT, 1),        // sampler env map
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	                                                     VK_SHADER_STAGE_FRAGMENT_BIT, 2),        // sampler sphere
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
	                                                     VK_SHADER_STAGE_FRAGMENT_BIT,
	                                                     3),        // input_frequency
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
	                                                     VK_SHADER_STAGE_FRAGMENT_BIT,
	                                                     4)        // output_sampling_rate
	};

	VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info =
	    vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(),
	                                                         static_cast<uint32_t>(set_layout_bindings.size()));

	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr,
	                                     &descriptor_set_layout));

	VkPipelineLayoutCreateInfo pipeline_layout_create_info =
	    vkb::initializers::pipeline_layout_create_info(&descriptor_set_layout, 1);

	// Pass object offset and color via push constant
	VkPushConstantRange push_constant_range = vkb::initializers::push_constant_range(
	    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(push_const_block), 0);
	pipeline_layout_create_info.pushConstantRangeCount = 1;
	pipeline_layout_create_info.pPushConstantRanges    = &push_constant_range;

	VK_CHECK(
	    vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout));
}

void FragmentShadingRateDynamic::setup_descriptor_sets()
{
	// Shared model object descriptor set
	VkDescriptorSetAllocateInfo alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &descriptor_set_layout, 1);

	render_descriptor_sets.resize(draw_cmd_buffers.size(), VK_NULL_HANDLE);
	assert(!compute_buffers.empty());

	for (size_t i = 0; i < render_descriptor_sets.size(); ++i)
	{
		const size_t prev_frame     = (i + compute_buffers.size() - 1) % compute_buffers.size();
		auto        &descriptor_set = render_descriptor_sets[i];
		if (!descriptor_set)
		{
			VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_set));
		}

		VkDescriptorBufferInfo scene_buffer_descriptor      = create_descriptor(*uniform_buffers.scene);
		VkDescriptorImageInfo  environment_image_descriptor = create_descriptor(textures.skysphere);
		VkDescriptorImageInfo  sphere_image_descriptor      = create_descriptor(textures.scene);

		// We want to visualize the previous frame's frequency and shading rate image
		VkDescriptorImageInfo frequency_descriptor = vkb::initializers::descriptor_image_info(VK_NULL_HANDLE,
		                                                                                      compute_buffers[prev_frame].frequency_content_image_view->get_handle(),
		                                                                                      VK_IMAGE_LAYOUT_GENERAL);
		VkDescriptorImageInfo shading_image        = vkb::initializers::descriptor_image_info(VK_NULL_HANDLE,
		                                                                                      compute_buffers[prev_frame].shading_rate_image_compute_view->get_handle(),
		                                                                                      VK_IMAGE_LAYOUT_GENERAL);

		std::vector<VkWriteDescriptorSet>
		    write_descriptor_sets = {
		        vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
		                                                &scene_buffer_descriptor),
		        vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
		                                                &environment_image_descriptor),
		        vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2,
		                                                &sphere_image_descriptor),
		        vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3,
		                                                &frequency_descriptor),
		        vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 4,
		                                                &shading_image),
		    };
		vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()),
		                       write_descriptor_sets.data(), 0, nullptr);
	}
}

void FragmentShadingRateDynamic::create_compute_pipeline()
{
	// Descriptor set layout
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
	                                                     VK_SHADER_STAGE_COMPUTE_BIT, 0),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
	                                                     VK_SHADER_STAGE_COMPUTE_BIT, 1),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	                                                     VK_SHADER_STAGE_COMPUTE_BIT, 2),
	};

	VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info = vkb::initializers::descriptor_set_layout_create_info(
	    set_layout_bindings);

	VK_CHECK(vkCreateDescriptorSetLayout(device->get_handle(), &descriptor_layout_create_info, VK_NULL_HANDLE,
	                                     &compute.descriptor_set_layout));

	VkPipelineLayoutCreateInfo pipeline_layout_create_info = vkb::initializers::pipeline_layout_create_info(
	    &compute.descriptor_set_layout, 1);
	VK_CHECK(vkCreatePipelineLayout(device->get_handle(), &pipeline_layout_create_info, VK_NULL_HANDLE,
	                                &compute.pipeline_layout));

	// Descriptor pool
	if (compute.descriptor_pool)
	{
		vkDestroyDescriptorPool(device->get_handle(), compute.descriptor_pool, VK_NULL_HANDLE);
		compute.descriptor_pool = VK_NULL_HANDLE;
	}

	const auto                        N     = static_cast<uint32_t>(draw_cmd_buffers.size());
	std::vector<VkDescriptorPoolSize> sizes = {
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 4 * N),
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4 * N)};
	const auto pool_create = vkb::initializers::descriptor_pool_create_info(sizes, N);
	VK_CHECK(vkCreateDescriptorPool(device->get_handle(), &pool_create, VK_NULL_HANDLE, &compute.descriptor_pool));

	// Descriptor sets
	for (auto &&compute_buffer : compute_buffers)
	{
		VkDescriptorSetAllocateInfo alloc_info = vkb::initializers::descriptor_set_allocate_info(
		    compute.descriptor_pool, &compute.descriptor_set_layout, 1);
		VK_CHECK(vkAllocateDescriptorSets(device->get_handle(), &alloc_info, &compute_buffer.descriptor_set));
	}

	// Pipeline
	VkComputePipelineCreateInfo pipeline_create_info = vkb::initializers::compute_pipeline_create_info(
	    compute.pipeline_layout);
	pipeline_create_info.stage = load_shader("fragment_shading_rate_dynamic/generate_shading_rate.comp",
	                                         VK_SHADER_STAGE_COMPUTE_BIT);
	VK_CHECK(vkCreateComputePipelines(device->get_handle(), pipeline_cache, 1, &pipeline_create_info, VK_NULL_HANDLE,
	                                  &compute.pipeline));

	for (auto &&compute_buffer : compute_buffers)
	{
		auto create = vkb::initializers::command_buffer_allocate_info(command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
		VK_CHECK(vkAllocateCommandBuffers(get_device().get_handle(), &create, &compute_buffer.command_buffer));
	}
	update_compute_pipeline();
}

void FragmentShadingRateDynamic::update_compute_pipeline()
{
	// Update array
	assert(!fragment_shading_rates.empty());

	uint32_t                max_rate_x = 0, max_rate_y = 0;
	std::vector<glm::uvec2> shading_rates_u_vec_2;
	shading_rates_u_vec_2.reserve(fragment_shading_rates.size());
	for (auto &&rate : fragment_shading_rates)
	{
		max_rate_x = std::max(max_rate_x, rate.fragmentSize.width);
		max_rate_y = std::max(max_rate_y, rate.fragmentSize.height);
		shading_rates_u_vec_2.emplace_back(glm::uvec2(rate.fragmentSize.width, rate.fragmentSize.height));
	}

	assert(max_rate_x && max_rate_y);
	FrequencyInformation params{
	    glm::uvec2(subpass_extent.width, subpass_extent.height),
	    glm::uvec2(compute_buffers[0].shading_rate_image->get_extent().width,
	               compute_buffers[0].shading_rate_image->get_extent().height),
	    glm::uvec2(max_rate_x, max_rate_y),
	    static_cast<uint32_t>(fragment_shading_rates.size()),
	    static_cast<uint32_t>(0)};

	// Transfer frequency information to buffer
	const uint32_t buffer_size =
	    sizeof(FrequencyInformation) + shading_rates_u_vec_2.size() * sizeof(shading_rates_u_vec_2[0]);
	frequency_information_params = std::make_unique<vkb::core::Buffer>(*device, buffer_size,
	                                                                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	                                                                   VMA_MEMORY_USAGE_CPU_TO_GPU);
	frequency_information_params->update(&params, sizeof(FrequencyInformation), 0);
	frequency_information_params->update(shading_rates_u_vec_2.data(),
	                                     shading_rates_u_vec_2.size() * sizeof(shading_rates_u_vec_2[0]),
	                                     sizeof(FrequencyInformation));

	// Update descriptor sets
	for (auto &compute_buffer : compute_buffers)
	{
		auto &shading_rate_image              = compute_buffer.shading_rate_image;
		auto &shading_rate_image_view         = compute_buffer.shading_rate_image_view;
		auto &frequency_content_image         = compute_buffer.frequency_content_image;
		auto &frequency_content_image_view    = compute_buffer.frequency_content_image_view;
		auto &shading_rate_image_compute      = compute_buffer.shading_rate_image_compute;
		auto &shading_rate_image_compute_view = compute_buffer.shading_rate_image_compute_view;

		VkDescriptorImageInfo               frequency_image       = vkb::initializers::descriptor_image_info(VK_NULL_HANDLE,
		                                                                                                     frequency_content_image_view->get_handle(),
		                                                                                                     VK_IMAGE_LAYOUT_GENERAL);
		VkDescriptorImageInfo               shading_image         = vkb::initializers::descriptor_image_info(VK_NULL_HANDLE,
		                                                                                                     shading_rate_image_compute_view->get_handle(),
		                                                                                                     VK_IMAGE_LAYOUT_GENERAL);
		VkDescriptorBufferInfo              buffer_info           = create_descriptor(*frequency_information_params);
		std::array<VkWriteDescriptorSet, 3> write_descriptor_sets = {
		    vkb::initializers::write_descriptor_set(compute_buffer.descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		                                            0, &frequency_image),
		    vkb::initializers::write_descriptor_set(compute_buffer.descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		                                            1, &shading_image),
		    vkb::initializers::write_descriptor_set(compute_buffer.descriptor_set,
		                                            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2, &buffer_info)};
		vkUpdateDescriptorSets(device->get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()),
		                       write_descriptor_sets.data(), 0, VK_NULL_HANDLE);

		// Command Buffer
		assert(compute_buffer.command_buffer);
		auto &command_buffer = compute_buffer.command_buffer;

		auto begin = vkb::initializers::command_buffer_begin_info();
		VK_CHECK(vkBeginCommandBuffer(command_buffer, &begin));

		const auto     fragment_extent = compute_buffer.shading_rate_image->get_extent();
		const uint32_t fragment_width = std::max(static_cast<uint32_t>(1), fragment_extent.width), fragment_height = std::max(
		                                                                                               static_cast<uint32_t>(1), fragment_extent.height);

		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.pipeline);
		vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.pipeline_layout, 0, 1,
		                        &compute_buffer.descriptor_set, 0, nullptr);
		vkCmdDispatch(command_buffer, 1 + (fragment_width - 1) / 8, 1 + (fragment_height - 1) / 8, 1);

		VkImageCopy image_copy;
		image_copy.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
		image_copy.dstOffset      = {0, 0, 0};
		image_copy.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
		image_copy.extent         = shading_rate_image->get_extent();
		image_copy.srcOffset      = {0, 0, 0};

		vkb::image_layout_transition(
		    command_buffer, shading_rate_image->get_handle(), VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		VkImageSubresourceRange subresource_range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
		vkb::image_layout_transition(command_buffer,
		                             shading_rate_image_compute->get_handle(),
		                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		                             VK_PIPELINE_STAGE_TRANSFER_BIT,
		                             VK_ACCESS_SHADER_WRITE_BIT,
		                             VK_ACCESS_TRANSFER_READ_BIT,
		                             VK_IMAGE_LAYOUT_GENERAL,
		                             VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		                             subresource_range);

		vkCmdCopyImage(command_buffer, shading_rate_image_compute->get_handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		               shading_rate_image->get_handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_copy);

		vkb::image_layout_transition(
		    command_buffer, shading_rate_image->get_handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR);

		vkb::image_layout_transition(command_buffer,
		                             shading_rate_image_compute->get_handle(),
		                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		                             {},
		                             {},
		                             VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		                             VK_IMAGE_LAYOUT_GENERAL,
		                             subresource_range);

		VK_CHECK(vkEndCommandBuffer(compute_buffer.command_buffer));

		if (debug_utils_supported)
		{
			auto set_name = [device{get_device().get_handle()}](VkObjectType object_type, const char *name,
			                                                    const void *handle) {
				VkDebugUtilsObjectNameInfoEXT name_info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
				name_info.objectType                    = object_type;
				name_info.objectHandle                  = (uint64_t) handle;
				name_info.pObjectName                   = name;
				vkSetDebugUtilsObjectNameEXT(device, &name_info);
			};
			set_name(VK_OBJECT_TYPE_IMAGE_VIEW, "shading_rate_image_compute_view",
			         reinterpret_cast<void *>(shading_rate_image_compute_view->get_handle()));
			set_name(VK_OBJECT_TYPE_IMAGE_VIEW, "shading_rate_image_view", reinterpret_cast<void *>(shading_rate_image_view->get_handle()));
			set_name(VK_OBJECT_TYPE_IMAGE_VIEW, "frequency_content_image_view",
			         reinterpret_cast<void *>(frequency_content_image_view->get_handle()));
			set_name(VK_OBJECT_TYPE_IMAGE, "shading_rate_image_compute", reinterpret_cast<void *>(shading_rate_image_compute->get_handle()));
			set_name(VK_OBJECT_TYPE_IMAGE, "shading_rate_image", reinterpret_cast<void *>(shading_rate_image->get_handle()));
			set_name(VK_OBJECT_TYPE_IMAGE, "frequency_content_image", reinterpret_cast<void *>(frequency_content_image->get_handle()));
		}
	}
}

void FragmentShadingRateDynamic::prepare_pipelines()
{
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state =
	    vkb::initializers::pipeline_input_assembly_state_create_info(
	        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	        0,
	        VK_FALSE);

	VkPipelineRasterizationStateCreateInfo rasterization_state =
	    vkb::initializers::pipeline_rasterization_state_create_info(
	        VK_POLYGON_MODE_FILL,
	        VK_CULL_MODE_BACK_BIT,
	        VK_FRONT_FACE_COUNTER_CLOCKWISE,
	        0);

	std::vector<VkPipelineColorBlendAttachmentState> blend_attachment_state(2,
	                                                                        vkb::initializers::pipeline_color_blend_attachment_state(
	                                                                            0xf,
	                                                                            VK_FALSE));

	VkPipelineColorBlendStateCreateInfo color_blend_state =
	    vkb::initializers::pipeline_color_blend_state_create_info(
	        blend_attachment_state.size(),
	        blend_attachment_state.data());

	// Note: Using reversed depth-buffer for increased precision, so Greater depth values are kept
	VkPipelineDepthStencilStateCreateInfo depth_stencil_state =
	    vkb::initializers::pipeline_depth_stencil_state_create_info(
	        VK_FALSE,
	        VK_FALSE,
	        VK_COMPARE_OP_GREATER);

	VkPipelineViewportStateCreateInfo viewport_state =
	    vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);

	VkPipelineMultisampleStateCreateInfo multisample_state =
	    vkb::initializers::pipeline_multisample_state_create_info(
	        VK_SAMPLE_COUNT_1_BIT,
	        0);

	std::vector<VkDynamicState> dynamic_state_enables = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR,
	    // Add fragment shading rate dynamic state, so we can easily toggle this at runtime
	    VK_DYNAMIC_STATE_FRAGMENT_SHADING_RATE_KHR};
	VkPipelineDynamicStateCreateInfo dynamic_state =
	    vkb::initializers::pipeline_dynamic_state_create_info(
	        dynamic_state_enables.data(),
	        static_cast<uint32_t>(dynamic_state_enables.size()),
	        0);

	VkGraphicsPipelineCreateInfo pipeline_create_info =
	    vkb::initializers::pipeline_create_info(
	        pipeline_layout,
	        fragment_render_pass,
	        0);

	std::vector<VkPipelineColorBlendAttachmentState> blend_attachment_states = {
	    vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE),
	};

	pipeline_create_info.pInputAssemblyState = &input_assembly_state;
	pipeline_create_info.pRasterizationState = &rasterization_state;
	pipeline_create_info.pColorBlendState    = &color_blend_state;
	pipeline_create_info.pMultisampleState   = &multisample_state;
	pipeline_create_info.pViewportState      = &viewport_state;
	pipeline_create_info.pDepthStencilState  = &depth_stencil_state;
	pipeline_create_info.pDynamicState       = &dynamic_state;

	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};
	pipeline_create_info.stageCount = static_cast<uint32_t>(shader_stages.size());
	pipeline_create_info.pStages    = shader_stages.data();

	// Vertex bindings an attributes for model rendering
	std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
	};
	std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT,
	                                                          0),        // Position
	    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32B32_SFLOAT,
	                                                          sizeof(float) * 3),        // Normal
	    vkb::initializers::vertex_input_attribute_description(0, 2, VK_FORMAT_R32G32_SFLOAT,
	                                                          sizeof(float) * 6),        // UV
	};

	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	pipeline_create_info.pVertexInputState = &vertex_input_state;
	pipeline_create_info.layout            = pipeline_layout;
	pipeline_create_info.subpass           = 0;

	// Sky-sphere
	shader_stages[0] = load_shader("fragment_shading_rate_dynamic/scene.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("fragment_shading_rate_dynamic/scene.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr,
	                                   &pipelines.skysphere));

	// Objects
	// Enable depth test and write
	depth_stencil_state.depthWriteEnable = VK_TRUE;
	depth_stencil_state.depthTestEnable  = VK_TRUE;
	// Flip cull mode
	rasterization_state.cullMode = VK_CULL_MODE_FRONT_BIT;
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr,
	                                   &pipelines.sphere));
}

void FragmentShadingRateDynamic::prepare_uniform_buffers()
{
	uniform_buffers.scene = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                            sizeof(ubo_scene),
	                                                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                            VMA_MEMORY_USAGE_CPU_TO_GPU);
	update_uniform_buffers();
}

void FragmentShadingRateDynamic::update_uniform_buffers()
{
	ubo_scene.projection          = camera.matrices.perspective;
	ubo_scene.modelview           = camera.matrices.view * glm::mat4(1.0f);
	ubo_scene.skysphere_modelview = camera.matrices.view;
	uniform_buffers.scene->convert_and_update(ubo_scene);
}

void FragmentShadingRateDynamic::draw()
{
	VkSemaphore        semaphore{VK_NULL_HANDLE};
	const VkSemaphore *old_semaphore{VK_NULL_HANDLE};
	const VkSemaphore *wait_semaphore{VK_NULL_HANDLE};
	auto               semaphore_create = vkb::initializers::semaphore_create_info();
	vkCreateSemaphore(device->get_handle(), &semaphore_create, VK_NULL_HANDLE, &semaphore);

	ApiVulkanSample::prepare_frame();
	const auto start_submit = submit_info;
	assert(submit_info.signalSemaphoreCount == 1);
	old_semaphore  = submit_info.pSignalSemaphores;
	wait_semaphore = submit_info.pWaitSemaphores;
	std::vector<VkSemaphore> semaphores{submit_info.pSignalSemaphores[0], semaphore};

	submit_info.commandBufferCount   = 1;
	submit_info.pCommandBuffers      = &draw_cmd_buffers[current_buffer];
	submit_info.signalSemaphoreCount = 2;
	submit_info.pSignalSemaphores    = semaphores.data();

	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();

	const std::array<VkPipelineStageFlags, 2> small_wait_mask = {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	                                                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
	VkSemaphore                               small_semaphore{VK_NULL_HANDLE};
	vkCreateSemaphore(device->get_handle(), &semaphore_create, VK_NULL_HANDLE, &small_semaphore);
	submit_info.pCommandBuffers      = &small_command_buffers[current_buffer];
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores    = &small_semaphore;
	submit_info.waitSemaphoreCount   = 1;
	submit_info.pWaitDstStageMask    = small_wait_mask.data();
	submit_info.pWaitSemaphores      = &semaphore;
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

	const VkPipelineStageFlags wait_mask           = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	auto                       compute_submit_info = vkb::initializers::submit_info();
	compute_submit_info.commandBufferCount         = 1;
	compute_submit_info.pCommandBuffers            = &compute_buffers[current_buffer].command_buffer;
	compute_submit_info.pWaitDstStageMask          = &wait_mask;
	compute_submit_info.pWaitSemaphores            = &small_semaphore;
	compute_submit_info.waitSemaphoreCount         = 1;
	compute_submit_info.signalSemaphoreCount       = 0;
	compute_submit_info.pSignalSemaphores          = VK_NULL_HANDLE;

	if (!compute_fence)
	{
		auto fence_create = vkb::initializers::fence_create_info();
		VK_CHECK(vkCreateFence(device->get_handle(), &fence_create, VK_NULL_HANDLE, &compute_fence));
	}

	VK_CHECK(vkQueueSubmit(queue, 1, &compute_submit_info, compute_fence));
	VK_CHECK(vkWaitForFences(device->get_handle(), 1, &compute_fence, VK_TRUE, UINT64_MAX));
	VK_CHECK(vkResetFences(device->get_handle(), 1, &compute_fence));

	vkDestroySemaphore(device->get_handle(), semaphore, VK_NULL_HANDLE);
	vkDestroySemaphore(device->get_handle(), small_semaphore, VK_NULL_HANDLE);
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores    = old_semaphore;
	submit_info.waitSemaphoreCount   = 1;
	submit_info.pWaitSemaphores      = wait_semaphore;
	submit_info.pWaitDstStageMask    = start_submit.pWaitDstStageMask;
}

bool FragmentShadingRateDynamic::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	const auto enabled_instance_extensions = instance->get_extensions();
	debug_utils_supported =
	    std::find_if(enabled_instance_extensions.cbegin(), enabled_instance_extensions.cend(), [](const char *ext) {
		    return strcmp(ext, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0;
	    }) != enabled_instance_extensions.cend();

	camera.type = vkb::CameraType::FirstPerson;
	camera.set_position(glm::vec3(0.0f, 0.0f, -4.0f));

	// Note: Using Revered depth-buffer for increased precision, so Znear and Zfar are flipped
	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 256.0f, 0.1f);

	// VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
	auto command_pool_create  = vkb::initializers::command_pool_create_info();
	command_pool_create.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	command_pool_create.pNext = VK_NULL_HANDLE;
	vkCreateCommandPool(get_device().get_handle(), &command_pool_create, VK_NULL_HANDLE, &command_pool);

	load_assets();
	prepare_uniform_buffers();
	setup_descriptor_set_layout();
	prepare_pipelines();
	setup_descriptor_pool();
	create_compute_pipeline();
	setup_descriptor_sets();
	build_command_buffers();
	prepared = true;
	return true;
}

void FragmentShadingRateDynamic::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	draw();
	if (camera.updated)
	{
		update_uniform_buffers();
	}
}

void FragmentShadingRateDynamic::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Settings"))
	{
		if (drawer.checkbox("Enable attachment shading rate", &enable_attachment_shading_rate))
		{
			build_command_buffers();
		}

		static const std::vector<std::string> frequency_decimation_rates = {"1", "2", "4", "8", "16"};

		int32_t selection = std::min(static_cast<int32_t>(frequency_decimation_rates.size()) - 1,
		                             static_cast<int32_t>(log2f(static_cast<float>(subpass_extent_ratio))));
		if (drawer.combo_box("Subpass size reduction", &selection, frequency_decimation_rates))
		{
			subpass_extent_ratio = (1 << selection);
			resize(width, height);
		}

		static const std::vector<std::string> shading_rate_names = {"Render output", "Shading Rates",
		                                                            "Frequency channel"};
		if (drawer.combo_box("Data visualize", &ubo_scene.color_shading_rate, shading_rate_names))
		{
			update_uniform_buffers();
		}

		if (drawer.checkbox("sky-sphere", &display_sky_sphere))
		{
			build_command_buffers();
		}
	}
}

bool FragmentShadingRateDynamic::resize(const uint32_t new_width, const uint32_t new_height)
{
	invalidate_shading_rate_attachment();
	if (!ApiVulkanSample::resize(new_width, new_height))
	{
		setup_framebuffer();
	}
	create_shading_rate_attachment();
	create_compute_pipeline();
	setup_framebuffer();
	setup_descriptor_sets();
	build_command_buffers();
	update_uniform_buffers();
	return true;
}

std::unique_ptr<vkb::VulkanSample> create_fragment_shading_rate_dynamic()
{
	return std::make_unique<FragmentShadingRateDynamic>();
}
