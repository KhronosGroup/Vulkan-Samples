/* Copyright (c) 2021-2024, NVIDIA CORPORATION. All rights reserved.
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

bool HPPApiVulkanSample::prepare(const vkb::ApplicationOptions &options)
{
	if (!HPPVulkanSample::prepare(options))
	{
		return false;
	}

	depth_format = vkb::common::get_suitable_depth_format(get_device()->get_gpu().get_handle());

	// Create synchronization objects
	// Create a semaphore used to synchronize image presentation
	// Ensures that the current swapchain render target has completed presentation and has been released by the presentation engine, ready for rendering
	semaphores.acquired_image_ready = get_device()->get_handle().createSemaphore({});
	// Create a semaphore used to synchronize command submission
	// Ensures that the image is not presented until all commands have been sumbitted and executed
	semaphores.render_complete = get_device()->get_handle().createSemaphore({});

	// Set up submit info structure
	// Semaphores will stay the same during application lifetime
	// Command buffer submission info is set by each example
	submit_info                   = vk::SubmitInfo();
	submit_info.pWaitDstStageMask = &submit_pipeline_stages;

	if (window->get_window_mode() != vkb::Window::Mode::Headless)
	{
		submit_info.setWaitSemaphores(semaphores.acquired_image_ready);
		submit_info.setSignalSemaphores(semaphores.render_complete);
	}

	queue = get_device()->get_suitable_graphics_queue().get_handle();

	create_swapchain_buffers();
	create_command_pool();
	create_command_buffers();
	create_synchronization_primitives();
	setup_depth_stencil();
	setup_render_pass();
	create_pipeline_cache();
	setup_framebuffer();

	extent = get_render_context().get_surface_extent();

	prepare_gui();

	return true;
}

void HPPApiVulkanSample::prepare_gui()
{
	gui = std::make_unique<vkb::HPPGui>(*this, *window, /*stats=*/nullptr, 15.0f, true);
	gui->prepare(pipeline_cache,
	             render_pass,
	             {static_cast<VkPipelineShaderStageCreateInfo>(load_shader("uioverlay/uioverlay.vert", vk::ShaderStageFlagBits::eVertex)),
	              static_cast<VkPipelineShaderStageCreateInfo>(load_shader("uioverlay/uioverlay.frag", vk::ShaderStageFlagBits::eFragment))});
}

void HPPApiVulkanSample::update(float delta_time)
{
	if (view_updated)
	{
		view_updated = false;
		view_changed();
	}

	render(delta_time);
	camera.update(delta_time);
	if (camera.moving())
	{
		view_updated = true;
	}
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
	get_device()->get_handle().waitIdle();

	create_swapchain_buffers();

	// Recreate the frame buffers
	get_device()->get_handle().destroyImageView(depth_stencil.view);
	get_device()->get_handle().destroyImage(depth_stencil.image);
	get_device()->get_handle().freeMemory(depth_stencil.mem);
	setup_depth_stencil();
	for (uint32_t i = 0; i < framebuffers.size(); i++)
	{
		get_device()->get_handle().destroyFramebuffer(framebuffers[i]);
		framebuffers[i] = nullptr;
	}
	setup_framebuffer();

	if (extent.width && extent.height && gui)
	{
		gui->resize(extent.width, extent.height);
	}

	rebuild_command_buffers();

	get_device()->get_handle().waitIdle();

	if (extent.width && extent.height)
	{
		camera.update_aspect_ratio(static_cast<float>(extent.width) / static_cast<float>(extent.height));
	}

	// Notify derived class
	view_changed();

	prepared = true;
	return true;
}

