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

#include <hpp_vulkan_sample.h>

#include <common/hpp_utils.h>
#include <hpp_gltf_loader.h>
#include <hpp_gui.h>
#include <rendering/hpp_render_context.h>
#include <scene_graph/components/camera.h>
#include <scene_graph/scripts/animation.h>

namespace vkb
{
HPPVulkanSample::~HPPVulkanSample()
{
	if (device)
	{
		device->get_handle().waitIdle();
	}

	scene.reset();
	stats.reset();
	gui.reset();
	render_context.reset();
	device.reset();

	if (surface)
	{
		instance->get_handle().destroySurfaceKHR(surface);
	}

	instance.reset();
}

void HPPVulkanSample::set_render_pipeline(vkb::rendering::HPPRenderPipeline &&rp)
{
	render_pipeline = std::make_unique<vkb::rendering::HPPRenderPipeline>(std::move(rp));
}

vkb::rendering::HPPRenderPipeline const &HPPVulkanSample::get_render_pipeline() const
{
	assert(render_pipeline && "Render pipeline was not created");
	return *render_pipeline;
}

bool HPPVulkanSample::prepare(const vkb::ApplicationOptions &options)
{
	if (!vkb::Application::prepare(options))
	{
		return false;
	}

	LOGI("Initializing Vulkan sample");

	// initialize function pointers
	static vk::DynamicLoader dl;
	VULKAN_HPP_DEFAULT_DISPATCHER.init(dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));

	// for non-vulkan.hpp stuff, we need to initialize volk as well !!
	VkResult result = volkInitialize();
	if (result)
	{
		throw VulkanException(result, "Failed to initialize volk.");
	}

	bool headless = window->get_window_mode() == Window::Mode::Headless;

	// Creating the vulkan instance
	for (const char *extension_name : window->get_required_surface_extensions())
	{
		add_instance_extension(extension_name);
	}

