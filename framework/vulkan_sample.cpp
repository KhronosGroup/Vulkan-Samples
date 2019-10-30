/* Copyright (c) 2019, Arm Limited and Contributors
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
#include <glm/glm.hpp>
#include <imgui.h>
VKBP_ENABLE_WARNINGS()

#include "api_vulkan_sample.h"
#include "common/helpers.h"
#include "common/logging.h"
#include "common/vk_common.h"
#include "gltf_loader.h"
#include "platform/platform.h"
#include "scene_graph/components/camera.h"
#include "scene_graph/script.h"
#include "scene_graph/scripts/free_camera.h"
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
#	include "platform/android/android_platform.h"
#endif

namespace vkb
{
namespace
{
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT /*type*/,
                                                     uint64_t /*object*/, size_t /*location*/, int32_t /*message_code*/,
                                                     const char *layer_prefix, const char *message, void * /*user_data*/)
{
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
	{
		LOGE("{}: {}", layer_prefix, message);
	}
	else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
	{
		LOGW("{}: {}", layer_prefix, message);
	}
	else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
	{
		LOGW("{}: {}", layer_prefix, message);
	}
	else
	{
		LOGI("{}: {}", layer_prefix, message);
	}
	return VK_FALSE;
}
#endif
bool validate_extensions(const std::vector<const char *> &         required,
                         const std::vector<VkExtensionProperties> &available)
{
	for (auto extension : required)
	{
		bool found = false;
		for (auto &available_extension : available)
		{
			if (strcmp(available_extension.extensionName, extension) == 0)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			return false;
		}
	}

	return true;
}

bool validate_layers(const std::vector<const char *> &     required,
                     const std::vector<VkLayerProperties> &available)
{
	for (auto extension : required)
	{
		bool found = false;
		for (auto &available_extension : available)
		{
			if (strcmp(available_extension.layerName, extension) == 0)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			return false;
		}
	}

	return true;
}
}        // namespace

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
		vkDestroySurfaceKHR(instance, surface, nullptr);
	}

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	vkDestroyDebugReportCallbackEXT(instance, debug_report_callback, nullptr);
#endif

	if (instance != VK_NULL_HANDLE)
	{
		vkDestroyInstance(instance, nullptr);
	}
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

	get_debug_info().insert<field::MinMax, float>("fps", fps);
	get_debug_info().insert<field::MinMax, float>("frame_time", frame_time);

	LOGI("Initializing context");

	std::vector<const char *> requested_instance_extensions = get_instance_extensions();
	requested_instance_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	instance = create_instance(requested_instance_extensions, get_validation_layers());

	surface = platform.create_surface(instance);

	uint32_t physical_device_count{0};
	VK_CHECK(vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr));

	if (physical_device_count < 1)
	{
		LOGE("Failed to enumerate Vulkan physical device.");
		return false;
	}

	gpus.resize(physical_device_count);

	VK_CHECK(vkEnumeratePhysicalDevices(instance, &physical_device_count, gpus.data()));

	auto physical_device = get_gpu();

	// Get supported features from the physical device, and requested features from the sample
	vkGetPhysicalDeviceFeatures(physical_device, &supported_device_features);
	get_device_features();

	// Creating vulkan device, specifying the swapchain extension always
	std::vector<const char *> requested_device_extensions = get_device_extensions();
	requested_device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	device = std::make_unique<vkb::Device>(physical_device, surface, requested_device_extensions, requested_device_features);

	// Preparing render context for rendering
	render_context = std::make_unique<vkb::RenderContext>(*device, surface);
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

