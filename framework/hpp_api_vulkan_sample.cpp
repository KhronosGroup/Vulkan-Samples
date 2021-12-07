/* Copyright (c) 2021, NVIDIA CORPORATION. All rights reserved.
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

#include <hpp_api_vulkan_sample.h>

#include <common/hpp_vk_common.h>
#include <core/hpp_buffer.h>
#include <hpp_gltf_loader.h>

// Instantiate the default dispatcher
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

bool HPPApiVulkanSample::prepare(vkb::platform::HPPPlatform &platform)
{
	if (!HPPVulkanSample::prepare(platform))
	{
		return false;
	}

	// initialize function pointers for C++-bindings
	static vk::DynamicLoader dl;
	VULKAN_HPP_DEFAULT_DISPATCHER.init(dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));
	VULKAN_HPP_DEFAULT_DISPATCHER.init(get_instance().get_handle());
	VULKAN_HPP_DEFAULT_DISPATCHER.init(get_device().get_handle());

	depth_format = vkb::common::get_suitable_depth_format(get_device().get_gpu().get_handle());

	// Create synchronization objects
	// Create a semaphore used to synchronize image presentation
	// Ensures that the current swapchain render target has completed presentation and has been released by the presentation engine, ready for rendering
	semaphores.acquired_image_ready = get_device().get_handle().createSemaphore({});
	// Create a semaphore used to synchronize command submission
	// Ensures that the image is not presented until all commands have been sumbitted and executed
	semaphores.render_complete = get_device().get_handle().createSemaphore({});

	// Set up submit info structure
	// Semaphores will stay the same during application lifetime
	// Command buffer submission info is set by each example
	submit_info                   = vk::SubmitInfo();
	submit_info.pWaitDstStageMask = &submit_pipeline_stages;

	if (platform.get_window().get_window_mode() != vkb::Window::Mode::Headless)
	{
		submit_info.setWaitSemaphores(semaphores.acquired_image_ready);
		submit_info.setSignalSemaphores(semaphores.render_complete);
	}

	queue = get_device().get_suitable_graphics_queue().get_handle();

	create_swapchain_buffers();
	create_command_pool();
	create_command_buffers();
	create_synchronization_primitives();
	setup_depth_stencil();
	setup_render_pass();
	create_pipeline_cache();
	setup_framebuffer();

	extent = get_render_context().get_surface_extent();

	gui = std::make_unique<vkb::Gui>(*this, platform.get_window(), /*stats=*/nullptr, 15.0f, true);
	gui->prepare(pipeline_cache,
	             render_pass,
	             {load_shader("uioverlay/uioverlay.vert", vk::ShaderStageFlagBits::eVertex),
	              load_shader("uioverlay/uioverlay.frag", vk::ShaderStageFlagBits::eFragment)});

	return true;
}

void HPPApiVulkanSample::update(float delta_time)
{
	if (view_updated)
	{
		view_updated = false;
		view_changed();
	}

	update_overlay(delta_time);

	render(delta_time);
	camera.update(delta_time);
	if (camera.moving())
	{
		view_updated = true;
	}

	get_platform().on_post_draw(get_render_context());
}

bool HPPApiVulkanSample::resize(const uint32_t, const uint32_t)
{
	if (!prepared)
	{
		return false;
	}

	get_render_context().handle_surface_changes();

	// Don't recreate the swapchain if the dimensions haven't changed
	if (extent == get_render_context().get_surface_extent())
	{
		return false;
	}

	extent = get_render_context().get_surface_extent();

	prepared = false;

	// Ensure all operations on the device have been finished before destroying resources
	get_device().wait_idle();

	create_swapchain_buffers();

	// Recreate the frame buffers
	get_device().get_handle().destroyImageView(depth_stencil.view);
	get_device().get_handle().destroyImage(depth_stencil.image);
	get_device().get_handle().freeMemory(depth_stencil.mem);
	setup_depth_stencil();
	for (uint32_t i = 0; i < framebuffers.size(); i++)
	{
		get_device().get_handle().destroyFramebuffer(framebuffers[i]);
		framebuffers[i] = nullptr;
	}
	setup_framebuffer();

	if (extent.width && extent.height && gui)
	{
		gui->resize(extent.width, extent.height);
	}

	// Command buffers need to be recreated as they may store
	// references to the recreated frame buffer
	destroy_command_buffers();
	create_command_buffers();
	build_command_buffers();

	get_device().wait_idle();

	if (extent.width && extent.height)
	{
		camera.update_aspect_ratio((float) extent.width / (float) extent.height);
	}

	// Notify derived class
	view_changed();

	prepared = true;
	return true;
}

void HPPApiVulkanSample::create_render_context(vkb::Platform &platform)
{
	HPPVulkanSample::create_render_context(platform,
	                                       {{vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear},
	                                        {vk::Format::eR8G8B8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear},
	                                        {vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear},
	                                        {vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear}});
}

void HPPApiVulkanSample::prepare_render_context()
{
	HPPVulkanSample::prepare_render_context();
}

