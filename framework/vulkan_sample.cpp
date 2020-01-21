/* Copyright (c) 2019-2020, Arm Limited and Contributors
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

#include "vulkan_sample.h"

#include "common/error.h"

VKBP_DISABLE_WARNINGS()
#include "common/glm_common.h"
#include <imgui.h>
VKBP_ENABLE_WARNINGS()

#include "api_vulkan_sample.h"
#include "common/helpers.h"
#include "common/logging.h"
#include "common/strings.h"
#include "common/utils.h"
#include "common/vk_common.h"
#include "gltf_loader.h"
#include "platform/platform.h"
#include "platform/window.h"
#include "scene_graph/components/camera.h"
#include "scene_graph/script.h"
#include "scene_graph/scripts/free_camera.h"

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
#	include "platform/android/android_platform.h"
#endif

namespace vkb
{
VulkanSample::~VulkanSample()
{
	if (device)
	{
		device->wait_idle();
	}

	scene.reset();

	stats.reset();
	gui.reset();
	render_context.reset();
	device.reset();

	if (surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(instance->get_handle(), surface, nullptr);
	}

	instance.reset();
}

void VulkanSample::set_render_pipeline(RenderPipeline &&rp)
{
	render_pipeline = std::make_unique<RenderPipeline>(std::move(rp));
}

RenderPipeline &VulkanSample::get_render_pipeline()
{
	assert(render_pipeline && "Render pipeline was not created");
	return *render_pipeline;
}

bool VulkanSample::prepare(Platform &platform)
{
	if (!Application::prepare(platform))
	{
		return false;
	}

	LOGI("Initializing Vulkan sample");

	// Creating the vulkan instance
	add_instance_extension(platform.get_surface_extension());
	instance = std::make_unique<Instance>(get_name(), get_instance_extensions(), get_validation_layers(), is_headless());

	// Getting a valid vulkan surface from the platform
	surface = platform.get_window().create_surface(*instance);

	auto physical_device = instance->get_gpu();

	// Get supported features from the physical device, and requested features from the sample
	vkGetPhysicalDeviceFeatures(physical_device, &supported_device_features);
	get_device_features();

	// Creating vulkan device, specifying the swapchain extension always
	if (!is_headless() || instance->is_enabled(VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME))
	{
		add_device_extension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	}

	device = std::make_unique<vkb::Device>(physical_device, surface, get_device_extensions(), requested_device_features);

	// Preparing render context for rendering
	render_context = std::make_unique<vkb::RenderContext>(*device, surface, platform.get_window().get_width(), platform.get_window().get_height());
	render_context->set_present_mode_priority({VK_PRESENT_MODE_FIFO_KHR,
	                                           VK_PRESENT_MODE_MAILBOX_KHR});

	render_context->set_surface_format_priority({{VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
	                                             {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
	                                             {VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
	                                             {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}});

	prepare_render_context();

	return true;
}

void VulkanSample::prepare_render_context()
{
	render_context->prepare();
}

void VulkanSample::update_scene(float delta_time)
{
	if (scene)
	{
		//Update scripts
		if (scene->has_component<sg::Script>())
		{
			auto scripts = scene->get_components<sg::Script>();

			for (auto script : scripts)
			{
				script->update(delta_time);
			}
		}
	}
}

void VulkanSample::update_stats(float delta_time)
{
	if (stats)
	{
		stats->update(delta_time);

		static float stats_view_count = 0.0f;
		stats_view_count += delta_time;

		// Reset every STATS_VIEW_RESET_TIME seconds
		if (stats_view_count > STATS_VIEW_RESET_TIME)
		{
			reset_stats_view();
			stats_view_count = 0.0f;
		}
	}
}

void VulkanSample::update_gui(float delta_time)
{
	if (gui)
	{
		if (gui->is_debug_view_active())
		{
			update_debug_window();
		}

		gui->new_frame();

		gui->show_top_window(get_name(), stats.get(), &get_debug_info());

		// Samples can override this
		draw_gui();

		gui->update(delta_time);
	}
}

void VulkanSample::update(float delta_time)
{
	update_scene(delta_time);

	update_stats(delta_time);

	update_gui(delta_time);

	auto &command_buffer = render_context->begin();

	command_buffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	draw(command_buffer, render_context->get_active_frame().get_render_target());

	command_buffer.end();

	render_context->submit(command_buffer);
}

void VulkanSample::draw(CommandBuffer &command_buffer, RenderTarget &render_target)
{
	auto &views = render_target.get_views();

	{
		// Image 0 is the swapchain
		ImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout      = VK_IMAGE_LAYOUT_UNDEFINED;
		memory_barrier.new_layout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		memory_barrier.src_access_mask = 0;
		memory_barrier.dst_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		command_buffer.image_memory_barrier(views.at(0), memory_barrier);

		// Skip 1 as it is handled later as a depth-stencil attachment
		for (size_t i = 2; i < views.size(); ++i)
		{
			command_buffer.image_memory_barrier(views.at(i), memory_barrier);
		}
	}

	{
		ImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout      = VK_IMAGE_LAYOUT_UNDEFINED;
		memory_barrier.new_layout      = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		memory_barrier.src_access_mask = 0;
		memory_barrier.dst_access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

		command_buffer.image_memory_barrier(views.at(1), memory_barrier);
	}

	draw_renderpass(command_buffer, render_target);

	{
		ImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		memory_barrier.new_layout      = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		memory_barrier.src_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

		command_buffer.image_memory_barrier(views.at(0), memory_barrier);
	}
}

void VulkanSample::draw_renderpass(CommandBuffer &command_buffer, RenderTarget &render_target)
{
	auto &extent = render_target.get_extent();

	VkViewport viewport{};
	viewport.width    = static_cast<float>(extent.width);
	viewport.height   = static_cast<float>(extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	command_buffer.set_viewport(0, {viewport});

	VkRect2D scissor{};
	scissor.extent = extent;
	command_buffer.set_scissor(0, {scissor});

	render(command_buffer);

	if (gui)
	{
		gui->draw(command_buffer);
	}

	command_buffer.end_render_pass();
}

void VulkanSample::render(CommandBuffer &command_buffer)
{
	if (render_pipeline)
	{
		render_pipeline->draw(command_buffer, render_context->get_active_frame().get_render_target());
	}
}

void VulkanSample::resize(uint32_t width, uint32_t height)
{
	Application::resize(width, height);

	if (gui)
	{
		gui->resize(width, height);
	}

	if (scene->has_component<sg::Script>())
	{
		auto scripts = scene->get_components<sg::Script>();

		for (auto script : scripts)
		{
			script->resize(width, height);
		}
	}

	if (stats)
	{
		stats->resize(width);
	}
}

void VulkanSample::input_event(const InputEvent &input_event)
{
	Application::input_event(input_event);

	bool gui_captures_event = false;

	if (gui)
	{
		gui_captures_event = gui->input_event(input_event);
	}

	if (!gui_captures_event)
	{
		if (scene->has_component<sg::Script>())
		{
			auto scripts = scene->get_components<sg::Script>();

			for (auto script : scripts)
			{
				script->input_event(input_event);
			}
		}
	}

	if (input_event.get_source() == EventSource::Keyboard)
	{
		const auto &key_event = static_cast<const KeyInputEvent &>(input_event);
		if (key_event.get_action() == KeyAction::Down && key_event.get_code() == KeyCode::PrintScreen)
		{
			screenshot(*render_context, "screenshot-" + get_name());
		}

		if (key_event.get_code() == KeyCode::F6 && key_event.get_action() == KeyAction::Down)
		{
			if (!graphs::generate_all(get_render_context(), *scene.get()))
			{
				LOGE("Failed to save Graphs");
			}
		}
	}
}

void VulkanSample::finish()
{
	Application::finish();

	if (device)
	{
		device->wait_idle();
	}
}

Device &VulkanSample::get_device()
{
	return *device;
}

Configuration &VulkanSample::get_configuration()
{
	return configuration;
}

void VulkanSample::draw_gui()
{
}

void VulkanSample::update_debug_window()
{
	auto        driver_version     = device->get_driver_version();
	std::string driver_version_str = fmt::format("major: {} minor: {} patch: {}", driver_version.major, driver_version.minor, driver_version.patch);
	get_debug_info().insert<field::Static, std::string>("driver_version", driver_version_str);

	get_debug_info().insert<field::Static, std::string>("resolution",
	                                                    to_string(render_context->get_swapchain().get_extent()));

	get_debug_info().insert<field::Static, std::string>("surface_format",
	                                                    to_string(render_context->get_swapchain().get_format()) + " (" +
	                                                        to_string(get_bits_per_pixel(render_context->get_swapchain().get_format())) + "bbp)");

	get_debug_info().insert<field::Static, uint32_t>("mesh_count", to_u32(scene->get_components<sg::SubMesh>().size()));

	get_debug_info().insert<field::Static, uint32_t>("texture_count", to_u32(scene->get_components<sg::Texture>().size()));

	if (auto camera = scene->get_components<vkb::sg::Camera>().at(0))
	{
		if (auto camera_node = camera->get_node())
		{
			const glm::vec3 &pos = camera_node->get_transform().get_translation();
			get_debug_info().insert<field::Vector, float>("camera_pos", pos.x, pos.y, pos.z);
		}
	}
}

void VulkanSample::load_scene(const std::string &path)
{
	GLTFLoader loader{*device};

	scene = loader.read_scene_from_file(path);

	if (!scene)
	{
		LOGE("Cannot load scene: {}", path.c_str());
		throw std::runtime_error("Cannot load scene: " + path);
	}
}

VkSurfaceKHR VulkanSample::get_surface()
{
	return surface;
}

RenderContext &VulkanSample::get_render_context()
{
	assert(render_context && "Render context is not valid");
	return *render_context;
}

const std::vector<const char *> VulkanSample::get_validation_layers()
{
	return {};
}

const std::unordered_map<const char *, bool> VulkanSample::get_instance_extensions()
{
	return instance_extensions;
}

const std::unordered_map<const char *, bool> VulkanSample::get_device_extensions()
{
	return device_extensions;
}

void VulkanSample::add_device_extension(const char *extension, bool optional)
{
	device_extensions[extension] = optional;
}

void VulkanSample::add_instance_extension(const char *extension, bool optional)
{
	instance_extensions[extension] = optional;
}

void VulkanSample::get_device_features()
{
	// Can be overriden in derived class
}

sg::Scene &VulkanSample::get_scene()
{
	assert(scene && "Scene not loaded");
	return *scene;
}
}        // namespace vkb
