/* Copyright (c) 2019-2024, Sascha Willems
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

#include "api_vulkan_sample.h"

#include "core/device.h"
#include "core/swapchain.h"
#include "gltf_loader.h"
#include "scene_graph/components/image.h"
#include "scene_graph/components/sampler.h"
#include "scene_graph/components/sub_mesh.h"
#include "scene_graph/components/texture.h"

bool ApiVulkanSample::prepare(const vkb::ApplicationOptions &options)
{
	if (!VulkanSample::prepare(options))
	{
		return false;
	}

	depth_format = vkb::get_suitable_depth_format(get_device().get_gpu().get_handle());

	// Create synchronization objects
	VkSemaphoreCreateInfo semaphore_create_info = vkb::initializers::semaphore_create_info();
	// Create a semaphore used to synchronize image presentation
	// Ensures that the current swapchain render target has completed presentation and has been released by the presentation engine, ready for rendering
	VK_CHECK(vkCreateSemaphore(get_device().get_handle(), &semaphore_create_info, nullptr, &semaphores.acquired_image_ready));
	// Create a semaphore used to synchronize command submission
	// Ensures that the image is not presented until all commands have been sumbitted and executed
	VK_CHECK(vkCreateSemaphore(get_device().get_handle(), &semaphore_create_info, nullptr, &semaphores.render_complete));

	// Set up submit info structure
	// Semaphores will stay the same during application lifetime
	// Command buffer submission info is set by each example
	submit_info                   = vkb::initializers::submit_info();
	submit_info.pWaitDstStageMask = &submit_pipeline_stages;

	if (window->get_window_mode() != vkb::Window::Mode::Headless)
	{
		submit_info.waitSemaphoreCount   = 1;
		submit_info.pWaitSemaphores      = &semaphores.acquired_image_ready;
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores    = &semaphores.render_complete;
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

	width  = get_render_context().get_surface_extent().width;
	height = get_render_context().get_surface_extent().height;

	prepare_gui();

	return true;
}

void ApiVulkanSample::prepare_gui()
{
	create_gui(*window, nullptr, 15.0f, true);
	get_gui().prepare(pipeline_cache, render_pass,
	                  {load_shader("uioverlay/uioverlay.vert", VK_SHADER_STAGE_VERTEX_BIT),
	                   load_shader("uioverlay/uioverlay.frag", VK_SHADER_STAGE_FRAGMENT_BIT)});
}

void ApiVulkanSample::update(float delta_time)
{
	if (view_updated)
	{
		view_updated = false;
		view_changed();
	}

	assert(has_render_context());
	render(delta_time);
	camera.update(delta_time);
	if (camera.moving())
	{
		view_updated = true;
	}
}

bool ApiVulkanSample::resize(const uint32_t _width, const uint32_t _height)
{
	if (!prepared)
	{
		return false;
	}

	get_render_context().handle_surface_changes();

	// Don't recreate the swapchain if the dimensions haven't changed
	if (width == get_render_context().get_surface_extent().width && height == get_render_context().get_surface_extent().height)
	{
		return false;
	}

	width  = get_render_context().get_surface_extent().width;
	height = get_render_context().get_surface_extent().height;

	prepared = false;

	// Ensure all operations on the device have been finished before destroying resources
	get_device().wait_idle();

	create_swapchain_buffers();

	// Recreate the frame buffers
	vkDestroyImageView(get_device().get_handle(), depth_stencil.view, nullptr);
	vkDestroyImage(get_device().get_handle(), depth_stencil.image, nullptr);
	vkFreeMemory(get_device().get_handle(), depth_stencil.mem, nullptr);
	setup_depth_stencil();
	for (uint32_t i = 0; i < framebuffers.size(); i++)
	{
		vkDestroyFramebuffer(get_device().get_handle(), framebuffers[i], nullptr);
		framebuffers[i] = VK_NULL_HANDLE;
	}
	setup_framebuffer();

	if ((width > 0.0f) && (height > 0.0f))
	{
		if (has_gui())
		{
			get_gui().resize(width, height);
		}
	}

	rebuild_command_buffers();

	get_device().wait_idle();

	if ((width > 0.0f) && (height > 0.0f))
	{
		camera.update_aspect_ratio(static_cast<float>(width) / static_cast<float>(height));
	}

	// Notify derived class
	view_changed();

	prepared = true;
	return true;
}

void ApiVulkanSample::create_render_context()
{
	// We always want an sRGB surface to match the display.
	// If we used a UNORM surface, we'd have to do the conversion to sRGB ourselves at the end of our fragment shaders.
	auto surface_priority_list = std::vector<VkSurfaceFormatKHR>{{VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
	                                                             {VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}};

	VulkanSample::create_render_context(surface_priority_list);
}

void ApiVulkanSample::input_event(const vkb::InputEvent &input_event)
{
	VulkanSample::input_event(input_event);

	bool gui_captures_event = false;

	if (has_gui())
	{
		gui_captures_event = get_gui().input_event(input_event);
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
				if (has_gui())
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
						if (has_gui())
						{
							get_gui().visible = !get_gui().visible;
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

void ApiVulkanSample::handle_mouse_move(int32_t x, int32_t y)
{
	int32_t dx = static_cast<int32_t>(mouse_pos.x) - x;
	int32_t dy = static_cast<int32_t>(mouse_pos.y) - y;

	bool handled = false;

	if (has_gui())
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

void ApiVulkanSample::mouse_moved(double x, double y, bool &handled)
{}

bool ApiVulkanSample::check_command_buffers()
{
	for (auto &command_buffer : draw_cmd_buffers)
	{
		if (command_buffer == VK_NULL_HANDLE)
		{
			return false;
		}
	}
	return true;
}

void ApiVulkanSample::create_command_buffers()
{
	// Create one command buffer for each swap chain image and reuse for rendering
	draw_cmd_buffers.resize(get_render_context().get_render_frames().size());

	VkCommandBufferAllocateInfo allocate_info =
	    vkb::initializers::command_buffer_allocate_info(
	        cmd_pool,
	        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
	        static_cast<uint32_t>(draw_cmd_buffers.size()));

	VK_CHECK(vkAllocateCommandBuffers(get_device().get_handle(), &allocate_info, draw_cmd_buffers.data()));
}

void ApiVulkanSample::destroy_command_buffers()
{
	vkFreeCommandBuffers(get_device().get_handle(), cmd_pool, static_cast<uint32_t>(draw_cmd_buffers.size()), draw_cmd_buffers.data());
}

void ApiVulkanSample::recreate_current_command_buffer()
{
	auto &cmd = draw_cmd_buffers[current_buffer];
	assert(cmd);
	vkFreeCommandBuffers(get_device().get_handle(), cmd_pool, 1, &cmd);
	VkCommandBufferAllocateInfo command_buffer_allocate_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr, cmd_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1};
	VK_CHECK(vkAllocateCommandBuffers(get_device().get_handle(), &command_buffer_allocate_info, &cmd));
}

void ApiVulkanSample::create_pipeline_cache()
{
	VkPipelineCacheCreateInfo pipeline_cache_create_info = {};
	pipeline_cache_create_info.sType                     = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	VK_CHECK(vkCreatePipelineCache(get_device().get_handle(), &pipeline_cache_create_info, nullptr, &pipeline_cache));
}

VkPipelineShaderStageCreateInfo ApiVulkanSample::load_shader(const std::string &file, VkShaderStageFlagBits stage, vkb::ShaderSourceLanguage src_language)
{
	VkPipelineShaderStageCreateInfo shader_stage = {};
	shader_stage.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stage.stage                           = stage;
	shader_stage.module                          = vkb::load_shader(file.c_str(), get_device().get_handle(), stage, src_language);
	shader_stage.pName                           = "main";
	assert(shader_stage.module != VK_NULL_HANDLE);
	shader_modules.push_back(shader_stage.module);
	return shader_stage;
}

VkPipelineShaderStageCreateInfo ApiVulkanSample::load_shader(const std::string &sample_folder_name, const std::string &shader_filename, VkShaderStageFlagBits stage)
{
	// Note: this can be reworked once offline compilation for GLSL shaders is added

	// Default to GLSL
	std::string               shader_folder{"glsl"};
	std::string               shader_extension{""};
	vkb::ShaderSourceLanguage src_language = vkb::ShaderSourceLanguage::GLSL;

	if (get_shading_language() == vkb::ShadingLanguage::HLSL)
	{
		shader_folder = "hlsl";
		// HLSL shaders are offline compiled to SPIR-V, so source is SPV
		src_language     = vkb::ShaderSourceLanguage::SPV;
		shader_extension = ".spv";
	}

	std::string full_file_name = sample_folder_name + "/" + shader_folder + "/" + shader_filename + shader_extension;

	VkPipelineShaderStageCreateInfo shader_stage = {};
	shader_stage.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stage.stage                           = stage;
	shader_stage.module                          = vkb::load_shader(full_file_name, get_device().get_handle(), stage, src_language);
	shader_stage.pName                           = "main";
	assert(shader_stage.module != VK_NULL_HANDLE);
	shader_modules.push_back(shader_stage.module);
	return shader_stage;
}

void ApiVulkanSample::update_overlay(float delta_time, const std::function<void()> &additional_ui)
{
	if (has_gui())
	{
		frame_count++;
		accumulated_time += delta_time;
		if (0.5f < accumulated_time)
		{
			fps              = static_cast<uint32_t>(frame_count / accumulated_time);
			frame_count      = 0;
			accumulated_time = 0.0f;
		}

		get_gui().show_simple_window(get_name(), fps, [this, additional_ui]() {
			on_update_ui_overlay(get_gui().get_drawer());
			additional_ui();
		});

		get_gui().update(delta_time);

		if (get_gui().update_buffers() || get_gui().get_drawer().is_dirty())
		{
			rebuild_command_buffers();
			get_gui().get_drawer().clear();
		}
	}
}

void ApiVulkanSample::draw_ui(const VkCommandBuffer command_buffer)
{
	if (has_gui())
	{
		const VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		const VkRect2D   scissor  = vkb::initializers::rect2D(width, height, 0, 0);
		vkCmdSetViewport(command_buffer, 0, 1, &viewport);
		vkCmdSetScissor(command_buffer, 0, 1, &scissor);

		get_gui().draw(command_buffer);
	}
}

void ApiVulkanSample::prepare_frame()
{
	if (get_render_context().has_swapchain())
	{
		handle_surface_changes();
		// Acquire the next image from the swap chain
		VkResult result = get_render_context().get_swapchain().acquire_next_image(current_buffer, semaphores.acquired_image_ready, VK_NULL_HANDLE);
		// Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE)
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			resize(width, height);
		}
		// VK_SUBOPTIMAL_KHR means that acquire was successful and semaphore is signaled but image is suboptimal
		// allow rendering frame to suboptimal swapchain as otherwise we would have to manually unsignal semaphore and acquire image again
		else if (result != VK_SUBOPTIMAL_KHR)
		{
			VK_CHECK(result);
		}
	}
}

void ApiVulkanSample::submit_frame()
{
	if (get_render_context().has_swapchain())
	{
		const auto &queue = get_device().get_queue_by_present(0);

		VkSwapchainKHR sc = get_render_context().get_swapchain().get_handle();

		VkPresentInfoKHR present_info = {};
		present_info.sType            = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.pNext            = NULL;
		present_info.swapchainCount   = 1;
		present_info.pSwapchains      = &sc;
		present_info.pImageIndices    = &current_buffer;

		VkDisplayPresentInfoKHR disp_present_info{};
		if (get_device().is_extension_supported(VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME) &&
		    window->get_display_present_info(&disp_present_info, width, height))
		{
			// Add display present info if supported and wanted
			present_info.pNext = &disp_present_info;
		}

		// Check if a wait semaphore has been specified to wait for before presenting the image
		if (semaphores.render_complete != VK_NULL_HANDLE)
		{
			present_info.pWaitSemaphores    = &semaphores.render_complete;
			present_info.waitSemaphoreCount = 1;
		}

		VkResult present_result = queue.present(present_info);

		if (!((present_result == VK_SUCCESS) || (present_result == VK_SUBOPTIMAL_KHR)))
		{
			if (present_result == VK_ERROR_OUT_OF_DATE_KHR)
			{
				// Swap chain is no longer compatible with the surface and needs to be recreated
				resize(width, height);
				return;
			}
			else
			{
				VK_CHECK(present_result);
			}
		}
	}

	// DO NOT USE
	// vkDeviceWaitIdle and vkQueueWaitIdle are extremely expensive functions, and are used here purely for demonstrating the vulkan API
	// without having to concern ourselves with proper syncronization. These functions should NEVER be used inside the render loop like this (every frame).
	VK_CHECK(get_device().get_queue_by_present(0).wait_idle());
}

ApiVulkanSample::~ApiVulkanSample()
{
	if (has_device())
	{
		get_device().wait_idle();

		// Clean up Vulkan resources
		if (descriptor_pool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(get_device().get_handle(), descriptor_pool, nullptr);
		}
		destroy_command_buffers();
		if (render_pass != VK_NULL_HANDLE)
		{
			vkDestroyRenderPass(get_device().get_handle(), render_pass, nullptr);
		}
		for (uint32_t i = 0; i < framebuffers.size(); i++)
		{
			vkDestroyFramebuffer(get_device().get_handle(), framebuffers[i], nullptr);
		}

		for (auto &swapchain_buffer : swapchain_buffers)
		{
			vkDestroyImageView(get_device().get_handle(), swapchain_buffer.view, nullptr);
		}

		for (auto &shader_module : shader_modules)
		{
			vkDestroyShaderModule(get_device().get_handle(), shader_module, nullptr);
		}
		vkDestroyImageView(get_device().get_handle(), depth_stencil.view, nullptr);
		vkDestroyImage(get_device().get_handle(), depth_stencil.image, nullptr);
		vkFreeMemory(get_device().get_handle(), depth_stencil.mem, nullptr);

		vkDestroyPipelineCache(get_device().get_handle(), pipeline_cache, nullptr);

		vkDestroyCommandPool(get_device().get_handle(), cmd_pool, nullptr);

		vkDestroySemaphore(get_device().get_handle(), semaphores.acquired_image_ready, nullptr);
		vkDestroySemaphore(get_device().get_handle(), semaphores.render_complete, nullptr);
		for (auto &fence : wait_fences)
		{
			vkDestroyFence(get_device().get_handle(), fence, nullptr);
		}
	}
}

void ApiVulkanSample::view_changed()
{}

void ApiVulkanSample::build_command_buffers()
{}

void ApiVulkanSample::rebuild_command_buffers()
{
	vkResetCommandPool(get_device().get_handle(), cmd_pool, 0);
	build_command_buffers();
}

void ApiVulkanSample::create_synchronization_primitives()
{
	// Wait fences to sync command buffer access
	VkFenceCreateInfo fence_create_info = vkb::initializers::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
	wait_fences.resize(draw_cmd_buffers.size());
	for (auto &fence : wait_fences)
	{
		VK_CHECK(vkCreateFence(get_device().get_handle(), &fence_create_info, nullptr, &fence));
	}
}

void ApiVulkanSample::create_command_pool()
{
	VkCommandPoolCreateInfo command_pool_info = {};
	command_pool_info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_info.queueFamilyIndex        = get_device().get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, 0).get_family_index();
	VK_CHECK(vkCreateCommandPool(get_device().get_handle(), &command_pool_info, nullptr, &cmd_pool));
}

void ApiVulkanSample::setup_depth_stencil()
{
	VkImageCreateInfo image_create_info{};
	image_create_info.sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_create_info.imageType   = VK_IMAGE_TYPE_2D;
	image_create_info.format      = depth_format;
	image_create_info.extent      = {get_render_context().get_surface_extent().width, get_render_context().get_surface_extent().height, 1};
	image_create_info.mipLevels   = 1;
	image_create_info.arrayLayers = 1;
	image_create_info.samples     = VK_SAMPLE_COUNT_1_BIT;
	image_create_info.tiling      = VK_IMAGE_TILING_OPTIMAL;
	image_create_info.usage       = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	VK_CHECK(vkCreateImage(get_device().get_handle(), &image_create_info, nullptr, &depth_stencil.image));
	VkMemoryRequirements memReqs{};
	vkGetImageMemoryRequirements(get_device().get_handle(), depth_stencil.image, &memReqs);

	VkMemoryAllocateInfo memory_allocation{};
	memory_allocation.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocation.allocationSize  = memReqs.size;
	memory_allocation.memoryTypeIndex = get_device().get_memory_type(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK(vkAllocateMemory(get_device().get_handle(), &memory_allocation, nullptr, &depth_stencil.mem));
	VK_CHECK(vkBindImageMemory(get_device().get_handle(), depth_stencil.image, depth_stencil.mem, 0));

	VkImageViewCreateInfo image_view_create_info{};
	image_view_create_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	image_view_create_info.image                           = depth_stencil.image;
	image_view_create_info.format                          = depth_format;
	image_view_create_info.subresourceRange.baseMipLevel   = 0;
	image_view_create_info.subresourceRange.levelCount     = 1;
	image_view_create_info.subresourceRange.baseArrayLayer = 0;
	image_view_create_info.subresourceRange.layerCount     = 1;
	image_view_create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
	// Stencil aspect should only be set on depth + stencil formats (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT
	if (depth_format >= VK_FORMAT_D16_UNORM_S8_UINT)
	{
		image_view_create_info.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	VK_CHECK(vkCreateImageView(get_device().get_handle(), &image_view_create_info, nullptr, &depth_stencil.view));
}

void ApiVulkanSample::setup_framebuffer()
{
	VkImageView attachments[2];

	// Depth/Stencil attachment is the same for all frame buffers
	attachments[1] = depth_stencil.view;

	VkFramebufferCreateInfo framebuffer_create_info = {};
	framebuffer_create_info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebuffer_create_info.pNext                   = NULL;
	framebuffer_create_info.renderPass              = render_pass;
	framebuffer_create_info.attachmentCount         = 2;
	framebuffer_create_info.pAttachments            = attachments;
	framebuffer_create_info.width                   = get_render_context().get_surface_extent().width;
	framebuffer_create_info.height                  = get_render_context().get_surface_extent().height;
	framebuffer_create_info.layers                  = 1;

	// Delete existing frame buffers
	if (framebuffers.size() > 0)
	{
		for (uint32_t i = 0; i < framebuffers.size(); i++)
		{
			if (framebuffers[i] != VK_NULL_HANDLE)
			{
				vkDestroyFramebuffer(get_device().get_handle(), framebuffers[i], nullptr);
			}
		}
	}

	// Create frame buffers for every swap chain image
	framebuffers.resize(get_render_context().get_render_frames().size());
	for (uint32_t i = 0; i < framebuffers.size(); i++)
	{
		attachments[0] = swapchain_buffers[i].view;
		VK_CHECK(vkCreateFramebuffer(get_device().get_handle(), &framebuffer_create_info, nullptr, &framebuffers[i]));
	}
}

void ApiVulkanSample::setup_render_pass()
{
	std::array<VkAttachmentDescription, 2> attachments = {};
	// Color attachment
	attachments[0].format         = get_render_context().get_format();
	attachments[0].samples        = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	// Depth attachment
	attachments[1].format         = depth_format;
	attachments[1].samples        = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference color_reference = {};
	color_reference.attachment            = 0;
	color_reference.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_reference = {};
	depth_reference.attachment            = 1;
	depth_reference.layout                = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass_description    = {};
	subpass_description.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass_description.colorAttachmentCount    = 1;
	subpass_description.pColorAttachments       = &color_reference;
	subpass_description.pDepthStencilAttachment = &depth_reference;
	subpass_description.inputAttachmentCount    = 0;
	subpass_description.pInputAttachments       = nullptr;
	subpass_description.preserveAttachmentCount = 0;
	subpass_description.pPreserveAttachments    = nullptr;
	subpass_description.pResolveAttachments     = nullptr;

	// Subpass dependencies for layout transitions
	std::array<VkSubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass      = 0;
	dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[0].srcAccessMask   = VK_ACCESS_NONE_KHR;
	dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass      = 0;
	dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo render_pass_create_info = {};
	render_pass_create_info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_create_info.attachmentCount        = static_cast<uint32_t>(attachments.size());
	render_pass_create_info.pAttachments           = attachments.data();
	render_pass_create_info.subpassCount           = 1;
	render_pass_create_info.pSubpasses             = &subpass_description;
	render_pass_create_info.dependencyCount        = static_cast<uint32_t>(dependencies.size());
	render_pass_create_info.pDependencies          = dependencies.data();

	VK_CHECK(vkCreateRenderPass(get_device().get_handle(), &render_pass_create_info, nullptr, &render_pass));
}

void ApiVulkanSample::update_render_pass_flags(uint32_t flags)
{
	vkDestroyRenderPass(get_device().get_handle(), render_pass, nullptr);

	VkAttachmentLoadOp  color_attachment_load_op      = VK_ATTACHMENT_LOAD_OP_CLEAR;
	VkAttachmentStoreOp color_attachment_store_op     = VK_ATTACHMENT_STORE_OP_STORE;
	VkImageLayout       color_attachment_image_layout = VK_IMAGE_LAYOUT_UNDEFINED;

	// Samples can keep the color attachment contents, e.g. if they have previously written to the swap chain images
	if (flags & RenderPassCreateFlags::ColorAttachmentLoad)
	{
		color_attachment_load_op      = VK_ATTACHMENT_LOAD_OP_LOAD;
		color_attachment_image_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}

	std::array<VkAttachmentDescription, 2> attachments = {};
	// Color attachment
	attachments[0].format         = get_render_context().get_format();
	attachments[0].samples        = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp         = color_attachment_load_op;
	attachments[0].storeOp        = color_attachment_store_op;
	attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout  = color_attachment_image_layout;
	attachments[0].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	// Depth attachment
	attachments[1].format         = depth_format;
	attachments[1].samples        = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference color_reference = {};
	color_reference.attachment            = 0;
	color_reference.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_reference = {};
	depth_reference.attachment            = 1;
	depth_reference.layout                = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass_description    = {};
	subpass_description.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass_description.colorAttachmentCount    = 1;
	subpass_description.pColorAttachments       = &color_reference;
	subpass_description.pDepthStencilAttachment = &depth_reference;
	subpass_description.inputAttachmentCount    = 0;
	subpass_description.pInputAttachments       = nullptr;
	subpass_description.preserveAttachmentCount = 0;
	subpass_description.pPreserveAttachments    = nullptr;
	subpass_description.pResolveAttachments     = nullptr;

	// Subpass dependencies for layout transitions
	std::array<VkSubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass      = 0;
	dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[0].srcAccessMask   = VK_ACCESS_NONE_KHR;
	dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass      = 0;
	dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo render_pass_create_info = {};
	render_pass_create_info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_create_info.attachmentCount        = static_cast<uint32_t>(attachments.size());
	render_pass_create_info.pAttachments           = attachments.data();
	render_pass_create_info.subpassCount           = 1;
	render_pass_create_info.pSubpasses             = &subpass_description;
	render_pass_create_info.dependencyCount        = static_cast<uint32_t>(dependencies.size());
	render_pass_create_info.pDependencies          = dependencies.data();

	VK_CHECK(vkCreateRenderPass(get_device().get_handle(), &render_pass_create_info, nullptr, &render_pass));
}

void ApiVulkanSample::on_update_ui_overlay(vkb::Drawer &drawer)
{}

void ApiVulkanSample::create_swapchain_buffers()
{
	if (get_render_context().has_swapchain())
	{
		auto &images = get_render_context().get_swapchain().get_images();

		// Get the swap chain buffers containing the image and imageview
		for (auto &swapchain_buffer : swapchain_buffers)
		{
			vkDestroyImageView(get_device().get_handle(), swapchain_buffer.view, nullptr);
		}
		swapchain_buffers.clear();
		swapchain_buffers.resize(images.size());
		for (uint32_t i = 0; i < images.size(); i++)
		{
			VkImageViewCreateInfo color_attachment_view = {};
			color_attachment_view.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			color_attachment_view.pNext                 = NULL;
			color_attachment_view.format                = get_render_context().get_swapchain().get_format();
			color_attachment_view.components            = {
                VK_COMPONENT_SWIZZLE_R,
                VK_COMPONENT_SWIZZLE_G,
                VK_COMPONENT_SWIZZLE_B,
                VK_COMPONENT_SWIZZLE_A};
			color_attachment_view.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
			color_attachment_view.subresourceRange.baseMipLevel   = 0;
			color_attachment_view.subresourceRange.levelCount     = 1;
			color_attachment_view.subresourceRange.baseArrayLayer = 0;
			color_attachment_view.subresourceRange.layerCount     = 1;
			color_attachment_view.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
			color_attachment_view.flags                           = 0;

			swapchain_buffers[i].image = images[i];

			color_attachment_view.image = swapchain_buffers[i].image;

			VK_CHECK(vkCreateImageView(get_device().get_handle(), &color_attachment_view, nullptr, &swapchain_buffers[i].view));
		}
	}
	else
	{
		auto &frames = get_render_context().get_render_frames();

		// Get the swap chain buffers containing the image and imageview
		swapchain_buffers.clear();
		swapchain_buffers.resize(frames.size());
		for (uint32_t i = 0; i < frames.size(); i++)
		{
			auto &image_view = *frames[i]->get_render_target().get_views().begin();

			swapchain_buffers[i].image = image_view.get_image().get_handle();
			swapchain_buffers[i].view  = image_view.get_handle();
		}
	}
}

void ApiVulkanSample::update_swapchain_image_usage_flags(std::set<VkImageUsageFlagBits> image_usage_flags)
{
	get_render_context().update_swapchain(image_usage_flags);
	create_swapchain_buffers();
	setup_framebuffer();
}

void ApiVulkanSample::handle_surface_changes()
{
	VkSurfaceCapabilitiesKHR surface_properties;
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(get_device().get_gpu().get_handle(),
	                                                   get_render_context().get_swapchain().get_surface(),
	                                                   &surface_properties));

	if (surface_properties.currentExtent.width != get_render_context().get_surface_extent().width ||
	    surface_properties.currentExtent.height != get_render_context().get_surface_extent().height)
	{
		resize(surface_properties.currentExtent.width, surface_properties.currentExtent.height);
	}
}

VkDescriptorBufferInfo ApiVulkanSample::create_descriptor(vkb::core::BufferC &buffer, VkDeviceSize size, VkDeviceSize offset)
{
	VkDescriptorBufferInfo descriptor{};
	descriptor.buffer = buffer.get_handle();
	descriptor.range  = size;
	descriptor.offset = offset;
	return descriptor;
}

VkDescriptorImageInfo ApiVulkanSample::create_descriptor(Texture &texture, VkDescriptorType descriptor_type)
{
	VkDescriptorImageInfo descriptor{};
	descriptor.sampler   = texture.sampler;
	descriptor.imageView = texture.image->get_vk_image_view().get_handle();

	// Add image layout info based on descriptor type
	switch (descriptor_type)
	{
		case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
		case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
			if (vkb::is_depth_stencil_format(texture.image->get_vk_image_view().get_format()))
			{
				descriptor.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			}
			else
			{
				assert(!vkb::is_depth_format(texture.image->get_vk_image_view().get_format()));
				descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}
			break;
		case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			break;
		default:
			descriptor.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			break;
	}

	return descriptor;
}

Texture ApiVulkanSample::load_texture(const std::string &file, vkb::sg::Image::ContentType content_type)
{
	Texture texture{};

	texture.image = vkb::sg::Image::load(file, file, content_type);
	texture.image->create_vk_image(get_device());

	const auto &queue = get_device().get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);

	VkCommandBuffer command_buffer = get_device().create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	vkb::core::BufferC stage_buffer = vkb::core::BufferC::create_staging_buffer(get_device(), texture.image->get_data());

	// Setup buffer copy regions for each mip level
	std::vector<VkBufferImageCopy> bufferCopyRegions;

	auto &mipmaps = texture.image->get_mipmaps();

	for (size_t i = 0; i < mipmaps.size(); i++)
	{
		VkBufferImageCopy buffer_copy_region               = {};
		buffer_copy_region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		buffer_copy_region.imageSubresource.mipLevel       = vkb::to_u32(i);
		buffer_copy_region.imageSubresource.baseArrayLayer = 0;
		buffer_copy_region.imageSubresource.layerCount     = 1;
		buffer_copy_region.imageExtent.width               = texture.image->get_extent().width >> i;
		buffer_copy_region.imageExtent.height              = texture.image->get_extent().height >> i;
		buffer_copy_region.imageExtent.depth               = 1;
		buffer_copy_region.bufferOffset                    = mipmaps[i].offset;

		bufferCopyRegions.push_back(buffer_copy_region);
	}

	VkImageSubresourceRange subresource_range = {};
	subresource_range.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource_range.baseMipLevel            = 0;
	subresource_range.levelCount              = vkb::to_u32(mipmaps.size());
	subresource_range.layerCount              = 1;

	// Image barrier for optimal image (target)
	// Optimal image will be used as destination for the copy
	vkb::image_layout_transition(command_buffer,
	                             texture.image->get_vk_image().get_handle(),
	                             VK_IMAGE_LAYOUT_UNDEFINED,
	                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	                             subresource_range);

	// Copy mip levels from staging buffer
	vkCmdCopyBufferToImage(
	    command_buffer,
	    stage_buffer.get_handle(),
	    texture.image->get_vk_image().get_handle(),
	    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	    static_cast<uint32_t>(bufferCopyRegions.size()),
	    bufferCopyRegions.data());

	// Change texture image layout to shader read after all mip levels have been copied
	vkb::image_layout_transition(command_buffer,
	                             texture.image->get_vk_image().get_handle(),
	                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	                             subresource_range);

	get_device().flush_command_buffer(command_buffer, queue.get_handle());

	// Calculate valid filter and mipmap modes
	VkFilter            filter      = VK_FILTER_LINEAR;
	VkSamplerMipmapMode mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	vkb::make_filters_valid(get_device().get_gpu().get_handle(), texture.image->get_format(), &filter, &mipmap_mode);

	// Create a defaultsampler
	VkSamplerCreateInfo sampler_create_info = {};
	sampler_create_info.sType               = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_create_info.magFilter           = filter;
	sampler_create_info.minFilter           = filter;
	sampler_create_info.mipmapMode          = mipmap_mode;
	sampler_create_info.addressModeU        = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_create_info.addressModeV        = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_create_info.addressModeW        = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_create_info.mipLodBias          = 0.0f;
	sampler_create_info.compareOp           = VK_COMPARE_OP_NEVER;
	sampler_create_info.minLod              = 0.0f;
	// Max level-of-detail should match mip level count
	sampler_create_info.maxLod = static_cast<float>(mipmaps.size());
	// Only enable anisotropic filtering if enabled on the device
	// Note that for simplicity, we will always be using max. available anisotropy level for the current device
	// This may have an impact on performance, esp. on lower-specced devices
	// In a real-world scenario the level of anisotropy should be a user setting or e.g. lowered for mobile devices by default
	sampler_create_info.maxAnisotropy    = get_device().get_gpu().get_requested_features().samplerAnisotropy ? (get_device().get_gpu().get_properties().limits.maxSamplerAnisotropy) : 1.0f;
	sampler_create_info.anisotropyEnable = get_device().get_gpu().get_requested_features().samplerAnisotropy;
	sampler_create_info.borderColor      = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	VK_CHECK(vkCreateSampler(get_device().get_handle(), &sampler_create_info, nullptr, &texture.sampler));

	return texture;
}

Texture ApiVulkanSample::load_texture_array(const std::string &file, vkb::sg::Image::ContentType content_type)
{
	Texture texture{};

	texture.image = vkb::sg::Image::load(file, file, content_type);
	texture.image->create_vk_image(get_device(), VK_IMAGE_VIEW_TYPE_2D_ARRAY);

	const auto &queue = get_device().get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);

	VkCommandBuffer command_buffer = get_device().create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	vkb::core::BufferC stage_buffer = vkb::core::BufferC::create_staging_buffer(get_device(), texture.image->get_data());

	// Setup buffer copy regions for each mip level
	std::vector<VkBufferImageCopy> buffer_copy_regions;

	auto       &mipmaps = texture.image->get_mipmaps();
	const auto &layers  = texture.image->get_layers();

	auto &offsets = texture.image->get_offsets();

	for (uint32_t layer = 0; layer < layers; layer++)
	{
		for (size_t i = 0; i < mipmaps.size(); i++)
		{
			VkBufferImageCopy buffer_copy_region               = {};
			buffer_copy_region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
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

	VkImageSubresourceRange subresource_range = {};
	subresource_range.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource_range.baseMipLevel            = 0;
	subresource_range.levelCount              = vkb::to_u32(mipmaps.size());
	subresource_range.layerCount              = layers;

	// Image barrier for optimal image (target)
	// Optimal image will be used as destination for the copy
	vkb::image_layout_transition(command_buffer,
	                             texture.image->get_vk_image().get_handle(),
	                             VK_IMAGE_LAYOUT_UNDEFINED,
	                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	                             subresource_range);

	// Copy mip levels from staging buffer
	vkCmdCopyBufferToImage(
	    command_buffer,
	    stage_buffer.get_handle(),
	    texture.image->get_vk_image().get_handle(),
	    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	    static_cast<uint32_t>(buffer_copy_regions.size()),
	    buffer_copy_regions.data());

	// Change texture image layout to shader read after all mip levels have been copied
	vkb::image_layout_transition(command_buffer,
	                             texture.image->get_vk_image().get_handle(),
	                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	                             subresource_range);

	get_device().flush_command_buffer(command_buffer, queue.get_handle());

	// Calculate valid filter and mipmap modes
	VkFilter            filter      = VK_FILTER_LINEAR;
	VkSamplerMipmapMode mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	vkb::make_filters_valid(get_device().get_gpu().get_handle(), texture.image->get_format(), &filter, &mipmap_mode);

	// Create a defaultsampler
	VkSamplerCreateInfo sampler_create_info = {};
	sampler_create_info.sType               = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_create_info.magFilter           = filter;
	sampler_create_info.minFilter           = filter;
	sampler_create_info.mipmapMode          = mipmap_mode;
	sampler_create_info.addressModeU        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_create_info.addressModeV        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_create_info.addressModeW        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_create_info.mipLodBias          = 0.0f;
	sampler_create_info.compareOp           = VK_COMPARE_OP_NEVER;
	sampler_create_info.minLod              = 0.0f;
	// Max level-of-detail should match mip level count
	sampler_create_info.maxLod = static_cast<float>(mipmaps.size());
	// Only enable anisotropic filtering if enabled on the devicec
	sampler_create_info.maxAnisotropy    = get_device().get_gpu().get_features().samplerAnisotropy ? get_device().get_gpu().get_properties().limits.maxSamplerAnisotropy : 1.0f;
	sampler_create_info.anisotropyEnable = get_device().get_gpu().get_features().samplerAnisotropy;
	sampler_create_info.borderColor      = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	VK_CHECK(vkCreateSampler(get_device().get_handle(), &sampler_create_info, nullptr, &texture.sampler));

	return texture;
}

Texture ApiVulkanSample::load_texture_cubemap(const std::string &file, vkb::sg::Image::ContentType content_type)
{
	Texture texture{};

	texture.image = vkb::sg::Image::load(file, file, content_type);
	texture.image->create_vk_image(get_device(), VK_IMAGE_VIEW_TYPE_CUBE, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);

	const auto &queue = get_device().get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);

	VkCommandBuffer command_buffer = get_device().create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	vkb::core::BufferC stage_buffer = vkb::core::BufferC::create_staging_buffer(get_device(), texture.image->get_data());

	// Setup buffer copy regions for each mip level
	std::vector<VkBufferImageCopy> buffer_copy_regions;

	auto       &mipmaps = texture.image->get_mipmaps();
	const auto &layers  = texture.image->get_layers();

	auto &offsets = texture.image->get_offsets();

	for (uint32_t layer = 0; layer < layers; layer++)
	{
		for (size_t i = 0; i < mipmaps.size(); i++)
		{
			VkBufferImageCopy buffer_copy_region               = {};
			buffer_copy_region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
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

	VkImageSubresourceRange subresource_range = {};
	subresource_range.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource_range.baseMipLevel            = 0;
	subresource_range.levelCount              = vkb::to_u32(mipmaps.size());
	subresource_range.layerCount              = layers;

	// Image barrier for optimal image (target)
	// Optimal image will be used as destination for the copy
	vkb::image_layout_transition(command_buffer,
	                             texture.image->get_vk_image().get_handle(),
	                             VK_IMAGE_LAYOUT_UNDEFINED,
	                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	                             subresource_range);

	// Copy mip levels from staging buffer
	vkCmdCopyBufferToImage(
	    command_buffer,
	    stage_buffer.get_handle(),
	    texture.image->get_vk_image().get_handle(),
	    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	    static_cast<uint32_t>(buffer_copy_regions.size()),
	    buffer_copy_regions.data());

	// Change texture image layout to shader read after all mip levels have been copied
	vkb::image_layout_transition(command_buffer,
	                             texture.image->get_vk_image().get_handle(),
	                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	                             subresource_range);

	get_device().flush_command_buffer(command_buffer, queue.get_handle());

	// Calculate valid filter and mipmap modes
	VkFilter            filter      = VK_FILTER_LINEAR;
	VkSamplerMipmapMode mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	vkb::make_filters_valid(get_device().get_gpu().get_handle(), texture.image->get_format(), &filter, &mipmap_mode);

	// Create a defaultsampler
	VkSamplerCreateInfo sampler_create_info = {};
	sampler_create_info.sType               = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_create_info.magFilter           = filter;
	sampler_create_info.minFilter           = filter;
	sampler_create_info.mipmapMode          = mipmap_mode;
	sampler_create_info.addressModeU        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_create_info.addressModeV        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_create_info.addressModeW        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_create_info.mipLodBias          = 0.0f;
	sampler_create_info.compareOp           = VK_COMPARE_OP_NEVER;
	sampler_create_info.minLod              = 0.0f;
	// Max level-of-detail should match mip level count
	sampler_create_info.maxLod = static_cast<float>(mipmaps.size());
	// Only enable anisotropic filtering if enabled on the devicec
	sampler_create_info.maxAnisotropy    = get_device().get_gpu().get_features().samplerAnisotropy ? get_device().get_gpu().get_properties().limits.maxSamplerAnisotropy : 1.0f;
	sampler_create_info.anisotropyEnable = get_device().get_gpu().get_features().samplerAnisotropy;
	sampler_create_info.borderColor      = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	VK_CHECK(vkCreateSampler(get_device().get_handle(), &sampler_create_info, nullptr, &texture.sampler));

	return texture;
}

std::unique_ptr<vkb::sg::SubMesh> ApiVulkanSample::load_model(const std::string &file, uint32_t index, bool storage_buffer, VkBufferUsageFlags additional_buffer_usage_flags)
{
	vkb::GLTFLoader loader{get_device()};

	std::unique_ptr<vkb::sg::SubMesh> model = loader.read_model_from_file(file, index, storage_buffer, additional_buffer_usage_flags);

	if (!model)
	{
		LOGE("Cannot load model from file: {}", file.c_str());
		throw std::runtime_error("Cannot load model from: " + file);
	}

	return model;
}

void ApiVulkanSample::draw_model(std::unique_ptr<vkb::sg::SubMesh> &model, VkCommandBuffer command_buffer, uint32_t instance_count)
{
	VkDeviceSize offsets[1] = {0};

	const auto &vertex_buffer = model->vertex_buffers.at("vertex_buffer");
	auto       &index_buffer  = model->index_buffer;

	vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffer.get(), offsets);
	vkCmdBindIndexBuffer(command_buffer, index_buffer->get_handle(), 0, model->index_type);
	vkCmdDrawIndexed(command_buffer, model->vertex_indices, instance_count, 0, 0, 0);
}

void ApiVulkanSample::with_command_buffer(const std::function<void(VkCommandBuffer command_buffer)> &f, VkSemaphore signalSemaphore)
{
	VkCommandBuffer command_buffer = get_device().create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	f(command_buffer);
	get_device().flush_command_buffer(command_buffer, queue, true, signalSemaphore);
}

void ApiVulkanSample::with_vkb_command_buffer(const std::function<void(vkb::CommandBuffer &command_buffer)> &f)
{
	auto &cmd = get_device().request_command_buffer();
	cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, VK_NULL_HANDLE);
	f(cmd);
	cmd.end();
	auto &queue = get_device().get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);
	queue.submit(cmd, get_device().request_fence());
	get_device().get_fence_pool().wait();
}