void HPPApiVulkanSample::input_event(const vkb::InputEvent &input_event)
{
	HPPVulkanSample::input_event(input_event);

	bool gui_captures_event = false;

	if (gui)
	{
		gui_captures_event = gui->input_event(input_event);
	}

	if (!gui_captures_event)
	{
		if (input_event.get_source() == vkb::EventSource::Mouse)
		{
			const auto &mouse_button = static_cast<const vkb::MouseButtonInputEvent &>(input_event);

			handle_mouse_move(static_cast<int32_t>(mouse_button.get_pos_x()), static_cast<int32_t>(mouse_button.get_pos_y()));

			if (mouse_button.get_action() == vkb::MouseAction::Down)
			{
				switch (mouse_button.get_button())
				{
					case vkb::MouseButton::Left:
						mouse_buttons.left = true;
						break;
					case vkb::MouseButton::Right:
						mouse_buttons.right = true;
						break;
					case vkb::MouseButton::Middle:
						mouse_buttons.middle = true;
						break;
					default:
						break;
				}
			}
			else if (mouse_button.get_action() == vkb::MouseAction::Up)
			{
				switch (mouse_button.get_button())
				{
					case vkb::MouseButton::Left:
						mouse_buttons.left = false;
						break;
					case vkb::MouseButton::Right:
						mouse_buttons.right = false;
						break;
					case vkb::MouseButton::Middle:
						mouse_buttons.middle = false;
						break;
					default:
						break;
				}
			}
		}
		else if (input_event.get_source() == vkb::EventSource::Touchscreen)
		{
			const auto &touch_event = static_cast<const vkb::TouchInputEvent &>(input_event);

			if (touch_event.get_action() == vkb::TouchAction::Down)
			{
				touch_down         = true;
				touch_pos.x        = static_cast<int32_t>(touch_event.get_pos_x());
				touch_pos.y        = static_cast<int32_t>(touch_event.get_pos_y());
				mouse_pos.x        = touch_event.get_pos_x();
				mouse_pos.y        = touch_event.get_pos_y();
				mouse_buttons.left = true;
			}
			else if (touch_event.get_action() == vkb::TouchAction::Up)
			{
				touch_pos.x        = static_cast<int32_t>(touch_event.get_pos_x());
				touch_pos.y        = static_cast<int32_t>(touch_event.get_pos_y());
				touch_timer        = 0.0;
				touch_down         = false;
				camera.keys.up     = false;
				mouse_buttons.left = false;
			}
			else if (touch_event.get_action() == vkb::TouchAction::Move)
			{
				bool handled = false;
				if (gui)
				{
					ImGuiIO &io = ImGui::GetIO();
					handled     = io.WantCaptureMouse;
				}
				if (!handled)
				{
					int32_t eventX = static_cast<int32_t>(touch_event.get_pos_x());
					int32_t eventY = static_cast<int32_t>(touch_event.get_pos_y());

					float deltaX = (float) (touch_pos.y - eventY) * rotation_speed * 0.5f;
					float deltaY = (float) (touch_pos.x - eventX) * rotation_speed * 0.5f;

					camera.rotate(glm::vec3(deltaX, 0.0f, 0.0f));
					camera.rotate(glm::vec3(0.0f, -deltaY, 0.0f));

					rotation.x += deltaX;
					rotation.y -= deltaY;

					view_changed();

					touch_pos.x = eventX;
					touch_pos.y = eventY;
				}
			}
		}
		else if (input_event.get_source() == vkb::EventSource::Keyboard)
		{
			const auto &key_button = static_cast<const vkb::KeyInputEvent &>(input_event);

			if (key_button.get_action() == vkb::KeyAction::Down)
			{
				switch (key_button.get_code())
				{
					case vkb::KeyCode::W:
						camera.keys.up = true;
						break;
					case vkb::KeyCode::S:
						camera.keys.down = true;
						break;
					case vkb::KeyCode::A:
						camera.keys.left = true;
						break;
					case vkb::KeyCode::D:
						camera.keys.right = true;
						break;
					case vkb::KeyCode::P:
						paused = !paused;
						break;
					default:
						break;
				}
			}
			else if (key_button.get_action() == vkb::KeyAction::Up)
			{
				switch (key_button.get_code())
				{
					case vkb::KeyCode::W:
						camera.keys.up = false;
						break;
					case vkb::KeyCode::S:
						camera.keys.down = false;
						break;
					case vkb::KeyCode::A:
						camera.keys.left = false;
						break;
					case vkb::KeyCode::D:
						camera.keys.right = false;
						break;
					default:
						break;
				}
			}
		}
	}
}

void HPPApiVulkanSample::handle_mouse_move(int32_t x, int32_t y)
{
	int32_t dx = (int32_t) mouse_pos.x - x;
	int32_t dy = (int32_t) mouse_pos.y - y;

	bool handled = false;

	if (gui)
	{
		ImGuiIO &io = ImGui::GetIO();
		handled     = io.WantCaptureMouse;
	}
	mouse_moved((float) x, (float) y, handled);

	if (handled)
	{
		mouse_pos = glm::vec2((float) x, (float) y);
		return;
	}

	if (mouse_buttons.left)
	{
		rotation.x += dy * 1.25f * rotation_speed;
		rotation.y -= dx * 1.25f * rotation_speed;
		camera.rotate(glm::vec3(dy * camera.rotation_speed, -dx * camera.rotation_speed, 0.0f));
		view_updated = true;
	}
	if (mouse_buttons.right)
	{
		zoom += dy * .005f * zoom_speed;
		camera.translate(glm::vec3(-0.0f, 0.0f, dy * .005f * zoom_speed));
		view_updated = true;
	}
	if (mouse_buttons.middle)
	{
		camera_pos.x -= dx * 0.01f;
		camera_pos.y -= dy * 0.01f;
		camera.translate(glm::vec3(-dx * 0.01f, -dy * 0.01f, 0.0f));
		view_updated = true;
	}
	mouse_pos = glm::vec2((float) x, (float) y);
}