void VulkanSample::record_scene_rendering_commands(CommandBuffer &command_buffer, RenderTarget &render_target)
{
	auto &views = render_target.get_views();

	{
		ImageMemoryBarrier memory_barrier{};
		memory_barrier.new_layout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		memory_barrier.src_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
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
		memory_barrier.new_layout      = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		memory_barrier.src_access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		memory_barrier.dst_access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

		command_buffer.image_memory_barrier(views.at(1), memory_barrier);
	}

	draw_swapchain_renderpass(command_buffer, render_target);

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

void VulkanSample::update(float delta_time)
{
	update_scene(delta_time);

	update_stats(delta_time);

	update_gui(delta_time);

	auto acquired_semaphore = render_context->begin_frame();

	if (acquired_semaphore == VK_NULL_HANDLE)
	{
		return;
	}

	const auto &queue = device->get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);

	auto &render_target = render_context->get_active_frame().get_render_target();

	auto &command_buffer = render_context->request_frame_command_buffer(queue);

	command_buffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	record_scene_rendering_commands(command_buffer, render_target);

	command_buffer.end();

	auto render_semaphore = render_context->submit(queue, command_buffer, acquired_semaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

	render_context->end_frame(render_semaphore);
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

VkPhysicalDevice VulkanSample::get_gpu()
{
	// Find a discrete GPU
	for (auto gpu : gpus)
	{
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(gpu, &properties);
		if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			return gpu;
		}
	}

	// Otherwise just pick the first one
	return gpus.at(0);
}

VkSurfaceKHR VulkanSample::get_surface()
{
	return surface;
}

Configuration &VulkanSample::get_configuration()
{
	return configuration;
}

void VulkanSample::render(CommandBuffer &command_buffer)
{
	if (render_pipeline)
	{
		render_pipeline->draw(command_buffer, render_context->get_active_frame().get_render_target());
	}
}

void VulkanSample::draw_gui()
{
}

void VulkanSample::update_debug_window()
{
	get_debug_info().insert<field::Static, std::string>("resolution",
	                                                    to_string(render_context->get_swapchain().get_extent().width) + "x" +
	                                                        to_string(render_context->get_swapchain().get_extent().height));

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

sg::Node &VulkanSample::add_free_camera(const std::string &node_name)
{
	auto camera_node = scene->find_node(node_name);

	if (!camera_node)
	{
		LOGW("Camera node `{}` not found. Looking for `default_camera` node.", node_name.c_str());

		camera_node = scene->find_node("default_camera");
	}

	if (!camera_node)
	{
		throw std::runtime_error("Camera node with name `" + node_name + "` not found.");
	}

	if (!camera_node->has_component<sg::Camera>())
	{
		throw std::runtime_error("No camera component found for `" + node_name + "` node.");
	}

	auto free_camera_script = std::make_unique<sg::FreeCamera>(*camera_node);

	free_camera_script->resize(render_context->get_surface_extent().width, render_context->get_surface_extent().height);

	scene->add_component(std::move(free_camera_script), *camera_node);

	return *camera_node;
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

Device &VulkanSample::get_device()
{
	assert(render_context && "Device is not valid");
	return *device;
}

RenderContext &VulkanSample::get_render_context()
{
	assert(render_context && "Render context is not valid");
	return *render_context;
}

VkInstance VulkanSample::create_instance(const std::vector<const char *> &required_instance_extensions,
                                         const std::vector<const char *> &required_instance_layers)
{
	VkResult result = volkInitialize();
	if (result)
	{
		throw VulkanException(result, "Failed to initialize volk.");
	}

	uint32_t instance_extension_count;
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, nullptr));

	std::vector<VkExtensionProperties> instance_extensions(instance_extension_count);
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, instance_extensions.data()));

	std::vector<const char *> active_instance_extensions(required_instance_extensions);

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	active_instance_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
	active_instance_extensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
	active_instance_extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
	active_instance_extensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
	active_instance_extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_DISPLAY_KHR)
	active_instance_extensions.push_back(VK_KHR_DISPLAY_EXTENSION_NAME);
#endif

	if (!validate_extensions(active_instance_extensions, instance_extensions))
	{
		throw std::runtime_error("Required instance extensions are missing.");
	}

	uint32_t instance_layer_count;
	VK_CHECK(vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr));

	std::vector<VkLayerProperties> instance_layers(instance_layer_count);
	VK_CHECK(vkEnumerateInstanceLayerProperties(&instance_layer_count, instance_layers.data()));

	std::vector<const char *> active_instance_layers(required_instance_layers);

#ifdef VKB_VALIDATION_LAYERS
	active_instance_layers.push_back("VK_LAYER_KHRONOS_validation");
#endif

	if (!validate_layers(active_instance_layers, instance_layers))
	{
		throw std::runtime_error("Required instance layers are missing.");
	}

	VkApplicationInfo app_info{VK_STRUCTURE_TYPE_APPLICATION_INFO};

	app_info.pApplicationName   = get_name().c_str();
	app_info.applicationVersion = 0;
	app_info.pEngineName        = "Vulkan Samples";
	app_info.engineVersion      = 0;
	app_info.apiVersion         = VK_MAKE_VERSION(1, 0, 0);

	VkInstanceCreateInfo instance_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};

	instance_info.pApplicationInfo = &app_info;

	instance_info.enabledExtensionCount   = to_u32(active_instance_extensions.size());
	instance_info.ppEnabledExtensionNames = active_instance_extensions.data();

	instance_info.enabledLayerCount   = to_u32(active_instance_layers.size());
	instance_info.ppEnabledLayerNames = active_instance_layers.data();

	// Create the Vulkan instance

	VkInstance new_instance;
	result = vkCreateInstance(&instance_info, nullptr, &new_instance);
	if (result != VK_SUCCESS)
	{
		throw VulkanException(result, "Could not create Vulkan instance");
	}

	volkLoadInstance(new_instance);

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	VkDebugReportCallbackCreateInfoEXT info = {VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT};

	info.flags       = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	info.pfnCallback = debug_callback;

	result = vkCreateDebugReportCallbackEXT(new_instance, &info, nullptr, &debug_report_callback);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Could not create debug callback.");
	}
#endif

	return new_instance;
}

const std::vector<const char *> VulkanSample::get_validation_layers()
{
	return {};
}

const std::vector<const char *> VulkanSample::get_instance_extensions()
{
	return {};
}

const std::vector<const char *> VulkanSample::get_device_extensions()
{
	return {};
}

void VulkanSample::get_device_features()
{
	// Can be overriden in derived class
}

void VulkanSample::draw_swapchain_renderpass(CommandBuffer &command_buffer, RenderTarget &render_target)
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
}        // namespace vkb