void HPPApiVulkanSample::create_render_context()
{
	// We always want an sRGB surface to match the display.
	// If we used a UNORM surface, we'd have to do the conversion to sRGB ourselves at the end of our fragment shaders.
	auto surface_priority_list = std::vector<vk::SurfaceFormatKHR>{{vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear},
	                                                               {vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear}};

	HPPVulkanSample::create_render_context(surface_priority_list);
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

					float deltaX = static_cast<float>(touch_pos.y - eventY) * rotation_speed * 0.5f;
					float deltaY = static_cast<float>(touch_pos.x - eventX) * rotation_speed * 0.5f;

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
					case vkb::KeyCode::F1:
						if (gui)
						{
							gui->visible = !gui->visible;
						}
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
	int32_t dx = static_cast<int32_t>(mouse_pos.x) - x;
	int32_t dy = static_cast<int32_t>(mouse_pos.y) - y;

	bool handled = false;

	if (gui)
	{
		ImGuiIO &io = ImGui::GetIO();
		handled     = io.WantCaptureMouse;
	}
	mouse_moved(static_cast<float>(x), static_cast<float>(y), handled);

	if (handled)
	{
		mouse_pos = glm::vec2(static_cast<float>(x), static_cast<float>(y));
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
	mouse_pos = glm::vec2(static_cast<float>(x), static_cast<float>(y));
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

	draw_cmd_buffers = get_device()->get_handle().allocateCommandBuffers(allocate_info);
}

void HPPApiVulkanSample::destroy_command_buffers()
{
	get_device()->get_handle().freeCommandBuffers(cmd_pool, draw_cmd_buffers);
}

void HPPApiVulkanSample::create_pipeline_cache()
{
	pipeline_cache = get_device()->get_handle().createPipelineCache({});
}

vk::PipelineShaderStageCreateInfo HPPApiVulkanSample::load_shader(const std::string &file, vk::ShaderStageFlagBits stage, vkb::ShaderSourceLanguage src_language)
{
	shader_modules.push_back(vkb::common::load_shader(file.c_str(), get_device()->get_handle(), stage, src_language));
	assert(shader_modules.back());
	return vk::PipelineShaderStageCreateInfo({}, stage, shader_modules.back(), "main");
}

void HPPApiVulkanSample::update_overlay(float delta_time, const std::function<void()> &additional_ui)
{
	if (gui)
	{
		gui->show_simple_window(get_name(), vkb::to_u32(1.0f / delta_time), [this, additional_ui]() { on_update_ui_overlay(gui->get_drawer()); additional_ui(); });

		gui->update(delta_time);

		if (gui->update_buffers() || gui->get_drawer().is_dirty())
		{
			rebuild_command_buffers();
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
			std::tie(result, current_buffer) = get_render_context().get_swapchain().acquire_next_image(semaphores.acquired_image_ready);
		}
		// Recreate the swapchain if it's no longer compatible with the surface (eErrorOutOfDateKHR)
		// Don't catch other failures here, they are propagated up the calling hierarchy
		catch (vk::OutOfDateKHRError & /*err*/)
		{
			resize(extent.width, extent.height);
		}
		// VK_SUBOPTIMAL_KHR is a success code and means that acquire was successful and semaphore is signaled but image is suboptimal
		// allow rendering frame to suboptimal swapchain as otherwise we would have to manually unsignal semaphore and acquire image again
	}
}

void HPPApiVulkanSample::submit_frame()
{
	if (get_render_context().has_swapchain())
	{
		const auto &queue = get_device()->get_queue_by_present(0);

		vk::SwapchainKHR swapchain = get_render_context().get_swapchain().get_handle();

		vk::PresentInfoKHR present_info({}, swapchain, current_buffer);
		// Check if a wait semaphore has been specified to wait for before presenting the image
		if (semaphores.render_complete)
		{
			present_info.setWaitSemaphores(semaphores.render_complete);
		}

		vk::DisplayPresentInfoKHR disp_present_info;
		if (device->is_extension_supported(VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME) &&
		    window->get_display_present_info(reinterpret_cast<VkDisplayPresentInfoKHR *>(&disp_present_info), extent.width, extent.height))
		{
			// Add display present info if supported and wanted
			present_info.setPNext(&disp_present_info);
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
	get_device()->get_queue_by_present(0).get_handle().waitIdle();
}

HPPApiVulkanSample::~HPPApiVulkanSample()
{
	if (get_device() && get_device()->get_handle())
	{
		vk::Device device = get_device()->get_handle();

		device.waitIdle();

		// Clean up Vulkan resources
		device.destroyDescriptorPool(descriptor_pool);
		destroy_command_buffers();
		device.destroyRenderPass(render_pass);
		for (auto &framebuffer : framebuffers)
		{
			device.destroyFramebuffer(framebuffer);
		}

		for (auto &swapchain_buffer : swapchain_buffers)
		{
			device.destroyImageView(swapchain_buffer.view);
		}

		for (auto &shader_module : shader_modules)
		{
			device.destroyShaderModule(shader_module);
		}
		device.destroyImageView(depth_stencil.view);
		device.destroyImage(depth_stencil.image);
		device.freeMemory(depth_stencil.mem);

		device.destroyPipelineCache(pipeline_cache);

		device.destroyCommandPool(cmd_pool);

		device.destroySemaphore(semaphores.acquired_image_ready);
		device.destroySemaphore(semaphores.render_complete);
		for (auto &fence : wait_fences)
		{
			device.destroyFence(fence);
		}
	}

	gui.reset();
}

void HPPApiVulkanSample::view_changed()
{}

void HPPApiVulkanSample::build_command_buffers()
{}

void HPPApiVulkanSample::rebuild_command_buffers()
{
	get_device()->get_handle().resetCommandPool(cmd_pool);
	build_command_buffers();
}

void HPPApiVulkanSample::create_synchronization_primitives()
{
	// Wait fences to sync command buffer access
	vk::FenceCreateInfo fence_create_info(vk::FenceCreateFlagBits::eSignaled);
	wait_fences.resize(draw_cmd_buffers.size());
	for (auto &fence : wait_fences)
	{
		fence = get_device()->get_handle().createFence(fence_create_info);
	}
}

void HPPApiVulkanSample::create_command_pool()
{
	uint32_t                  queue_family_index = get_device()->get_queue_by_flags(vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute, 0).get_family_index();
	vk::CommandPoolCreateInfo command_pool_info({}, queue_family_index);
	cmd_pool = get_device()->get_handle().createCommandPool(command_pool_info);
}

void HPPApiVulkanSample::setup_depth_stencil()
{
	std::tie(depth_stencil.image, depth_stencil.mem) = get_device()->create_image(depth_format, get_render_context().get_surface_extent(), 1, vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eDeviceLocal);

	vk::ImageAspectFlags aspect_mask = vk::ImageAspectFlagBits::eDepth;
	// Stencil aspect should only be set on depth + stencil formats
	if ((vk::componentCount(depth_format) == 2) && (strcmp(vk::componentName(depth_format, 0), "D") == 0) &&
	    (strcmp(vk::componentName(depth_format, 1), "S") == 0))
	{
		aspect_mask |= vk::ImageAspectFlagBits::eStencil;
	}

	depth_stencil.view = vkb::common::create_image_view(get_device()->get_handle(), depth_stencil.image, vk::ImageViewType::e2D, depth_format, aspect_mask);
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
		get_device()->get_handle().destroyFramebuffer(framebuffer);
	}
	framebuffers.clear();

	// Create frame buffers for every swap chain image
	framebuffers.reserve(swapchain_buffers.size());
	for (auto &buffer : swapchain_buffers)
	{
		attachments[0] = buffer.view;
		framebuffers.push_back(get_device()->get_handle().createFramebuffer(framebuffer_create_info));
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
	dependencies[0].dstStageMask    = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
	dependencies[0].srcAccessMask   = vk::AccessFlagBits::eNoneKHR;
	dependencies[0].dstAccessMask   = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
	dependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

	dependencies[1].srcSubpass      = 0;
	dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask    = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
	dependencies[1].dstStageMask    = vk::PipelineStageFlagBits::eBottomOfPipe;
	dependencies[1].srcAccessMask   = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
	dependencies[1].dstAccessMask   = vk::AccessFlagBits::eMemoryRead;
	dependencies[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

	vk::RenderPassCreateInfo render_pass_create_info({}, attachments, subpass_description, dependencies);

	render_pass = get_device()->get_handle().createRenderPass(render_pass_create_info);
}

void HPPApiVulkanSample::update_render_pass_flags(RenderPassCreateFlags flags)
{
	get_device()->get_handle().destroyRenderPass(render_pass);

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
	dependencies[0].dstStageMask    = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
	dependencies[0].srcAccessMask   = vk::AccessFlagBits::eNoneKHR;
	dependencies[0].dstAccessMask   = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
	dependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

	dependencies[1].srcSubpass      = 0;
	dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask    = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
	dependencies[1].dstStageMask    = vk::PipelineStageFlagBits::eBottomOfPipe;
	dependencies[1].srcAccessMask   = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
	dependencies[1].dstAccessMask   = vk::AccessFlagBits::eMemoryRead;
	dependencies[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

	vk::RenderPassCreateInfo render_pass_create_info({}, attachments, subpass_description, dependencies);

	render_pass = get_device()->get_handle().createRenderPass(render_pass_create_info);
}

void HPPApiVulkanSample::on_update_ui_overlay(vkb::Drawer &drawer)
{}

vk::Sampler HPPApiVulkanSample::create_default_sampler(vk::SamplerAddressMode address_mode, size_t mipmaps_count, vk::Format format)
{
	return vkb::common::create_sampler(
	    get_device()->get_gpu().get_handle(),
	    get_device()->get_handle(),
	    format,
	    vk::Filter::eLinear,
	    address_mode,
	    get_device()->get_gpu().get_features().samplerAnisotropy ? (get_device()->get_gpu().get_properties().limits.maxSamplerAnisotropy) : 1.0f,
	    static_cast<float>(mipmaps_count));
}

void HPPApiVulkanSample::create_swapchain_buffers()
{
	if (get_render_context().has_swapchain())
	{
		auto &images = get_render_context().get_swapchain().get_images();

		// Get the swap chain buffers containing the image and imageview
		for (auto &swapchain_buffer : swapchain_buffers)
		{
			get_device()->get_handle().destroyImageView(swapchain_buffer.view);
		}
		swapchain_buffers.clear();
		swapchain_buffers.reserve(images.size());
		for (auto &image : images)
		{
			swapchain_buffers.push_back(
			    {image, vkb::common::create_image_view(get_device()->get_handle(), image, vk::ImageViewType::e2D, get_render_context().get_swapchain().get_format())});
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
	    get_device()->get_gpu().get_handle().getSurfaceCapabilitiesKHR(get_render_context().get_swapchain().get_surface());

	if (surface_properties.currentExtent != get_render_context().get_surface_extent())
	{
		resize(surface_properties.currentExtent.width, surface_properties.currentExtent.height);
	}
}

vk::ImageLayout HPPApiVulkanSample::descriptor_type_to_image_layout(vk::DescriptorType descriptor_type, vk::Format format) const
{
	switch (descriptor_type)
	{
		case vk::DescriptorType::eCombinedImageSampler:
		case vk::DescriptorType::eInputAttachment:
			return vkb::common::is_depth_format(format) ? vk::ImageLayout::eDepthStencilReadOnlyOptimal : vk::ImageLayout::eShaderReadOnlyOptimal;
		case vk::DescriptorType::eStorageImage:
			return vk::ImageLayout::eGeneral;
		default:
			return vk::ImageLayout::eUndefined;
	}
}

HPPTexture HPPApiVulkanSample::load_texture(const std::string &file, vkb::scene_graph::components::HPPImage::ContentType content_type, vk::SamplerAddressMode address_mode)
{
	HPPTexture texture;

	texture.image = vkb::scene_graph::components::HPPImage::load(file, file, content_type);
	texture.image->create_vk_image(*get_device());

	const auto &queue = get_device()->get_queue_by_flags(vk::QueueFlagBits::eGraphics, 0);

	vk::CommandBuffer command_buffer = get_device()->create_command_buffer(vk::CommandBufferLevel::ePrimary, true);

	vkb::core::HPPBuffer stage_buffer = vkb::core::HPPBuffer::create_staging_buffer(*get_device(), texture.image->get_data());

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
	vkb::common::image_layout_transition(
	    command_buffer, texture.image->get_vk_image().get_handle(), vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, subresource_range);

	// Copy mip levels from staging buffer
	command_buffer.copyBufferToImage(
	    stage_buffer.get_handle(), texture.image->get_vk_image().get_handle(), vk::ImageLayout::eTransferDstOptimal, bufferCopyRegions);

	// Change texture image layout to shader read after all mip levels have been copied
	vkb::common::image_layout_transition(command_buffer,
	                                     texture.image->get_vk_image().get_handle(),
	                                     vk::ImageLayout::eTransferDstOptimal,
	                                     vk::ImageLayout::eShaderReadOnlyOptimal,
	                                     subresource_range);

	get_device()->flush_command_buffer(command_buffer, queue.get_handle());

	texture.sampler = create_default_sampler(address_mode, mipmaps.size(), texture.image->get_format());

	return texture;
}

HPPTexture HPPApiVulkanSample::load_texture_array(const std::string &file, vkb::scene_graph::components::HPPImage::ContentType content_type, vk::SamplerAddressMode address_mode)
{
	HPPTexture texture{};

	texture.image = vkb::scene_graph::components::HPPImage::load(file, file, content_type);
	texture.image->create_vk_image(*get_device(), vk::ImageViewType::e2DArray);

	const auto &queue = get_device()->get_queue_by_flags(vk::QueueFlagBits::eGraphics, 0);

	vk::CommandBuffer command_buffer = get_device()->create_command_buffer(vk::CommandBufferLevel::ePrimary, true);

	vkb::core::HPPBuffer stage_buffer = vkb::core::HPPBuffer::create_staging_buffer(*get_device(), texture.image->get_data());

	// Setup buffer copy regions for each mip level
	std::vector<vk::BufferImageCopy> buffer_copy_regions;

	auto       &mipmaps = texture.image->get_mipmaps();
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
	vkb::common::image_layout_transition(
	    command_buffer, texture.image->get_vk_image().get_handle(), vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, subresource_range);

	// Copy mip levels from staging buffer
	command_buffer.copyBufferToImage(
	    stage_buffer.get_handle(), texture.image->get_vk_image().get_handle(), vk::ImageLayout::eTransferDstOptimal, buffer_copy_regions);

	// Change texture image layout to shader read after all mip levels have been copied
	vkb::common::image_layout_transition(command_buffer,
	                                     texture.image->get_vk_image().get_handle(),
	                                     vk::ImageLayout::eTransferDstOptimal,
	                                     vk::ImageLayout::eShaderReadOnlyOptimal,
	                                     subresource_range);

	get_device()->flush_command_buffer(command_buffer, queue.get_handle());

	texture.sampler = create_default_sampler(address_mode, mipmaps.size(), texture.image->get_format());

	return texture;
}

HPPTexture HPPApiVulkanSample::load_texture_cubemap(const std::string &file, vkb::scene_graph::components::HPPImage::ContentType content_type)
{
	HPPTexture texture{};

	texture.image = vkb::scene_graph::components::HPPImage::load(file, file, content_type);
	texture.image->create_vk_image(*get_device(), vk::ImageViewType::eCube, vk::ImageCreateFlagBits::eCubeCompatible);

	const auto &queue = get_device()->get_queue_by_flags(vk::QueueFlagBits::eGraphics, 0);

	vk::CommandBuffer command_buffer = get_device()->create_command_buffer(vk::CommandBufferLevel::ePrimary, true);

	vkb::core::HPPBuffer stage_buffer = vkb::core::HPPBuffer::create_staging_buffer(*get_device(), texture.image->get_data());

	// Setup buffer copy regions for each mip level
	std::vector<vk::BufferImageCopy> buffer_copy_regions;

	auto       &mipmaps = texture.image->get_mipmaps();
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
	vkb::common::image_layout_transition(
	    command_buffer, texture.image->get_vk_image().get_handle(), vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, subresource_range);

	// Copy mip levels from staging buffer
	command_buffer.copyBufferToImage(
	    stage_buffer.get_handle(), texture.image->get_vk_image().get_handle(), vk::ImageLayout::eTransferDstOptimal, buffer_copy_regions);

	// Change texture image layout to shader read after all mip levels have been copied
	vkb::common::image_layout_transition(command_buffer,
	                                     texture.image->get_vk_image().get_handle(),
	                                     vk::ImageLayout::eTransferDstOptimal,
	                                     vk::ImageLayout::eShaderReadOnlyOptimal,
	                                     subresource_range);

	get_device()->flush_command_buffer(command_buffer, queue.get_handle());

	texture.sampler = create_default_sampler(vk::SamplerAddressMode::eClampToEdge, mipmaps.size(), texture.image->get_format());

	return texture;
}

std::unique_ptr<vkb::scene_graph::components::HPPSubMesh> HPPApiVulkanSample::load_model(const std::string &file, uint32_t index)
{
	vkb::HPPGLTFLoader loader{*get_device()};

	std::unique_ptr<vkb::scene_graph::components::HPPSubMesh> model = loader.read_model_from_file(file, index);

	if (!model)
	{
		LOGE("Cannot load model from file: {}", file.c_str());
		throw std::runtime_error("Cannot load model from: " + file);
	}

	return model;
}

void HPPApiVulkanSample::draw_model(std::unique_ptr<vkb::scene_graph::components::HPPSubMesh> &model, vk::CommandBuffer command_buffer, uint32_t instance_count)
{
	vk::DeviceSize offset = 0;

	const auto &vertex_buffer = model->get_vertex_buffer("vertex_buffer");
	auto       &index_buffer  = model->get_index_buffer();

	command_buffer.bindVertexBuffers(0, vertex_buffer.get_handle(), offset);
	command_buffer.bindIndexBuffer(index_buffer.get_handle(), 0, model->get_index_type());
	command_buffer.drawIndexed(model->vertex_indices, instance_count, 0, 0, 0);
}

void HPPApiVulkanSample::with_command_buffer(const std::function<void(vk::CommandBuffer command_buffer)> &f, vk::Semaphore signalSemaphore)
{
	vk::CommandBuffer command_buffer = get_device()->create_command_buffer(vk::CommandBufferLevel::ePrimary, true);
	f(command_buffer);
	get_device()->flush_command_buffer(command_buffer, queue, true, signalSemaphore);
}