void HPPApiVulkanSample::mouse_moved(double x, double y, bool &handled)
{}

bool HPPApiVulkanSample::check_command_buffers()
{
	for (auto &command_buffer : draw_cmd_buffers)
	{
		if (command_buffer)
		{
			return false;
		}
	}
	return true;
}

void HPPApiVulkanSample::create_command_buffers()
{
	// Create one command buffer for each swap chain image and reuse for rendering
	vk::CommandBufferAllocateInfo allocate_info(
	    cmd_pool, vk::CommandBufferLevel::ePrimary, static_cast<uint32_t>(get_render_context().get_render_frames().size()));

	draw_cmd_buffers = get_device().get_handle().allocateCommandBuffers(allocate_info);
}

void HPPApiVulkanSample::destroy_command_buffers()
{
	get_device().get_handle().freeCommandBuffers(cmd_pool, draw_cmd_buffers);
}

void HPPApiVulkanSample::create_pipeline_cache()
{
	pipeline_cache = get_device().get_handle().createPipelineCache({});
}

vk::PipelineShaderStageCreateInfo HPPApiVulkanSample::load_shader(const std::string &file, vk::ShaderStageFlagBits stage)
{
	shader_modules.push_back(vkb::common::load_shader(file.c_str(), get_device().get_handle(), stage));
	assert(shader_modules.back());
	return vk::PipelineShaderStageCreateInfo({}, stage, shader_modules.back(), "main");
}

void HPPApiVulkanSample::update_overlay(float delta_time)
{
	if (gui)
	{
		gui->show_simple_window(get_name(), vkb::to_u32(1.0f / delta_time), [this]() { on_update_ui_overlay(gui->get_drawer()); });

		gui->update(delta_time);

		if (gui->update_buffers() || gui->get_drawer().is_dirty())
		{
			build_command_buffers();
			gui->get_drawer().clear();
		}
	}
}

void HPPApiVulkanSample::draw_ui(const vk::CommandBuffer command_buffer)
{
	if (gui)
	{
		command_buffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f));
		command_buffer.setScissor(0, vk::Rect2D({0, 0}, extent));

		gui->draw(command_buffer);
	}
}

void HPPApiVulkanSample::prepare_frame()
{
	if (get_render_context().has_swapchain())
	{
		handle_surface_changes();
		// Acquire the next image from the swap chain
		// Shows how to filter an error code from a vulkan function, which is mapped to an exception but should be handled here!
		vk::Result result;
		try
		{
			result = get_render_context().get_swapchain().acquire_next_image(current_buffer, semaphores.acquired_image_ready);
		}
		catch (const vk::SystemError &e)
		{
			if (e.code() == vk::Result::eErrorOutOfDateKHR)
			{
				result = vk::Result::eErrorOutOfDateKHR;
			}
			else
			{
				throw;
			}
		}
		// Recreate the swapchain if it's no longer compatible with the surface (eErrorOutOfDateKHR) or no longer optimal for
		// presentation (eSuboptimalKHR)
		if ((result == vk::Result::eErrorOutOfDateKHR) || (result == vk::Result::eSuboptimalKHR))
		{
			resize(extent.width, extent.height);
		}
	}
}

void HPPApiVulkanSample::submit_frame()
{
	if (get_render_context().has_swapchain())
	{
		const auto &queue = get_device().get_queue_by_present(0);

		vk::SwapchainKHR swapchain = get_render_context().get_swapchain().get_handle();

		vk::PresentInfoKHR present_info({}, swapchain, current_buffer);
		// Check if a wait semaphore has been specified to wait for before presenting the image
		if (semaphores.render_complete)
		{
			present_info.setWaitSemaphores(semaphores.render_complete);
		}

		// Shows how to filter an error code from a vulkan function, which is mapped to an exception but should be handled here!
		vk::Result present_result;
		try
		{
			present_result = queue.present(present_info);
		}
		catch (const vk::SystemError &e)
		{
			if (e.code() == vk::Result::eErrorOutOfDateKHR)
			{
				// Swap chain is no longer compatible with the surface and needs to be recreated
				resize(extent.width, extent.height);
				return;
			}
			else
			{
				// rethrow this error
				throw;
			}
		}
	}

	// DO NOT USE
	// vkDeviceWaitIdle and vkQueueWaitIdle are extremely expensive functions, and are used here purely for demonstrating the vulkan API
	// without having to concern ourselves with proper syncronization. These functions should NEVER be used inside the render loop like this (every frame).
	get_device().get_queue_by_present(0).wait_idle();
}

HPPApiVulkanSample::~HPPApiVulkanSample()
{
	if (get_device().get_handle())
	{
		get_device().wait_idle();

		// Clean up Vulkan resources
		get_device().get_handle().destroyDescriptorPool(descriptor_pool);
		destroy_command_buffers();
		get_device().get_handle().destroyRenderPass(render_pass);
		for (auto &framebuffer : framebuffers)
		{
			get_device().get_handle().destroyFramebuffer(framebuffer);
		}

		for (auto &swapchain_buffer : swapchain_buffers)
		{
			get_device().get_handle().destroyImageView(swapchain_buffer.view);
		}

		for (auto &shader_module : shader_modules)
		{
			get_device().get_handle().destroyShaderModule(shader_module);
		}
		get_device().get_handle().destroyImageView(depth_stencil.view);
		get_device().get_handle().destroyImage(depth_stencil.image);
		get_device().get_handle().freeMemory(depth_stencil.mem);

		get_device().get_handle().destroyPipelineCache(pipeline_cache);

		get_device().get_handle().destroyCommandPool(cmd_pool);

		get_device().get_handle().destroySemaphore(semaphores.acquired_image_ready);
		get_device().get_handle().destroySemaphore(semaphores.render_complete);
		for (auto &fence : wait_fences)
		{
			get_device().get_handle().destroyFence(fence);
		}
	}

	gui.reset();
}