	std::unique_ptr<vkb::core::HPPDebugUtils> debug_utils;
#ifdef VKB_VULKAN_DEBUG
	{
		std::vector<vk::ExtensionProperties> available_instance_extensions = vk::enumerateInstanceExtensionProperties();
		auto                                 debugExtensionIt              = std::find_if(available_instance_extensions.begin(),
		                                                                                  available_instance_extensions.end(),
		                                                                                  [](vk::ExtensionProperties const &ep) { return strcmp(ep.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0; });
		if (debugExtensionIt != available_instance_extensions.end())
		{
			LOGI("Vulkan debug utils enabled ({})", VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

			debug_utils = std::make_unique<vkb::core::HPPDebugUtilsExtDebugUtils>();
			add_instance_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
	}
#endif

	instance = std::make_unique<vkb::core::HPPInstance>(get_name(), get_instance_extensions(), get_validation_layers(), headless, api_version);

	// Getting a valid vulkan surface from the platform
	surface = static_cast<vk::SurfaceKHR>(window->create_surface(reinterpret_cast<vkb::Instance &>(*instance)));
	if (!surface)
	{
		throw std::runtime_error("Failed to create window surface.");
	}

	auto &gpu = instance->get_suitable_gpu(surface);
	gpu.set_high_priority_graphics_queue_enable(high_priority_graphics_queue);

	// Request to enable ASTC
	if (gpu.get_features().textureCompressionASTC_LDR)
	{
		gpu.get_mutable_requested_features().textureCompressionASTC_LDR = true;
	}

	// Request sample required GPU features
	request_gpu_features(gpu);

	// Creating vulkan device, specifying the swapchain extension always
	if (!headless || instance->is_enabled(VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME))
	{
		add_device_extension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		if (instance_extensions.find(VK_KHR_DISPLAY_EXTENSION_NAME) != instance_extensions.end())
		{
			add_device_extension(VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME, /*optional=*/true);
		}
	}

#ifdef VKB_VULKAN_DEBUG
	if (!debug_utils)
	{
		std::vector<vk::ExtensionProperties> available_device_extensions = gpu.get_handle().enumerateDeviceExtensionProperties();
		auto                                 debugExtensionIt            = std::find_if(available_device_extensions.begin(),
		                                                                                available_device_extensions.end(),
		                                                                                [](vk::ExtensionProperties const &ep) { return strcmp(ep.extensionName, VK_EXT_DEBUG_MARKER_EXTENSION_NAME) == 0; });
		if (debugExtensionIt != available_device_extensions.end())
		{
			LOGI("Vulkan debug utils enabled ({})", VK_EXT_DEBUG_MARKER_EXTENSION_NAME);

			debug_utils = std::make_unique<vkb::core::HPPDebugMarkerExtDebugUtils>();
			add_device_extension(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
		}
	}

	if (!debug_utils)
	{
		LOGW("Vulkan debug utils were requested, but no extension that provides them was found");
	}
#endif

	if (!debug_utils)
	{
		debug_utils = std::make_unique<vkb::core::HPPDummyDebugUtils>();
	}

	device = std::make_unique<vkb::core::HPPDevice>(gpu, surface, std::move(debug_utils), get_device_extensions());

	VULKAN_HPP_DEFAULT_DISPATCHER.init(get_device()->get_handle());

	create_render_context();
	prepare_render_context();

	stats = std::make_unique<vkb::stats::HPPStats>(*render_context);

	// Start the sample in the first GUI configuration
	configuration.reset();

	return true;
}

void HPPVulkanSample::create_render_context()
{
	auto surface_priority_list = std::vector<vk::SurfaceFormatKHR>{{vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear},
	                                                               {vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear}};

	create_render_context(surface_priority_list);
}

void HPPVulkanSample::create_render_context(const std::vector<vk::SurfaceFormatKHR> &surface_priority_list)
{
#ifdef VK_USE_PLATFORM_ANDROID_KHR
	vk::PresentModeKHR              present_mode = (window->get_properties().vsync == Window::Vsync::OFF) ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eFifo;
	std::vector<vk::PresentModeKHR> present_mode_priority_list{vk::PresentModeKHR::eFifo, vk::PresentModeKHR::eMailbox, vk::PresentModeKHR::eImmediate};
#else
	vk::PresentModeKHR              present_mode = (window->get_properties().vsync == Window::Vsync::ON) ? vk::PresentModeKHR::eFifo : vk::PresentModeKHR::eMailbox;
	std::vector<vk::PresentModeKHR> present_mode_priority_list{vk::PresentModeKHR::eMailbox, vk::PresentModeKHR::eFifo, vk::PresentModeKHR::eImmediate};
#endif

	render_context = std::make_unique<vkb::rendering::HPPRenderContext>(*get_device(), surface, *window, present_mode, present_mode_priority_list, surface_priority_list);
}

void HPPVulkanSample::prepare_render_context()
{
	render_context->prepare();
}

void HPPVulkanSample::update_scene(float delta_time)
{
	if (scene)
	{
		// Update scripts
		if (scene->has_component<sg::Script>())
		{
			auto scripts = scene->get_components<sg::Script>();

			for (auto script : scripts)
			{
				script->update(delta_time);
			}
		}

		// Update animations
		if (scene->has_component<sg::Animation>())
		{
			auto animations = scene->get_components<sg::Animation>();

			for (auto animation : animations)
			{
				animation->update(delta_time);
			}
		}
	}
}

void HPPVulkanSample::update_stats(float delta_time)
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

void HPPVulkanSample::update_gui(float delta_time)
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

void HPPVulkanSample::update(float delta_time)
{
	update_scene(delta_time);

	update_gui(delta_time);

	auto &command_buffer = render_context->begin();

	// Collect the performance data for the sample graphs
	update_stats(delta_time);

	command_buffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	stats->begin_sampling(command_buffer);

	draw(command_buffer, render_context->get_active_frame().get_render_target());

	stats->end_sampling(command_buffer);
	command_buffer.end();

	render_context->submit(command_buffer);
}

void HPPVulkanSample::draw(vkb::core::HPPCommandBuffer &command_buffer, vkb::rendering::HPPRenderTarget const &render_target) const
{
	auto &views = render_target.get_views();

	{
		// Image 0 is the swapchain
		vkb::common::HPPImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout      = vk::ImageLayout::eUndefined;
		memory_barrier.new_layout      = vk::ImageLayout::eColorAttachmentOptimal;
		memory_barrier.src_access_mask = {};
		memory_barrier.dst_access_mask = vk::AccessFlagBits::eColorAttachmentWrite;
		memory_barrier.src_stage_mask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		memory_barrier.dst_stage_mask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;

		command_buffer.image_memory_barrier(views[0], memory_barrier);

		// Skip 1 as it is handled later as a depth-stencil attachment
		for (size_t i = 2; i < views.size(); ++i)
		{
			command_buffer.image_memory_barrier(views[i], memory_barrier);
		}
	}

	{
		vkb::common::HPPImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout      = vk::ImageLayout::eUndefined;
		memory_barrier.new_layout      = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		memory_barrier.src_access_mask = {};
		memory_barrier.dst_access_mask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		memory_barrier.src_stage_mask  = vk::PipelineStageFlagBits::eTopOfPipe;
		memory_barrier.dst_stage_mask  = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;

		command_buffer.image_memory_barrier(views[1], memory_barrier);
	}

	draw_renderpass(command_buffer, render_target);

	{
		vkb::common::HPPImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout      = vk::ImageLayout::eColorAttachmentOptimal;
		memory_barrier.new_layout      = vk::ImageLayout::ePresentSrcKHR;
		memory_barrier.src_access_mask = vk::AccessFlagBits::eColorAttachmentWrite;
		memory_barrier.src_stage_mask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		memory_barrier.dst_stage_mask  = vk::PipelineStageFlagBits::eBottomOfPipe;

		command_buffer.image_memory_barrier(views[0], memory_barrier);
	}
}

void HPPVulkanSample::draw_renderpass(vkb::core::HPPCommandBuffer &command_buffer, vkb::rendering::HPPRenderTarget const &render_target) const
{
	set_viewport_and_scissor(command_buffer, render_target.get_extent());

	render(command_buffer);

	if (gui)
	{
		gui->draw(command_buffer);
	}

	command_buffer.get_handle().endRenderPass();
}

void HPPVulkanSample::render(vkb::core::HPPCommandBuffer &command_buffer) const
{
	if (render_pipeline)
	{
		render_pipeline->draw(command_buffer, render_context->get_active_frame().get_render_target());
	}
}

bool HPPVulkanSample::resize(uint32_t width, uint32_t height)
{
	if (!vkb::Application::resize(width, height))
	{
		return false;
	}

	if (gui)
	{
		gui->resize(width, height);
	}

	if (scene && scene->has_component<sg::Script>())
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
	return true;
}

void HPPVulkanSample::input_event(const InputEvent &input_event)
{
	vkb::Application::input_event(input_event);

	bool gui_captures_event = false;

	if (gui)
	{
		gui_captures_event = gui->input_event(input_event);
	}

	if (!gui_captures_event)
	{
		if (scene && scene->has_component<sg::Script>())
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
		if (key_event.get_action() == KeyAction::Down &&
		    (key_event.get_code() == KeyCode::PrintScreen || key_event.get_code() == KeyCode::F12))
		{
			vkb::common::screenshot(*render_context, "screenshot-" + get_name());
		}
	}
}

Drawer *HPPVulkanSample::get_drawer()
{
	if (nullptr == gui)
		return nullptr;
	return &gui->get_drawer();
}

void HPPVulkanSample::finish()
{
	vkb::Application::finish();

	if (device)
	{
		device->get_handle().waitIdle();
	}
}

vkb::core::HPPInstance const &HPPVulkanSample::get_instance() const
{
	return *instance;
}

std::unique_ptr<vkb::core::HPPDevice> const &HPPVulkanSample::get_device() const
{
	return device;
}

Configuration &HPPVulkanSample::get_configuration()
{
	return configuration;
}

void HPPVulkanSample::draw_gui()
{
}

void HPPVulkanSample::update_debug_window()
{
	auto        driver_version     = device->get_gpu().get_driver_version();
	std::string driver_version_str = fmt::format("major: {} minor: {} patch: {}", driver_version.major, driver_version.minor, driver_version.patch);

	get_debug_info().insert<field::Static, std::string>("driver_version", driver_version_str);
	get_debug_info().insert<field::Static, std::string>("resolution",
	                                                    to_string(static_cast<VkExtent2D const &>(render_context->get_swapchain().get_extent())));
	get_debug_info().insert<field::Static, std::string>("surface_format",
	                                                    to_string(render_context->get_swapchain().get_format()) + " (" +
	                                                        to_string(vkb::common::get_bits_per_pixel(render_context->get_swapchain().get_format())) +
	                                                        "bpp)");

	if (scene != nullptr)
	{
		get_debug_info().insert<field::Static, uint32_t>("mesh_count", to_u32(scene->get_components<sg::SubMesh>().size()));
		get_debug_info().insert<field::Static, uint32_t>("texture_count", to_u32(scene->get_components<sg::Texture>().size()));

		if (auto camera = scene->get_components<vkb::sg::Camera>()[0])
		{
			if (auto camera_node = camera->get_node())
			{
				const glm::vec3 &pos = camera_node->get_transform().get_translation();
				get_debug_info().insert<field::Vector, float>("camera_pos", pos.x, pos.y, pos.z);
			}
		}
	}
}

const std::map<ShaderSourceLanguage, std::vector<std::pair<vk::ShaderStageFlagBits, std::string>>> &HPPVulkanSample::get_available_shaders() const
{
	return reinterpret_cast<std::map<ShaderSourceLanguage, std::vector<std::pair<vk::ShaderStageFlagBits, std::string>>> const &>(Application::get_available_shaders());
}

void HPPVulkanSample::store_shaders(const vkb::ShaderSourceLanguage &shader_language, const std::vector<std::pair<vk::ShaderStageFlagBits, std::string>> &list_of_shaders)
{
	Application::store_shaders(shader_language, reinterpret_cast<std::vector<std::pair<VkShaderStageFlagBits, std::string>> const &>(list_of_shaders));
}

void HPPVulkanSample::set_viewport_and_scissor(vkb::core::HPPCommandBuffer const &command_buffer, const vk::Extent2D &extent)
{
	command_buffer.get_handle().setViewport(0, {{0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f}});
	command_buffer.get_handle().setScissor(0, vk::Rect2D({}, extent));
}

void HPPVulkanSample::load_scene(const std::string &path)
{
	vkb::HPPGLTFLoader loader(*device);

	scene = loader.read_scene_from_file(path);

	if (!scene)
	{
		LOGE("Cannot load scene: {}", path.c_str());
		throw std::runtime_error("Cannot load scene: " + path);
	}
}

vk::SurfaceKHR HPPVulkanSample::get_surface() const
{
	return surface;
}

vkb::rendering::HPPRenderContext &HPPVulkanSample::get_render_context()
{
	assert(render_context && "Render context is not valid");
	return *render_context;
}

std::vector<const char *> const &HPPVulkanSample::get_validation_layers() const
{
	static std::vector<const char *> validation_layers;
	return validation_layers;
}

std::unordered_map<const char *, bool> const &HPPVulkanSample::get_instance_extensions() const
{
	return instance_extensions;
}

std::unordered_map<const char *, bool> const &HPPVulkanSample::get_device_extensions() const
{
	return device_extensions;
}

void HPPVulkanSample::add_device_extension(const char *extension, bool optional)
{
	device_extensions[extension] = optional;
}

void HPPVulkanSample::add_instance_extension(const char *extension, bool optional)
{
	instance_extensions[extension] = optional;
}

void HPPVulkanSample::set_api_version(uint32_t requested_api_version)
{
	api_version = requested_api_version;
}

void HPPVulkanSample::request_gpu_features(vkb::core::HPPPhysicalDevice &gpu)
{
	// To be overridden by sample
}

sg::Scene &HPPVulkanSample::get_scene()
{
	assert(scene && "Scene not loaded");
	return *scene;
}

bool HPPVulkanSample::has_scene() const
{
	return scene != nullptr;
}

}        // namespace vkb