void HPPApiVulkanSample::view_changed()
{}

void HPPApiVulkanSample::build_command_buffers()
{}

void HPPApiVulkanSample::create_synchronization_primitives()
{
	// Wait fences to sync command buffer access
	vk::FenceCreateInfo fence_create_info(vk::FenceCreateFlagBits::eSignaled);
	wait_fences.resize(draw_cmd_buffers.size());
	for (auto &fence : wait_fences)
	{
		fence = get_device().get_handle().createFence(fence_create_info);
	}
}

void HPPApiVulkanSample::create_command_pool()
{
	uint32_t                  queue_family_index = get_device().get_queue_by_flags(vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute, 0).get_family_index();
	vk::CommandPoolCreateInfo command_pool_info(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queue_family_index);
	cmd_pool = get_device().get_handle().createCommandPool(command_pool_info);
}

void HPPApiVulkanSample::setup_depth_stencil()
{
	vk::ImageCreateInfo image_create_info;
	image_create_info.imageType   = vk::ImageType::e2D;
	image_create_info.format      = depth_format;
	image_create_info.extent      = vk::Extent3D(get_render_context().get_surface_extent().width, get_render_context().get_surface_extent().height, 1);
	image_create_info.mipLevels   = 1;
	image_create_info.arrayLayers = 1;
	image_create_info.samples     = vk::SampleCountFlagBits::e1;
	image_create_info.tiling      = vk::ImageTiling::eOptimal;
	image_create_info.usage       = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferSrc;

	depth_stencil.image            = get_device().get_handle().createImage(image_create_info);
	vk::MemoryRequirements memReqs = get_device().get_handle().getImageMemoryRequirements(depth_stencil.image);

	vk::MemoryAllocateInfo memory_allocation(memReqs.size, get_device().get_memory_type(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));
	depth_stencil.mem = get_device().get_handle().allocateMemory(memory_allocation);
	get_device().get_handle().bindImageMemory(depth_stencil.image, depth_stencil.mem, 0);

	vk::ImageViewCreateInfo image_view_create_info;
	image_view_create_info.viewType                        = vk::ImageViewType::e2D;
	image_view_create_info.image                           = depth_stencil.image;
	image_view_create_info.format                          = depth_format;
	image_view_create_info.subresourceRange.baseMipLevel   = 0;
	image_view_create_info.subresourceRange.levelCount     = 1;
	image_view_create_info.subresourceRange.baseArrayLayer = 0;
	image_view_create_info.subresourceRange.layerCount     = 1;
	image_view_create_info.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eDepth;
	// Stencil aspect should only be set on depth + stencil formats (vk::Format::eD16UnormS8Uint..vk::Format::eD32SfloatS8Uint
	if (vk::Format::eD16UnormS8Uint <= depth_format)
	{
		image_view_create_info.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
	}
	depth_stencil.view = get_device().get_handle().createImageView(image_view_create_info);
}

void HPPApiVulkanSample::setup_framebuffer()
{
	std::array<vk::ImageView, 2> attachments;

	// Depth/Stencil attachment is the same for all frame buffers
	attachments[1] = depth_stencil.view;

	vk::FramebufferCreateInfo framebuffer_create_info(
	    {}, render_pass, attachments, get_render_context().get_surface_extent().width, get_render_context().get_surface_extent().height, 1);

	// Delete existing frame buffers
	for (auto &framebuffer : framebuffers)
	{
		get_device().get_handle().destroyFramebuffer(framebuffer);
	}
	framebuffers.clear();

	// Create frame buffers for every swap chain image
	framebuffers.reserve(swapchain_buffers.size());
	for (auto &buffer : swapchain_buffers)
	{
		attachments[0] = buffer.view;
		framebuffers.push_back(get_device().get_handle().createFramebuffer(framebuffer_create_info));
	}
}

void HPPApiVulkanSample::setup_render_pass()
{
	std::array<vk::AttachmentDescription, 2> attachments;
	// Color attachment
	attachments[0].format         = get_render_context().get_format();
	attachments[0].samples        = vk::SampleCountFlagBits::e1;
	attachments[0].loadOp         = vk::AttachmentLoadOp::eClear;
	attachments[0].storeOp        = vk::AttachmentStoreOp::eStore;
	attachments[0].stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
	attachments[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	attachments[0].initialLayout  = vk::ImageLayout::eUndefined;
	attachments[0].finalLayout    = vk::ImageLayout::ePresentSrcKHR;
	// Depth attachment
	attachments[1].format         = depth_format;
	attachments[1].samples        = vk::SampleCountFlagBits::e1;
	attachments[1].loadOp         = vk::AttachmentLoadOp::eClear;
	attachments[1].storeOp        = vk::AttachmentStoreOp::eDontCare;
	attachments[1].stencilLoadOp  = vk::AttachmentLoadOp::eClear;
	attachments[1].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	attachments[1].initialLayout  = vk::ImageLayout::eUndefined;
	attachments[1].finalLayout    = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	vk::AttachmentReference color_reference(0, vk::ImageLayout::eColorAttachmentOptimal);

	vk::AttachmentReference depth_reference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

	vk::SubpassDescription subpass_description({}, vk::PipelineBindPoint::eGraphics, {}, color_reference, {}, &depth_reference);

	// Subpass dependencies for layout transitions
	std::array<vk::SubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass      = 0;
	dependencies[0].srcStageMask    = vk::PipelineStageFlagBits::eBottomOfPipe;
	dependencies[0].dstStageMask    = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependencies[0].srcAccessMask   = vk::AccessFlagBits::eMemoryRead;
	dependencies[0].dstAccessMask   = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
	dependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

	dependencies[1].srcSubpass      = 0;
	dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask    = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependencies[1].dstStageMask    = vk::PipelineStageFlagBits::eBottomOfPipe;
	dependencies[1].srcAccessMask   = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
	dependencies[1].dstAccessMask   = vk::AccessFlagBits::eMemoryRead;
	dependencies[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

	vk::RenderPassCreateInfo render_pass_create_info({}, attachments, subpass_description, dependencies);

	render_pass = get_device().get_handle().createRenderPass(render_pass_create_info);
}

void HPPApiVulkanSample::update_render_pass_flags(RenderPassCreateFlags flags)
{
	get_device().get_handle().destroyRenderPass(render_pass);

	vk::AttachmentLoadOp color_attachment_load_op      = vk::AttachmentLoadOp::eClear;
	vk::ImageLayout      color_attachment_image_layout = vk::ImageLayout::eUndefined;

	// Samples can keep the color attachment contents, e.g. if they have previously written to the swap chain images
	if (flags & RenderPassCreateFlags::ColorAttachmentLoad)
	{
		color_attachment_load_op      = vk::AttachmentLoadOp::eLoad;
		color_attachment_image_layout = vk::ImageLayout::ePresentSrcKHR;
	}

	std::array<vk::AttachmentDescription, 2> attachments = {};
	// Color attachment
	attachments[0].format         = get_render_context().get_format();
	attachments[0].samples        = vk::SampleCountFlagBits::e1;
	attachments[0].loadOp         = color_attachment_load_op;
	attachments[0].storeOp        = vk::AttachmentStoreOp::eStore;
	attachments[0].stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
	attachments[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	attachments[0].initialLayout  = color_attachment_image_layout;
	attachments[0].finalLayout    = vk::ImageLayout::ePresentSrcKHR;
	// Depth attachment
	attachments[1].format         = depth_format;
	attachments[1].samples        = vk::SampleCountFlagBits::e1;
	attachments[1].loadOp         = vk::AttachmentLoadOp::eClear;
	attachments[1].storeOp        = vk::AttachmentStoreOp::eDontCare;
	attachments[1].stencilLoadOp  = vk::AttachmentLoadOp::eClear;
	attachments[1].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	attachments[1].initialLayout  = vk::ImageLayout::eUndefined;
	attachments[1].finalLayout    = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	vk::AttachmentReference color_reference(0, vk::ImageLayout::eColorAttachmentOptimal);

	vk::AttachmentReference depth_reference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

	vk::SubpassDescription subpass_description({}, vk::PipelineBindPoint::eGraphics, {}, color_reference, {}, &depth_reference);

	// Subpass dependencies for layout transitions
	std::array<vk::SubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass      = 0;
	dependencies[0].srcStageMask    = vk::PipelineStageFlagBits::eBottomOfPipe;
	dependencies[0].dstStageMask    = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependencies[0].srcAccessMask   = vk::AccessFlagBits::eMemoryWrite;
	dependencies[0].dstAccessMask   = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
	dependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

	dependencies[1].srcSubpass      = 0;
	dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask    = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependencies[1].dstStageMask    = vk::PipelineStageFlagBits::eBottomOfPipe;
	dependencies[1].srcAccessMask   = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
	dependencies[1].dstAccessMask   = vk::AccessFlagBits::eMemoryRead;
	dependencies[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

	vk::RenderPassCreateInfo render_pass_create_info({}, attachments, subpass_description, dependencies);

	render_pass = get_device().get_handle().createRenderPass(render_pass_create_info);
}

void HPPApiVulkanSample::on_update_ui_overlay(vkb::Drawer &drawer)
{}

void HPPApiVulkanSample::create_swapchain_buffers()
{
	if (get_render_context().has_swapchain())
	{
		auto &images = get_render_context().get_swapchain().get_images();

		// Get the swap chain buffers containing the image and imageview
		for (auto &swapchain_buffer : swapchain_buffers)
		{
			get_device().get_handle().destroyImageView(swapchain_buffer.view);
		}
		swapchain_buffers.clear();
		swapchain_buffers.reserve(images.size());
		for (auto &image : images)
		{
			vk::ImageViewCreateInfo color_attachment_view;
			color_attachment_view.image                           = image;
			color_attachment_view.viewType                        = vk::ImageViewType::e2D;
			color_attachment_view.format                          = get_render_context().get_swapchain().get_format();
			color_attachment_view.components                      = {vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA};
			color_attachment_view.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;
			color_attachment_view.subresourceRange.baseMipLevel   = 0;
			color_attachment_view.subresourceRange.levelCount     = 1;
			color_attachment_view.subresourceRange.baseArrayLayer = 0;
			color_attachment_view.subresourceRange.layerCount     = 1;

			swapchain_buffers.push_back({image, get_device().get_handle().createImageView(color_attachment_view)});
		}
	}
	else
	{
		auto &frames = get_render_context().get_render_frames();

		// Get the swap chain buffers containing the image and imageview
		swapchain_buffers.clear();
		swapchain_buffers.reserve(frames.size());
		for (auto &frame : frames)
		{
			auto &image_view = *frame->get_render_target().get_views().begin();
			swapchain_buffers.push_back({image_view.get_image().get_handle(), image_view.get_handle()});
		}
	}
}

void HPPApiVulkanSample::update_swapchain_image_usage_flags(std::set<vk::ImageUsageFlagBits> const &image_usage_flags)
{
	get_render_context().update_swapchain(image_usage_flags);
	create_swapchain_buffers();
	setup_framebuffer();
}

void HPPApiVulkanSample::handle_surface_changes()
{
	vk::SurfaceCapabilitiesKHR surface_properties =
	    get_device().get_gpu().get_handle().getSurfaceCapabilitiesKHR(get_render_context().get_swapchain().get_surface());

	if (surface_properties.currentExtent != get_render_context().get_surface_extent())
	{
		resize(surface_properties.currentExtent.width, surface_properties.currentExtent.height);
	}
}

HPPTexture HPPApiVulkanSample::load_texture(const std::string &file)
{
	HPPTexture texture;

	texture.image = vkb::scene_graph::components::HPPImage::load(file, file);
	texture.image->create_vk_image(get_device());

	const auto &queue = get_device().get_queue_by_flags(vk::QueueFlagBits::eGraphics, 0);

	vk::CommandBuffer command_buffer = get_device().create_command_buffer(vk::CommandBufferLevel::ePrimary, true);

	vkb::core::HPPBuffer stage_buffer{get_device(), texture.image->get_data().size(), vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY};

	stage_buffer.update(texture.image->get_data());

	// Setup buffer copy regions for each mip level
	std::vector<vk::BufferImageCopy> bufferCopyRegions;

	auto &mipmaps = texture.image->get_mipmaps();

	for (size_t i = 0; i < mipmaps.size(); i++)
	{
		vk::BufferImageCopy buffer_copy_region;
		buffer_copy_region.imageSubresource.aspectMask     = vk::ImageAspectFlagBits::eColor;
		buffer_copy_region.imageSubresource.mipLevel       = vkb::to_u32(i);
		buffer_copy_region.imageSubresource.baseArrayLayer = 0;
		buffer_copy_region.imageSubresource.layerCount     = 1;
		buffer_copy_region.imageExtent.width               = texture.image->get_extent().width >> i;
		buffer_copy_region.imageExtent.height              = texture.image->get_extent().height >> i;
		buffer_copy_region.imageExtent.depth               = 1;
		buffer_copy_region.bufferOffset                    = mipmaps[i].offset;

		bufferCopyRegions.push_back(buffer_copy_region);
	}

	vk::ImageSubresourceRange subresource_range(vk::ImageAspectFlagBits::eColor, 0, vkb::to_u32(mipmaps.size()), 0, 1);

	// Image barrier for optimal image (target)
	// Optimal image will be used as destination for the copy
	vkb::common::set_image_layout(
	    command_buffer, texture.image->get_vk_image().get_handle(), vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, subresource_range);

	// Copy mip levels from staging buffer
	command_buffer.copyBufferToImage(
	    stage_buffer.get_handle(), texture.image->get_vk_image().get_handle(), vk::ImageLayout::eTransferDstOptimal, bufferCopyRegions);

	// Change texture image layout to shader read after all mip levels have been copied
	vkb::common::set_image_layout(command_buffer,
	                              texture.image->get_vk_image().get_handle(),
	                              vk::ImageLayout::eTransferDstOptimal,
	                              vk::ImageLayout::eShaderReadOnlyOptimal,
	                              subresource_range);

	get_device().flush_command_buffer(command_buffer, queue.get_handle());

	// Create a defaultsampler
	vk::SamplerCreateInfo sampler_create_info;
	sampler_create_info.magFilter    = vk::Filter::eLinear;
	sampler_create_info.minFilter    = vk::Filter::eLinear;
	sampler_create_info.mipmapMode   = vk::SamplerMipmapMode::eLinear;
	sampler_create_info.addressModeU = vk::SamplerAddressMode::eRepeat;
	sampler_create_info.addressModeV = vk::SamplerAddressMode::eRepeat;
	sampler_create_info.addressModeW = vk::SamplerAddressMode::eRepeat;
	sampler_create_info.mipLodBias   = 0.0f;
	sampler_create_info.compareOp    = vk::CompareOp::eNever;
	sampler_create_info.minLod       = 0.0f;
	// Max level-of-detail should match mip level count
	sampler_create_info.maxLod = static_cast<float>(mipmaps.size());
	// Only enable anisotropic filtering if enabled on the device
	// Note that for simplicity, we will always be using max. available anisotropy level for the current device
	// This may have an impact on performance, esp. on lower-specced devices
	// In a real-world scenario the level of anisotropy should be a user setting or e.g. lowered for mobile devices by default
	sampler_create_info.maxAnisotropy =
	    get_device().get_gpu().get_features().samplerAnisotropy ? (get_device().get_gpu().get_properties().limits.maxSamplerAnisotropy) : 1.0f;
	sampler_create_info.anisotropyEnable = get_device().get_gpu().get_features().samplerAnisotropy;
	sampler_create_info.borderColor      = vk::BorderColor::eFloatOpaqueWhite;
	texture.sampler                      = get_device().get_handle().createSampler(sampler_create_info);

	return texture;
}

HPPTexture HPPApiVulkanSample::load_texture_array(const std::string &file)
{
	HPPTexture texture{};

	texture.image = vkb::scene_graph::components::HPPImage::load(file, file);
	texture.image->create_vk_image(get_device(), vk::ImageViewType::e2DArray);

	const auto &queue = get_device().get_queue_by_flags(vk::QueueFlagBits::eGraphics, 0);

	vk::CommandBuffer command_buffer = get_device().create_command_buffer(vk::CommandBufferLevel::ePrimary, true);

	vkb::core::HPPBuffer stage_buffer{get_device(), texture.image->get_data().size(), vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY};

	stage_buffer.update(texture.image->get_data());

	// Setup buffer copy regions for each mip level
	std::vector<vk::BufferImageCopy> buffer_copy_regions;

	auto &      mipmaps = texture.image->get_mipmaps();
	const auto &layers  = texture.image->get_layers();

	auto &offsets = texture.image->get_offsets();

	for (uint32_t layer = 0; layer < layers; layer++)
	{
		for (size_t i = 0; i < mipmaps.size(); i++)
		{
			vk::BufferImageCopy buffer_copy_region;
			buffer_copy_region.imageSubresource.aspectMask     = vk::ImageAspectFlagBits::eColor;
			buffer_copy_region.imageSubresource.mipLevel       = vkb::to_u32(i);
			buffer_copy_region.imageSubresource.baseArrayLayer = layer;
			buffer_copy_region.imageSubresource.layerCount     = 1;
			buffer_copy_region.imageExtent.width               = texture.image->get_extent().width >> i;
			buffer_copy_region.imageExtent.height              = texture.image->get_extent().height >> i;
			buffer_copy_region.imageExtent.depth               = 1;
			buffer_copy_region.bufferOffset                    = offsets[layer][i];

			buffer_copy_regions.push_back(buffer_copy_region);
		}
	}

	vk::ImageSubresourceRange subresource_range(vk::ImageAspectFlagBits::eColor, 0, vkb::to_u32(mipmaps.size()), 0, layers);

	// Image barrier for optimal image (target)
	// Optimal image will be used as destination for the copy
	vkb::common::set_image_layout(
	    command_buffer, texture.image->get_vk_image().get_handle(), vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, subresource_range);

	// Copy mip levels from staging buffer
	command_buffer.copyBufferToImage(
	    stage_buffer.get_handle(), texture.image->get_vk_image().get_handle(), vk::ImageLayout::eTransferDstOptimal, buffer_copy_regions);

	// Change texture image layout to shader read after all mip levels have been copied
	vkb::common::set_image_layout(command_buffer,
	                              texture.image->get_vk_image().get_handle(),
	                              vk::ImageLayout::eTransferDstOptimal,
	                              vk::ImageLayout::eShaderReadOnlyOptimal,
	                              subresource_range);

	get_device().flush_command_buffer(command_buffer, queue.get_handle());

	// Create a defaultsampler
	vk::SamplerCreateInfo sampler_create_info;
	sampler_create_info.magFilter    = vk::Filter::eLinear;
	sampler_create_info.minFilter    = vk::Filter::eLinear;
	sampler_create_info.mipmapMode   = vk::SamplerMipmapMode::eLinear;
	sampler_create_info.addressModeU = vk::SamplerAddressMode::eClampToEdge;
	sampler_create_info.addressModeV = vk::SamplerAddressMode::eClampToEdge;
	sampler_create_info.addressModeW = vk::SamplerAddressMode::eClampToEdge;
	sampler_create_info.mipLodBias   = 0.0f;
	sampler_create_info.compareOp    = vk::CompareOp::eNever;
	sampler_create_info.minLod       = 0.0f;
	// Max level-of-detail should match mip level count
	sampler_create_info.maxLod = static_cast<float>(mipmaps.size());
	// Only enable anisotropic filtering if enabled on the devicec
	sampler_create_info.maxAnisotropy =
	    get_device().get_gpu().get_features().samplerAnisotropy ? get_device().get_gpu().get_properties().limits.maxSamplerAnisotropy : 1.0f;
	sampler_create_info.anisotropyEnable = get_device().get_gpu().get_features().samplerAnisotropy;
	sampler_create_info.borderColor      = vk::BorderColor::eFloatOpaqueWhite;
	texture.sampler                      = get_device().get_handle().createSampler(sampler_create_info);

	return texture;
}

HPPTexture HPPApiVulkanSample::load_texture_cubemap(const std::string &file)
{
	HPPTexture texture{};

	texture.image = vkb::scene_graph::components::HPPImage::load(file, file);
	texture.image->create_vk_image(get_device(), vk::ImageViewType::eCube, vk::ImageCreateFlagBits::eCubeCompatible);

	const auto &queue = get_device().get_queue_by_flags(vk::QueueFlagBits::eGraphics, 0);

	vk::CommandBuffer command_buffer = get_device().create_command_buffer(vk::CommandBufferLevel::ePrimary, true);

	vkb::core::HPPBuffer stage_buffer{get_device(), texture.image->get_data().size(), vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY};

	stage_buffer.update(texture.image->get_data());

	// Setup buffer copy regions for each mip level
	std::vector<vk::BufferImageCopy> buffer_copy_regions;

	auto &      mipmaps = texture.image->get_mipmaps();
	const auto &layers  = texture.image->get_layers();

	auto &offsets = texture.image->get_offsets();

	for (uint32_t layer = 0; layer < layers; layer++)
	{
		for (size_t i = 0; i < mipmaps.size(); i++)
		{
			vk::BufferImageCopy buffer_copy_region;
			buffer_copy_region.imageSubresource.aspectMask     = vk::ImageAspectFlagBits::eColor;
			buffer_copy_region.imageSubresource.mipLevel       = vkb::to_u32(i);
			buffer_copy_region.imageSubresource.baseArrayLayer = layer;
			buffer_copy_region.imageSubresource.layerCount     = 1;
			buffer_copy_region.imageExtent.width               = texture.image->get_extent().width >> i;
			buffer_copy_region.imageExtent.height              = texture.image->get_extent().height >> i;
			buffer_copy_region.imageExtent.depth               = 1;
			buffer_copy_region.bufferOffset                    = offsets[layer][i];

			buffer_copy_regions.push_back(buffer_copy_region);
		}
	}

	vk::ImageSubresourceRange subresource_range(vk::ImageAspectFlagBits::eColor, 0, vkb::to_u32(mipmaps.size()), 0, layers);

	// Image barrier for optimal image (target)
	// Optimal image will be used as destination for the copy
	vkb::common::set_image_layout(
	    command_buffer, texture.image->get_vk_image().get_handle(), vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, subresource_range);

	// Copy mip levels from staging buffer
	command_buffer.copyBufferToImage(
	    stage_buffer.get_handle(), texture.image->get_vk_image().get_handle(), vk::ImageLayout::eTransferDstOptimal, buffer_copy_regions);

	// Change texture image layout to shader read after all mip levels have been copied
	vkb::common::set_image_layout(command_buffer,
	                              texture.image->get_vk_image().get_handle(),
	                              vk::ImageLayout::eTransferDstOptimal,
	                              vk::ImageLayout::eShaderReadOnlyOptimal,
	                              subresource_range);

	get_device().flush_command_buffer(command_buffer, queue.get_handle());

	// Create a defaultsampler
	vk::SamplerCreateInfo sampler_create_info;
	sampler_create_info.magFilter    = vk::Filter::eLinear;
	sampler_create_info.minFilter    = vk::Filter::eLinear;
	sampler_create_info.mipmapMode   = vk::SamplerMipmapMode::eLinear;
	sampler_create_info.addressModeU = vk::SamplerAddressMode::eClampToEdge;
	sampler_create_info.addressModeV = vk::SamplerAddressMode::eClampToEdge;
	sampler_create_info.addressModeW = vk::SamplerAddressMode::eClampToEdge;
	sampler_create_info.mipLodBias   = 0.0f;
	sampler_create_info.compareOp    = vk::CompareOp::eNever;
	sampler_create_info.minLod       = 0.0f;
	// Max level-of-detail should match mip level count
	sampler_create_info.maxLod = static_cast<float>(mipmaps.size());
	// Only enable anisotropic filtering if enabled on the devicec
	sampler_create_info.maxAnisotropy =
	    get_device().get_gpu().get_features().samplerAnisotropy ? get_device().get_gpu().get_properties().limits.maxSamplerAnisotropy : 1.0f;
	sampler_create_info.anisotropyEnable = get_device().get_gpu().get_features().samplerAnisotropy;
	sampler_create_info.borderColor      = vk::BorderColor::eFloatOpaqueWhite;
	texture.sampler                      = get_device().get_handle().createSampler(sampler_create_info);

	return texture;
}

std::unique_ptr<vkb::scene_graph::components::HPPSubMesh> HPPApiVulkanSample::load_model(const std::string &file, uint32_t index)
{
	vkb::HPPGLTFLoader loader{get_device()};

	std::unique_ptr<vkb::scene_graph::components::HPPSubMesh> model = loader.read_model_from_file(file, index);

	if (!model)
	{
		LOGE("Cannot load model from file: {}", file.c_str());
		throw std::runtime_error("Cannot load model from: " + file);
	}

	return model;
}

void HPPApiVulkanSample::draw_model(std::unique_ptr<vkb::scene_graph::components::HPPSubMesh> &model, vk::CommandBuffer command_buffer)
{
	vk::DeviceSize offset = 0;

	const auto &vertex_buffer = model->get_vertex_buffer("vertex_buffer");
	auto &      index_buffer  = model->get_index_buffer();

	command_buffer.bindVertexBuffers(0, vertex_buffer.get_handle(), offset);
	command_buffer.bindIndexBuffer(index_buffer.get_handle(), 0, model->get_index_type());
	command_buffer.drawIndexed(model->vertex_indices, 1, 0, 0, 0);
}

void HPPApiVulkanSample::with_command_buffer(const std::function<void(vk::CommandBuffer command_buffer)> &f, vk::Semaphore signalSemaphore)
{
	vk::CommandBuffer command_buffer = get_device().create_command_buffer(vk::CommandBufferLevel::ePrimary, true);
	f(command_buffer);
	get_device().flush_command_buffer(command_buffer, queue, true, signalSemaphore);
}
