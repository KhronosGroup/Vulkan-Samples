/* Copyright (c) 2019-2026, Arm Limited and Contributors
 * Copyright (c) 2021-2026, NVIDIA CORPORATION. All rights reserved.
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

#pragma once

#include "common/hpp_utils.h"
#include "core/debug.h"
#include "core/hpp_debug.h"
#include "gui.h"
#include "hpp_gltf_loader.h"
#include "platform/application.h"
#include "platform/window.h"
#include "rendering/hpp_render_pipeline.h"
#include "stats/hpp_stats.h"

#if defined(PLATFORM__MACOS)
#	include <TargetConditionals.h>
#endif

namespace vkb
{
/**
 * @mainpage Overview of the framework
 *
 * @section initialization Initialization
 *
 * @subsection platform_init Platform initialization
 * The lifecycle of a Vulkan sample starts by instantiating the correct Platform
 * (e.g. WindowsPlatform) and then calling initialize() on it, which sets up
 * the windowing system and logging. Then it calls the parent Platform::initialize(),
 * which takes ownership of the active application. It's the platforms responsibility
 * to then call VulkanSample::prepare() to prepare the vulkan sample when it is ready.
 *
 * @subsection sample_init Sample initialization
 * The preparation step is divided in two steps, one in VulkanSample and the other in the
 * specific sample, such as SurfaceRotation.
 * VulkanSample::prepare() contains functions that do not require customization,
 * including creating a Vulkan instance, the surface and getting physical devices.
 * The prepare() function for the specific sample completes the initialization, including:
 * - setting enabled Stats
 * - creating the Device
 * - creating the Swapchain
 * - creating the RenderContext (or child class)
 * - preparing the RenderContext
 * - loading the sg::Scene
 * - creating the RenderPipeline with ShaderModule (s)
 * - creating the sg::Camera
 * - creating the Gui
 *
 * @section frame_rendering Frame rendering
 *
 * @subsection update Update function
 * Rendering happens in the update() function. Each sample can override it, e.g.
 * to recreate the Swapchain in SwapchainImages when required by user input.
 * Typically a sample will then call VulkanSample::update().
 *
 * @subsection rendering Rendering
 * A series of steps are performed, some of which can be customized (it will be
 * highlighted when that's the case):
 *
 * - calling sg::Script::update() for all sg::Script (s)
 * - beginning a frame in RenderContext (does the necessary waiting on fences and
 *   acquires an core::Image)
 * - requesting a CommandBuffer
 * - updating Stats and Gui
 * - getting an active RenderTarget constructed by the factory function of the RenderFrame
 * - setting up barriers for color and depth, note that these are only for the default RenderTarget
 * - calling VulkanSample::draw_renderpass (see below)
 * - setting up a barrier for the Swapchain transition to present
 * - submitting the CommandBuffer and end the Frame (present)
 *
 * @subsection draw_renderpass Draw renderpass
 * The function starts and ends a RenderPass which includes setting up viewport, scissors,
 * blend state (etc.) and calling draw_scene.
 * Note that RenderPipeline::draw is not virtual in RenderPipeline, but internally it calls
 * Subpass::draw for each Subpass, which is virtual and can be customized.
 *
 * @section framework_classes Main framework classes
 *
 * - RenderContext
 * - RenderFrame
 * - RenderTarget
 * - RenderPipeline
 * - ShaderModule
 * - ResourceCache
 * - BufferPool
 * - Core classes: Classes in vkb::core wrap Vulkan objects for indexing and hashing.
 */

class RenderPipeline;
class Stats;

namespace core
{
class HPPDevice;
class HPPInstance;
class HPPPhysicalDevice;
}        // namespace core

namespace rendering
{
template <vkb::BindingType bindingType>
class RenderContext;

class HPPRenderTarget;
}        // namespace rendering

namespace stats
{
class HPPStats;
}

template <vkb::BindingType bindingType>
class VulkanSample : public vkb::Application
{
	using Parent = vkb::Application;

	/// <summary>
	/// PUBLIC INTERFACE
	/// </summary>
  public:
	VulkanSample() = default;
	~VulkanSample() override;

	using RenderPipelineType = typename std::conditional<bindingType == BindingType::Cpp, vkb::rendering::HPPRenderPipeline, vkb::RenderPipeline>::type;
	using RenderTargetType   = typename std::conditional<bindingType == BindingType::Cpp, vkb::rendering::HPPRenderTarget, vkb::RenderTarget>::type;
	using SceneType          = typename std::conditional<bindingType == BindingType::Cpp, vkb::scene_graph::HPPScene, vkb::sg::Scene>::type;
	using StatsType          = typename std::conditional<bindingType == BindingType::Cpp, vkb::stats::HPPStats, vkb::Stats>::type;

	using Extent2DType                      = typename std::conditional<bindingType == BindingType::Cpp, vk::Extent2D, VkExtent2D>::type;
	using DebugReportCallbackCreateInfoType = typename std::conditional<bindingType == BindingType::Cpp, vk::DebugReportCallbackCreateInfoEXT, VkDebugReportCallbackCreateInfoEXT>::type;
	using DebugUtilsMessengerCreateInfoType = typename std::conditional<bindingType == BindingType::Cpp, vk::DebugUtilsMessengerCreateInfoEXT, VkDebugUtilsMessengerCreateInfoEXT>::type;
	using InstanceCreateFlagsType           = typename std::conditional<bindingType == BindingType::Cpp, vk::InstanceCreateFlags, VkInstanceCreateFlags>::type;
	using LayerSettingType                  = typename std::conditional<bindingType == BindingType::Cpp, vk::LayerSettingEXT, VkLayerSettingEXT>::type;
	using PhysicalDeviceType                = typename std::conditional<bindingType == BindingType::Cpp, vk::PhysicalDevice, VkPhysicalDevice>::type;
	using SurfaceFormatType                 = typename std::conditional<bindingType == BindingType::Cpp, vk::SurfaceFormatKHR, VkSurfaceFormatKHR>::type;
	using SurfaceType                       = typename std::conditional<bindingType == BindingType::Cpp, vk::SurfaceKHR, VkSurfaceKHR>::type;
	using ValidationFeatureEnableType       = typename std::conditional<bindingType == BindingType::Cpp, vk::ValidationFeatureEnableEXT, VkValidationFeatureEnableEXT>::type;

	Configuration                                    &get_configuration();
	vkb::rendering::RenderContext<bindingType>       &get_render_context();
	vkb::rendering::RenderContext<bindingType> const &get_render_context() const;
	bool                                              has_render_context() const;

	/// <summary>
	/// PROTECTED VIRTUAL INTERFACE
	/// </summary>
  protected:
	// from Application
	void input_event(const InputEvent &input_event) override;
	void finish() override;
	bool resize(uint32_t width, uint32_t height) override;

	/**
	 * @brief Create the Vulkan device used by this sample
	 * @note Can be overridden to implement custom device creation
	 */
	virtual std::unique_ptr<vkb::core::Device<bindingType>> create_device(vkb::core::PhysicalDevice<bindingType> &gpu);

	/**
	 * @brief Create the Vulkan instance used by this sample
	 * @note Can be overridden to implement custom instance creation
	 */
	virtual std::unique_ptr<vkb::core::Instance<bindingType>> create_instance();

	/**
	 * @brief Override this to customise the creation of the render_context
	 */
	virtual void create_render_context();

	virtual size_t determine_physical_device_score(PhysicalDeviceType const &gpu) const;

	/**
	 * @brief Prepares the render target and draws to it, calling draw_renderpass
	 * @param command_buffer The command buffer to record the commands to
	 * @param render_target The render target that is being drawn to
	 */
	virtual void draw(vkb::core::CommandBuffer<bindingType> &command_buffer, RenderTargetType &render_target);

	/**
	 * @brief Samples should override this function to draw their interface
	 */
	virtual void draw_gui();

	/**
	 * @brief Starts the render pass, executes the render pipeline, and then ends the render pass
	 * @param command_buffer The command buffer to record the commands to
	 * @param render_target The render target that is being drawn to
	 */
	virtual void draw_renderpass(vkb::core::CommandBuffer<bindingType> &command_buffer, RenderTargetType &render_target);

	virtual uint32_t                                 get_api_version() const;
	virtual DebugReportCallbackCreateInfoType const *get_debug_report_callback_create_info() const;
	virtual DebugUtilsMessengerCreateInfoType const *get_debug_utils_messenger_create_info() const;
	virtual InstanceCreateFlagsType                  get_instance_create_flags(std::vector<std::string> const &enabled_extensions) const;
	virtual void const                              *get_instance_create_info_extensions(std::vector<std::string> const &enabled_layers, std::vector<std::string> const &enabled_extensions) const;

	/**
	 * @brief Override this to customise the creation of the swapchain and render_context
	 */
	virtual void prepare_render_context();

	/**
	 * @brief Triggers the render pipeline, it can be overridden by samples to specialize their rendering logic
	 * @param command_buffer The command buffer to record the commands to
	 */
	virtual void render(vkb::core::CommandBuffer<bindingType> &command_buffer);

	/**
	 * @brief Request features from the gpu based on what is supported
	 */
	virtual void request_gpu_features(vkb::core::PhysicalDevice<bindingType> &gpu);

	virtual void request_instance_extensions(std::unordered_map<std::string, vkb::RequestMode> &requested_extensions) const;
	virtual void request_layers(std::unordered_map<std::string, vkb::RequestMode> &requested_layers) const;
	virtual void request_layer_settings(std::vector<LayerSettingType> &requested_layer_settings) const;
	virtual void request_validation_feature_enables(std::vector<ValidationFeatureEnableType> &requested_validation_feature_enables) const;

	/**
	 * @brief Resets the stats view max values for high demanding configs
	 *        Should be overridden by the samples since they
	 *        know which configuration is resource demanding
	 */
	virtual void reset_stats_view();

	/**
	 * @brief Updates the debug window, samples can override this to insert their own data elements
	 */
	virtual void update_debug_window();

	/// <summary>
	/// PROTECTED INTERFACE
	/// </summary>
	/**
	 * @brief Add a sample-specific device extension
	 * @param extension The extension name
	 * @param optional (Optional) Whether the extension is optional
	 */
	void add_device_extension(const char *extension, bool optional = false);

	void create_gui(const Window &window, StatsType const *stats = nullptr, const float font_size = 21.0f, bool explicit_update = false);

	/**
	 * @brief A helper to create a render context
	 */
	void create_render_context(const std::vector<SurfaceFormatType> &surface_priority_list);

	vkb::core::Device<bindingType>         &get_device();
	vkb::core::Device<bindingType> const   &get_device() const;
	vkb::Gui<bindingType>                  &get_gui();
	vkb::Gui<bindingType> const            &get_gui() const;
	vkb::core::Instance<bindingType>       &get_instance();
	vkb::core::Instance<bindingType> const &get_instance() const;
	RenderPipelineType                     &get_render_pipeline();
	RenderPipelineType const               &get_render_pipeline() const;
	SceneType                              &get_scene();
	StatsType                              &get_stats();
	SurfaceType                             get_surface() const;
	std::vector<SurfaceFormatType>         &get_surface_priority_list();
	std::vector<SurfaceFormatType> const   &get_surface_priority_list() const;
	bool                                    has_device() const;
	bool                                    has_instance() const;
	bool                                    has_gui() const;
	bool                                    has_render_pipeline() const;
	bool                                    has_scene();

	/**
	 * @brief Loads the scene
	 *
	 * @param path The path of the glTF file
	 */
	void load_scene(const std::string &path);

	/**
	 * @brief Additional sample initialization
	 */
	bool prepare(const ApplicationOptions &options) override;

	void select_physical_device();

	/**
	 * @brief Sets whether or not the first graphics queue should have higher priority than other queues.
	 * Very specific feature which is used by async compute samples.
	 * Needs to be called before prepare().
	 * @param enable If true, present queue will have prio 1.0 and other queues have prio 0.5.
	 * Default state is false, where all queues have 0.5 priority.
	 */
	void set_high_priority_graphics_queue_enable(bool enable);

	void set_render_context(std::unique_ptr<vkb::rendering::RenderContext<bindingType>> &&render_context);

	void set_render_pipeline(std::unique_ptr<RenderPipelineType> &&render_pipeline);

	/**
	 * @brief Main loop sample events
	 */
	void update(float delta_time) override;

	/**
	 * @brief Update GUI
	 * @param delta_time
	 */
	void update_gui(float delta_time);

	/**
	 * @brief Update scene
	 * @param delta_time
	 */
	void update_scene(float delta_time);

	/**
	 * @brief Update counter values
	 * @param delta_time
	 */
	void update_stats(float delta_time);

	/**
	 * @brief Set viewport and scissor state in command buffer for a given extent
	 */
	static void set_viewport_and_scissor(vkb::core::CommandBuffer<bindingType> const &command_buffer, Extent2DType const &extent);

	/// <summary>
	/// PRIVATE INTERFACE
	/// </summary>
  private:
	void        create_render_context_impl(const std::vector<vk::SurfaceFormatKHR> &surface_priority_list);
	size_t      determine_physical_device_score_impl(vk::PhysicalDevice const &gpu) const;
	void        draw_impl(vkb::core::CommandBufferCpp &command_buffer, vkb::rendering::HPPRenderTarget &render_target);
	void        draw_renderpass_impl(vkb::core::CommandBufferCpp &command_buffer, vkb::rendering::HPPRenderTarget &render_target);
	void        render_impl(vkb::core::CommandBufferCpp &command_buffer);
	void        request_layer_settings_impl(std::vector<vk::LayerSettingEXT> &requested_layer_settings) const;
	static void set_viewport_and_scissor_impl(vkb::core::CommandBufferCpp const &command_buffer, vk::Extent2D const &extent);

	/**
	 * @brief Get sample-specific device extensions.
	 *
	 * @return Map of device extensions and whether or not they are optional. Default is empty map.
	 */
	std::unordered_map<const char *, bool> const &get_device_extensions() const;

	/// <summary>
	/// PRIVATE MEMBERS
	/// </summary>
  private:
	/**
	 * @brief The Vulkan instance
	 */
	std::unique_ptr<vkb::core::InstanceCpp> instance;

	vk::DebugReportCallbackEXT debug_report_callback;
	vk::DebugUtilsMessengerEXT debug_utils_messenger;

	/**
	 * @brief The physical device selected for this sample
	 */
	std::unique_ptr<vkb::core::PhysicalDeviceCpp> physical_device;

	/**
	 * @brief The Vulkan device
	 */
	std::unique_ptr<vkb::core::DeviceCpp> device;

	/**
	 * @brief Context used for rendering, it is responsible for managing the frames and their underlying images
	 */
	std::unique_ptr<vkb::rendering::RenderContextCpp> render_context;

	/**
	 * @brief Pipeline used for rendering, it should be set up by the concrete sample
	 */
	std::unique_ptr<vkb::rendering::HPPRenderPipeline> render_pipeline;

	/**
	 * @brief Holds all scene information
	 */
	std::unique_ptr<vkb::scene_graph::HPPScene> scene;

	std::unique_ptr<vkb::GuiCpp> gui;

	std::unique_ptr<vkb::stats::HPPStats> stats;

	static constexpr float STATS_VIEW_RESET_TIME{10.0f};        // 10 seconds

	/**
	 * @brief The Vulkan surface
	 */
	vk::SurfaceKHR surface;

	/**
	 * @brief A list of surface formats in order of priority (vector[0] has high priority, vector[size-1] has low priority)
	 */
	std::vector<vk::SurfaceFormatKHR> surface_priority_list = {
	    {vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear},
	    {vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear}};

	/**
	 * @brief The configuration of the sample
	 */
	Configuration configuration{};

	/** @brief Set of device extensions to be enabled for this example and whether they are optional (must be set in the derived constructor) */
	std::unordered_map<const char *, bool> device_extensions;

	/** @brief Whether or not we want a high priority graphics queue. */
	bool high_priority_graphics_queue{false};

	std::unique_ptr<vkb::core::HPPDebugUtils> debug_utils;

  public:
	/**
	 * @brief Can be set from the GPU selection plugin to explicitly select a GPU instead
	 */
	inline static uint32_t selected_gpu_index = ~0;
};

using VulkanSampleC   = VulkanSample<vkb::BindingType::C>;
using VulkanSampleCpp = VulkanSample<vkb::BindingType::Cpp>;

// Member function definitions

template <vkb::BindingType bindingType>
inline VulkanSample<bindingType>::~VulkanSample()
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
	if (debug_utils_messenger)
	{
		instance->get_handle().destroyDebugUtilsMessengerEXT(debug_utils_messenger);
	}
	if (debug_report_callback)
	{
		instance->get_handle().destroyDebugReportCallbackEXT(debug_report_callback);
	}

	instance.reset();
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::add_device_extension(const char *extension, bool optional)
{
	device_extensions[extension] = optional;
}

template <vkb::BindingType bindingType>
inline std::unique_ptr<typename vkb::core::Device<bindingType>>
    VulkanSample<bindingType>::create_device(vkb::core::PhysicalDevice<bindingType> &gpu)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return std::make_unique<vkb::core::DeviceCpp>(
		    gpu, surface, std::move(debug_utils), get_device_extensions(), [this](vkb::core::PhysicalDeviceCpp &gpu) { request_gpu_features(gpu); });
	}
	else
	{
		return std::make_unique<vkb::core::DeviceC>(gpu,
		                                            static_cast<VkSurfaceKHR>(surface),
		                                            std::unique_ptr<vkb::DebugUtils>(reinterpret_cast<vkb::DebugUtils *>(debug_utils.release())),
		                                            get_device_extensions(),
		                                            [this](vkb::core::PhysicalDeviceC &gpu) { request_gpu_features(gpu); });
	}
}

template <vkb::BindingType bindingType>
inline std::unique_ptr<vkb::core::Instance<bindingType>> VulkanSample<bindingType>::create_instance()
{
	std::unordered_map<std::string, vkb::RequestMode> requested_layers, requested_extensions;
	request_layers(requested_layers);
	request_instance_extensions(requested_extensions);

	return std::make_unique<vkb::core::Instance<bindingType>>(
	    get_name(),
	    get_api_version(),
	    requested_layers,
	    requested_extensions,
	    [this](std::vector<std::string> const &enabled_layers, std::vector<std::string> const &enabled_extensions) { return get_instance_create_info_extensions(enabled_layers, enabled_extensions); },
	    [this](std::vector<std::string> const &enabled_extensions) {
		    if constexpr (bindingType == BindingType::Cpp)
		    {
			    return get_instance_create_flags(enabled_extensions);
		    }
		    else
		    {
			    return static_cast<VkInstanceCreateFlags>(get_instance_create_flags(enabled_extensions));
		    }
	    });
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::create_render_context()
{
	create_render_context_impl(surface_priority_list);
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::create_render_context(const std::vector<SurfaceFormatType> &surface_priority_list)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		create_render_context_impl(surface_priority_list);
	}
	else
	{
		create_render_context_impl(reinterpret_cast<std::vector<vk::SurfaceFormatKHR> const &>(surface_priority_list));
	}
}

template <vkb::BindingType bindingType>
void VulkanSample<bindingType>::create_render_context_impl(const std::vector<vk::SurfaceFormatKHR> &surface_priority_list)
{
#ifdef VK_USE_PLATFORM_ANDROID_KHR
	vk::PresentModeKHR              present_mode = (window->get_properties().vsync == Window::Vsync::OFF) ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eFifo;
	std::vector<vk::PresentModeKHR> present_mode_priority_list{vk::PresentModeKHR::eFifo, vk::PresentModeKHR::eMailbox, vk::PresentModeKHR::eImmediate};
#else
	vk::PresentModeKHR               present_mode = (window->get_properties().vsync == Window::Vsync::ON) ? vk::PresentModeKHR::eFifo : vk::PresentModeKHR::eMailbox;
	std::vector<vk::PresentModeKHR>  present_mode_priority_list{vk::PresentModeKHR::eMailbox, vk::PresentModeKHR::eImmediate, vk::PresentModeKHR::eFifo};
#endif

	render_context =
	    std::make_unique<vkb::rendering::RenderContextCpp>(*device, surface, *window, present_mode, present_mode_priority_list, surface_priority_list);
}

template <vkb::BindingType bindingType>
inline size_t VulkanSample<bindingType>::determine_physical_device_score(PhysicalDeviceType const &gpu) const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return determine_physical_device_score_impl(gpu);
	}
	else
	{
		return determine_physical_device_score_impl(reinterpret_cast<vk::PhysicalDevice const &>(gpu));
	}
}

template <vkb::BindingType bindingType>
inline size_t VulkanSample<bindingType>::determine_physical_device_score_impl(vk::PhysicalDevice const &gpu) const
{
	// Prefer discrete GPUs that support presenting to our surface, as they are most likely to provide good performance for rendering and presenting.
	auto supports_surface = [this, &gpu]() {
		for (uint32_t queue_idx = 0; queue_idx < gpu.getQueueFamilyProperties().size(); ++queue_idx)
		{
			if (gpu.getSurfaceSupportKHR(queue_idx, surface))
			{
				return true;
			}
		}
		return false;
	};
	return (gpu.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu) && supports_surface() ? 1000 : 1;
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::draw(vkb::core::CommandBuffer<bindingType> &command_buffer, RenderTargetType &render_target)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		draw_impl(command_buffer, render_target);
	}
	else
	{
		draw_impl(reinterpret_cast<vkb::core::CommandBufferCpp &>(command_buffer),
		          reinterpret_cast<vkb::rendering::HPPRenderTarget &>(render_target));
	}
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::draw_impl(vkb::core::CommandBufferCpp     &command_buffer,
                                                 vkb::rendering::HPPRenderTarget &render_target)
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
		render_target.set_layout(0, memory_barrier.new_layout);

		// Skip 1 as it is handled later as a depth-stencil attachment
		for (size_t i = 2; i < views.size(); ++i)
		{
			command_buffer.image_memory_barrier(views[i], memory_barrier);
			render_target.set_layout(static_cast<uint32_t>(i), memory_barrier.new_layout);
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
		render_target.set_layout(1, memory_barrier.new_layout);
	}

	// draw_renderpass is a virtual function, thus we have to call that, instead of directly calling draw_renderpass_impl!
	if constexpr (bindingType == BindingType::Cpp)
	{
		draw_renderpass(command_buffer, render_target);
	}
	else
	{
		draw_renderpass(reinterpret_cast<vkb::core::CommandBufferC &>(command_buffer), reinterpret_cast<vkb::RenderTarget &>(render_target));
	}

	{
		vkb::common::HPPImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout      = vk::ImageLayout::eColorAttachmentOptimal;
		memory_barrier.new_layout      = vk::ImageLayout::ePresentSrcKHR;
		memory_barrier.src_access_mask = vk::AccessFlagBits::eColorAttachmentWrite;
		memory_barrier.src_stage_mask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		memory_barrier.dst_stage_mask  = vk::PipelineStageFlagBits::eBottomOfPipe;

		command_buffer.image_memory_barrier(views[0], memory_barrier);
		render_target.set_layout(0, memory_barrier.new_layout);
	}
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::draw_gui()
{
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::draw_renderpass(vkb::core::CommandBuffer<bindingType> &command_buffer, RenderTargetType &render_target)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		draw_renderpass_impl(command_buffer, render_target);
	}
	else
	{
		draw_renderpass_impl(reinterpret_cast<vkb::core::CommandBufferCpp &>(command_buffer),
		                     reinterpret_cast<vkb::rendering::HPPRenderTarget &>(render_target));
	}
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::draw_renderpass_impl(vkb::core::CommandBufferCpp     &command_buffer,
                                                            vkb::rendering::HPPRenderTarget &render_target)
{
	set_viewport_and_scissor_impl(command_buffer, render_target.get_extent());

	// render is a virtual function, thus we have to call that, instead of directly calling render_impl!
	if constexpr (bindingType == BindingType::Cpp)
	{
		render(command_buffer);
	}
	else
	{
		render(reinterpret_cast<vkb::core::CommandBufferC &>(command_buffer));
	}

	if (gui)
	{
		gui->draw(command_buffer);
	}

	command_buffer.get_handle().endRenderPass();
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::finish()
{
	Parent::finish();

	if (device)
	{
		device->get_handle().waitIdle();
	}
}

template <vkb::BindingType bindingType>
inline uint32_t VulkanSample<bindingType>::get_api_version() const
{
	return VK_API_VERSION_1_1;
}

inline bool enable_layer_setting(vk::LayerSettingEXT const        &requested_layer_setting,
                                 std::vector<std::string> const   &enabled_layers,
                                 std::vector<vk::LayerSettingEXT> &enabled_layer_settings)
{
	// We are checking if the layer is available.
	// Vulkan does not provide a reflection API for layer settings. Layer settings are described in each layer JSON manifest.
	bool is_available = std::ranges::any_of(
	    enabled_layers, [&requested_layer_setting](auto const &enabled_layer) { return enabled_layer == requested_layer_setting.pLayerName; });

#if defined(PLATFORM__MACOS)
	// On Apple the MoltenVK driver configuration layer is implicitly enabled and available, and cannot be explicitly added or checked via enabled_layers.
	if (!is_available && strcmp(requested_layer_setting.pLayerName, "MoltenVK") == 0)
	{
		// Check for VK_EXT_layer_settings extension in the driver which indicates MoltenVK vs. KosmicKrisp (note: VK_MVK_moltenvk extension is deprecated).
		std::vector<vk::ExtensionProperties> available_instance_extensions = vk::enumerateInstanceExtensionProperties();
		if (std::ranges::any_of(available_instance_extensions,
		                        [](vk::ExtensionProperties const &extension) { return strcmp(extension.extensionName, VK_EXT_LAYER_SETTINGS_EXTENSION_NAME) == 0; }))
		{
			is_available = true;
		}
	}
#endif

	if (!is_available)
	{
		LOGW("Layer: {} not found. Disabling layer setting: {}", requested_layer_setting.pLayerName, requested_layer_setting.pSettingName);
		return false;
	}

	bool is_already_enabled = std::ranges::any_of(enabled_layer_settings,
	                                              [&requested_layer_setting](vk::LayerSettingEXT const &enabled_layer_setting) {
		                                              return (strcmp(requested_layer_setting.pLayerName, enabled_layer_setting.pLayerName) == 0) &&
		                                                     (strcmp(requested_layer_setting.pSettingName, enabled_layer_setting.pSettingName) == 0);
	                                              });

	if (is_already_enabled)
	{
		LOGW("Ignoring duplicated layer setting {} in layer {}.", requested_layer_setting.pSettingName, requested_layer_setting.pLayerName);
		return false;
	}

	LOGI("Enabling layer setting {} in layer {}.", requested_layer_setting.pSettingName, requested_layer_setting.pLayerName);
	enabled_layer_settings.push_back(requested_layer_setting);
	return true;
}

inline bool enable_layer_setting(VkLayerSettingEXT const          &requested_layer_setting,
                                 std::vector<std::string> const   &enabled_layers,
                                 std::vector<vk::LayerSettingEXT> &enabled_layer_settings)
{
	return enable_layer_setting(reinterpret_cast<vk::LayerSettingEXT const &>(requested_layer_setting), enabled_layers, enabled_layer_settings);
}

template <vkb::BindingType bindingType>
inline typename VulkanSample<bindingType>::DebugReportCallbackCreateInfoType const *VulkanSample<bindingType>::get_debug_report_callback_create_info() const
{
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	static vk::DebugReportCallbackCreateInfoEXT debug_report_callback_createInfo{.flags       = vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning | vk::DebugReportFlagBitsEXT::ePerformanceWarning,
	                                                                             .pfnCallback = vkb::core::debug_callback};
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return &debug_report_callback_createInfo;
	}
	else
	{
		return reinterpret_cast<VkDebugReportCallbackCreateInfoEXT *>(&debug_report_callback_createInfo);
	}
#else
	return nullptr;
#endif
}

template <vkb::BindingType bindingType>
inline typename VulkanSample<bindingType>::DebugUtilsMessengerCreateInfoType const *VulkanSample<bindingType>::get_debug_utils_messenger_create_info() const
{
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	static vk::DebugUtilsMessengerCreateInfoEXT debug_utils_messenger_create_info{.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
	                                                                              .messageType     = vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
	                                                                              .pfnUserCallback = vkb::core::debug_utils_messenger_callback};
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return &debug_utils_messenger_create_info;
	}
	else
	{
		return reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT *>(&debug_utils_messenger_create_info);
	}
#else
	return nullptr;
#endif
}

template <vkb::BindingType bindingType>
inline typename VulkanSample<bindingType>::InstanceCreateFlagsType VulkanSample<bindingType>::get_instance_create_flags(std::vector<std::string> const &enabled_extensions) const
{
	vk::InstanceCreateFlags flags;
#if defined(VKB_ENABLE_PORTABILITY)
	if (std::ranges::any_of(enabled_extensions,
	                        [](auto const &enabled_extension) { return enabled_extension == VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME; }))
	{
		flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
	}
#endif
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return flags;
	}
	else
	{
		return static_cast<VkInstanceCreateFlags>(flags);
	}
}

template <vkb::BindingType bindingType>
inline void const *VulkanSample<bindingType>::get_instance_create_info_extensions(std::vector<std::string> const &enabled_layers,
                                                                                  std::vector<std::string> const &enabled_extensions) const
{
	void const *pNext = nullptr;

	if (contains(enabled_extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
	{
		pNext = get_debug_utils_messenger_create_info();
	}
	else if (contains(enabled_extensions, VK_EXT_DEBUG_REPORT_EXTENSION_NAME))
	{
		pNext = get_debug_report_callback_create_info();
	}

	if (contains(enabled_extensions, VK_EXT_LAYER_SETTINGS_EXTENSION_NAME))
	{
		std::vector<LayerSettingType> requested_layer_settings;
		request_layer_settings(requested_layer_settings);

		static std::vector<vk::LayerSettingEXT> enabled_layer_settings;
		for (auto const &layer_setting : requested_layer_settings)
		{
			enable_layer_setting(layer_setting, enabled_layers, enabled_layer_settings);
		}

		if (!enabled_layer_settings.empty())
		{
			// If layer settings are defined, then activate the sample's required layer settings during instance creation
			static vk::LayerSettingsCreateInfoEXT layer_settings_create_info_ext{.pNext        = pNext,
			                                                                     .settingCount = static_cast<uint32_t>(enabled_layer_settings.size()),
			                                                                     .pSettings    = enabled_layer_settings.data()};
			pNext = &layer_settings_create_info_ext;
		}
	}
	else if (contains(enabled_extensions, VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME))
	{
		static std::vector<ValidationFeatureEnableType> requested_validation_feature_enables;
		request_validation_feature_enables(requested_validation_feature_enables);

		if (!requested_validation_feature_enables.empty())
		{
			static vk::ValidationFeaturesEXT validation_features_ext{
			    .pNext                         = pNext,
			    .enabledValidationFeatureCount = static_cast<uint32_t>(requested_validation_feature_enables.size()),
			    .pEnabledValidationFeatures    = reinterpret_cast<vk::ValidationFeatureEnableEXT const *>(requested_validation_feature_enables.data())};
			pNext = &validation_features_ext;
		}
	}

	return pNext;
}

template <vkb::BindingType bindingType>
inline Configuration &VulkanSample<bindingType>::get_configuration()
{
	return configuration;
}

template <vkb::BindingType bindingType>
inline vkb::core::Device<bindingType> const &VulkanSample<bindingType>::get_device() const
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return *device;
	}
	else
	{
		return reinterpret_cast<vkb::core::DeviceC const &>(*device);
	}
}

template <vkb::BindingType bindingType>
inline vkb::core::Device<bindingType> &VulkanSample<bindingType>::get_device()
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return *device;
	}
	else
	{
		return reinterpret_cast<vkb::core::DeviceC &>(*device);
	}
}

template <vkb::BindingType bindingType>
inline std::unordered_map<const char *, bool> const &VulkanSample<bindingType>::get_device_extensions() const
{
	return device_extensions;
}

template <vkb::BindingType bindingType>
inline vkb::Gui<bindingType> &VulkanSample<bindingType>::get_gui()
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return *gui;
	}
	else
	{
		return reinterpret_cast<vkb::GuiC &>(*gui);
	}
}

template <vkb::BindingType bindingType>
inline vkb::Gui<bindingType> const &VulkanSample<bindingType>::get_gui() const
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return *gui;
	}
	else
	{
		return reinterpret_cast<vkb::GuiC const &>(*gui);
	}
}

template <vkb::BindingType bindingType>
inline std::vector<typename VulkanSample<bindingType>::SurfaceFormatType> &VulkanSample<bindingType>::get_surface_priority_list()
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return surface_priority_list;
	}
	else
	{
		return reinterpret_cast<std::vector<VkSurfaceFormatKHR> &>(surface_priority_list);
	}
}

template <vkb::BindingType bindingType>
inline std::vector<typename VulkanSample<bindingType>::SurfaceFormatType> const &VulkanSample<bindingType>::get_surface_priority_list() const
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return surface_priority_list;
	}
	else
	{
		return reinterpret_cast<std::vector<VkSurfaceFormatKHR> const &>(surface_priority_list);
	}
}

template <vkb::BindingType bindingType>
inline vkb::core::Instance<bindingType> &VulkanSample<bindingType>::get_instance()
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return *instance;
	}
	else
	{
		return reinterpret_cast<vkb::core::InstanceC &>(*instance);
	}
}

template <vkb::BindingType bindingType>
inline vkb::core::Instance<bindingType> const &VulkanSample<bindingType>::get_instance() const
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return *instance;
	}
	else
	{
		return reinterpret_cast<vkb::core::InstanceC const &>(*instance);
	}
}

template <vkb::BindingType bindingType>
inline vkb::rendering::RenderContext<bindingType> const &VulkanSample<bindingType>::get_render_context() const
{
	assert(render_context && "Render context is not valid");
	if constexpr (bindingType == BindingType::Cpp)
	{
		return *render_context;
	}
	else
	{
		return reinterpret_cast<vkb::rendering::RenderContextC const &>(*render_context);
	}
}

template <vkb::BindingType bindingType>
inline vkb::rendering::RenderContext<bindingType> &VulkanSample<bindingType>::get_render_context()
{
	assert(render_context && "Render context is not valid");
	if constexpr (bindingType == BindingType::Cpp)
	{
		return *render_context;
	}
	else
	{
		return reinterpret_cast<vkb::rendering::RenderContextC &>(*render_context);
	}
}

template <vkb::BindingType bindingType>
inline typename VulkanSample<bindingType>::RenderPipelineType const &VulkanSample<bindingType>::get_render_pipeline() const
{
	assert(render_pipeline && "Render pipeline was not created");
	if constexpr (bindingType == BindingType::Cpp)
	{
		return *render_pipeline;
	}
	else
	{
		return reinterpret_cast<vkb::RenderPipeline const &>(*render_pipeline);
	}
}

template <vkb::BindingType bindingType>
inline typename VulkanSample<bindingType>::RenderPipelineType &VulkanSample<bindingType>::get_render_pipeline()
{
	assert(render_pipeline && "Render pipeline was not created");
	if constexpr (bindingType == BindingType::Cpp)
	{
		return *render_pipeline;
	}
	else
	{
		return reinterpret_cast<vkb::RenderPipeline &>(*render_pipeline);
	}
}

template <vkb::BindingType bindingType>
inline typename VulkanSample<bindingType>::SceneType &VulkanSample<bindingType>::get_scene()
{
	assert(scene && "Scene not loaded");
	if constexpr (bindingType == BindingType::Cpp)
	{
		return *scene;
	}
	else
	{
		return reinterpret_cast<vkb::sg::Scene &>(*scene);
	}
}

template <vkb::BindingType bindingType>
inline typename VulkanSample<bindingType>::StatsType &VulkanSample<bindingType>::get_stats()
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return *stats;
	}
	else
	{
		return reinterpret_cast<vkb::Stats &>(*stats);
	}
}

template <vkb::BindingType bindingType>
inline typename VulkanSample<bindingType>::SurfaceType VulkanSample<bindingType>::get_surface() const
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return surface;
	}
	else
	{
		return static_cast<VkSurfaceKHR>(surface);
	}
}

template <vkb::BindingType bindingType>
inline bool VulkanSample<bindingType>::has_device() const
{
	return device != nullptr;
}

template <vkb::BindingType bindingType>
inline bool VulkanSample<bindingType>::has_instance() const
{
	return instance != nullptr;
}

template <vkb::BindingType bindingType>
inline bool VulkanSample<bindingType>::has_gui() const
{
	return gui != nullptr;
}

template <vkb::BindingType bindingType>
inline bool VulkanSample<bindingType>::has_render_context() const
{
	return render_context != nullptr;
}

template <vkb::BindingType bindingType>
inline bool VulkanSample<bindingType>::has_render_pipeline() const
{
	return render_pipeline != nullptr;
}

template <vkb::BindingType bindingType>
bool VulkanSample<bindingType>::has_scene()
{
	return scene != nullptr;
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::input_event(const InputEvent &input_event)
{
	Parent::input_event(input_event);

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

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::load_scene(const std::string &path)
{
	vkb::HPPGLTFLoader loader(*device);

	scene = loader.read_scene_from_file(path);

	if (!scene)
	{
		LOGE("Cannot load scene: {}", path.c_str());
		throw std::runtime_error("Cannot load scene: " + path);
	}
}

template <vkb::BindingType bindingType>
inline bool VulkanSample<bindingType>::prepare(const ApplicationOptions &options)
{
	if (!Parent::prepare(options))
	{
		return false;
	}

	LOGI("Initializing Vulkan sample");

	// initialize C++-Bindings default dispatcher, first step
#if defined(_HPP_VULKAN_LIBRARY)
	static vk::detail::DynamicLoader dl(_HPP_VULKAN_LIBRARY);
#else
	static vk::detail::DynamicLoader dl;
#endif
	VULKAN_HPP_DEFAULT_DISPATCHER.init(dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));

	bool headless = window->get_window_mode() == Window::Mode::Headless;

	// for a while we're running on mixed C- and C++-bindings, needing volk for the C-bindings!
	VkResult result = volkInitialize();
	if (result)
	{
		throw VulkanException(result, "Failed to initialize volk.");
	}

#ifdef VKB_VULKAN_DEBUG
	{
		std::vector<vk::ExtensionProperties> available_instance_extensions = vk::enumerateInstanceExtensionProperties();
		auto                                 debugExtensionIt =
		    std::ranges::find_if(available_instance_extensions,
		                         [](vk::ExtensionProperties const &ep) { return strcmp(ep.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0; });
		if (debugExtensionIt != available_instance_extensions.end())
		{
			LOGI("Vulkan debug utils enabled ({})", VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

			debug_utils = std::make_unique<vkb::core::HPPDebugUtilsExtDebugUtils>();
		}
	}
#endif

	if constexpr (bindingType == BindingType::Cpp)
	{
		instance = create_instance();
	}
	else
	{
		instance.reset(reinterpret_cast<vkb::core::InstanceCpp *>(create_instance().release()));
	}

	// initialize debug utils or report callback based on enabled extensions, if any
	if (instance->is_extension_enabled(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
	{
		auto const *debug_utils_messenger_create_info = get_debug_utils_messenger_create_info();
		if (debug_utils_messenger_create_info)
		{
			if constexpr (bindingType == BindingType::Cpp)
			{
				debug_utils_messenger = instance->get_handle().createDebugUtilsMessengerEXT(*debug_utils_messenger_create_info);
			}
			else
			{
				debug_utils_messenger = instance->get_handle().createDebugUtilsMessengerEXT(*reinterpret_cast<vk::DebugUtilsMessengerCreateInfoEXT const *>(debug_utils_messenger_create_info));
			}
		}
	}
	else if (instance->is_extension_enabled(VK_EXT_DEBUG_REPORT_EXTENSION_NAME))
	{
		auto const *debug_report_callback_create_info = get_debug_report_callback_create_info();
		if (debug_report_callback_create_info)
		{
			if constexpr (bindingType == BindingType::Cpp)
			{
				debug_report_callback = instance->get_handle().createDebugReportCallbackEXT(*debug_report_callback_create_info);
			}
			else
			{
				debug_report_callback = instance->get_handle().createDebugReportCallbackEXT(*reinterpret_cast<vk::DebugReportCallbackCreateInfoEXT const *>(debug_report_callback_create_info));
			}
		}
	}

	// Getting a valid vulkan surface from the platform
	surface = static_cast<vk::SurfaceKHR>(window->create_surface(reinterpret_cast<vkb::core::InstanceC &>(*instance)));
	if (!surface)
	{
		throw std::runtime_error("Failed to create window surface.");
	}

	select_physical_device();

	physical_device->set_high_priority_graphics_queue_enable(high_priority_graphics_queue);

	// Request to enable ASTC
	if (physical_device->get_features().textureCompressionASTC_LDR)
	{
		physical_device->get_mutable_requested_features().textureCompressionASTC_LDR = true;
	}

	// Creating vulkan device, specifying the swapchain extension always
	// If using VK_EXT_headless_surface, we still create and use a swap-chain
	{
		add_device_extension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		if (instance->is_extension_enabled(VK_KHR_DISPLAY_EXTENSION_NAME))
		{
			add_device_extension(VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME, /*optional=*/true);
		}
	}

	// Shaders generated by Slang require a certain SPIR-V environment that can't be satisfied by Vulkan 1.0, so we need to expliclity up that to at least 1.1 and enable some required extensions
	if (get_shading_language() == ShadingLanguage::SLANG)
	{
		assert(VK_API_VERSION_1_1 <= get_api_version());
		add_device_extension(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
		add_device_extension(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
		add_device_extension(VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME);
	}

#ifdef VKB_ENABLE_PORTABILITY
	// VK_KHR_portability_subset must be enabled if present in the implementation (e.g on macOS/iOS using MoltenVK with beta extensions enabled)
	add_device_extension(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME, /*optional=*/true);
#endif

#ifdef VKB_VULKAN_DEBUG
	if (!debug_utils)
	{
		std::vector<vk::ExtensionProperties> available_device_extensions = physical_device->get_handle().enumerateDeviceExtensionProperties();
		auto                                 debugExtensionIt =
		    std::ranges::find_if(available_device_extensions,
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

	if constexpr (bindingType == BindingType::Cpp)
	{
		device = create_device(*physical_device);
	}
	else
	{
		device.reset(reinterpret_cast<vkb::core::DeviceCpp *>(create_device(reinterpret_cast<vkb::core::PhysicalDeviceC &>(*physical_device)).release()));
	}

	// initialize C++-Bindings default dispatcher, optional third step
	VULKAN_HPP_DEFAULT_DISPATCHER.init(device->get_handle());

	create_render_context();
	prepare_render_context();

	stats = std::make_unique<vkb::stats::HPPStats>(*render_context);

	// Start the sample in the first GUI configuration
	configuration.reset();

	return true;
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::create_gui(const Window &window, StatsType const *stats, const float font_size, bool explicit_update)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		gui = std::make_unique<vkb::GuiCpp>(get_render_context(), window, stats, font_size, explicit_update);
	}
	else
	{
		gui = std::make_unique<vkb::GuiCpp>(reinterpret_cast<vkb::rendering::RenderContextCpp &>(get_render_context()),
		                                    window,
		                                    reinterpret_cast<vkb::stats::HPPStats const *>(stats),
		                                    font_size,
		                                    explicit_update);
	}
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::prepare_render_context()
{
	render_context->prepare();
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::render(vkb::core::CommandBuffer<bindingType> &command_buffer)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		render_impl(command_buffer);
	}
	else
	{
		render_impl(reinterpret_cast<vkb::core::CommandBufferCpp &>(command_buffer));
	}
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::render_impl(vkb::core::CommandBufferCpp &command_buffer)
{
	if (render_pipeline)
	{
		render_pipeline->draw(command_buffer, render_context->get_active_frame().get_render_target());
	}
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::request_gpu_features(vkb::core::PhysicalDevice<bindingType> &gpu)
{
	// To be overridden by sample
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::request_instance_extensions(std::unordered_map<std::string, vkb::RequestMode> &requested_extensions) const
{
	assert(window && "Window is not valid");
	std::vector<char const *> surface_extensions = window->get_required_surface_extensions();
	for (auto const &surface_extension : surface_extensions)
	{
		requested_extensions[surface_extension] = vkb::RequestMode::Required;
	}

	// We're going to use VK_KHR_swapchain on device creation, which requires VK_KHR_surface on instance creation
	// Just in case the windowing system didn't already request it...
	requested_extensions[VK_KHR_SURFACE_EXTENSION_NAME] = vkb::RequestMode::Required;

#if defined(VKB_VULKAN_DEBUG) || defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	requested_extensions[VK_EXT_DEBUG_UTILS_EXTENSION_NAME] = vkb::RequestMode::Required;
#endif

#if defined(VKB_ENABLE_PORTABILITY)
	requested_extensions[VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME] = vkb::RequestMode::Required;
#endif

	// VK_KHR_get_physical_device_properties2 is a prerequisite of VK_KHR_performance_query
	// which will be used for stats gathering where available.
	requested_extensions[VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME] = vkb::RequestMode::Required;

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	requested_extensions[VK_EXT_DEBUG_REPORT_EXTENSION_NAME] = vkb::RequestMode::Optional;
#endif

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS) && (defined(VKB_VALIDATION_LAYERS_GPU_ASSISTED) || defined(VKB_VALIDATION_LAYERS_BEST_PRACTICES) || defined(VKB_VALIDATION_LAYERS_SYNCHRONIZATION))
	requested_extensions[VK_EXT_LAYER_SETTINGS_EXTENSION_NAME] = vkb::RequestMode::Optional;
#endif
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::request_layers(std::unordered_map<std::string, vkb::RequestMode> &requested_layers) const
{
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	requested_layers["VK_LAYER_KHRONOS_validation"] = vkb::RequestMode::Required;
#endif
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::request_layer_settings(std::vector<LayerSettingType> &requested_layer_settings) const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		request_layer_settings_impl(requested_layer_settings);
	}
	else
	{
		request_layer_settings_impl(reinterpret_cast<std::vector<vk::LayerSettingEXT> &>(requested_layer_settings));
	}
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::request_layer_settings_impl(std::vector<vk::LayerSettingEXT> &requested_layer_settings) const
{
#if defined(VKB_VALIDATION_LAYERS_GPU_ASSISTED)
	static const vk::Bool32 setting_validate_gpuav = true;
	requested_layer_settings.push_back({"VK_LAYER_KHRONOS_validation", "gpuav_enable", vk::LayerSettingTypeEXT::eBool32, 1, &setting_validate_gpuav});
#endif

#if defined(VKB_VALIDATION_LAYERS_BEST_PRACTICES)
	static const vk::Bool32 setting_validate_best_practices        = true;
	static const vk::Bool32 setting_validate_best_practices_amd    = true;
	static const vk::Bool32 setting_validate_best_practices_arm    = true;
	static const vk::Bool32 setting_validate_best_practices_img    = true;
	static const vk::Bool32 setting_validate_best_practices_nvidia = true;
	requested_layer_settings.push_back(
	    {"VK_LAYER_KHRONOS_validation", "validate_best_practices", vk::LayerSettingTypeEXT::eBool32, 1, &setting_validate_best_practices});
	requested_layer_settings.push_back(
	    {"VK_LAYER_KHRONOS_validation", "validate_best_practices_amd", vk::LayerSettingTypeEXT::eBool32, 1, &setting_validate_best_practices_amd});
	requested_layer_settings.push_back(
	    {"VK_LAYER_KHRONOS_validation", "validate_best_practices_arm", vk::LayerSettingTypeEXT::eBool32, 1, &setting_validate_best_practices_arm});
	requested_layer_settings.push_back(
	    {"VK_LAYER_KHRONOS_validation", "validate_best_practices_img", vk::LayerSettingTypeEXT::eBool32, 1, &setting_validate_best_practices_img});
	requested_layer_settings.push_back(
	    {"VK_LAYER_KHRONOS_validation", "validate_best_practices_nvidia", vk::LayerSettingTypeEXT::eBool32, 1, &setting_validate_best_practices_nvidia});
#endif

#if defined(VKB_VALIDATION_LAYERS_SYNCHRONIZATION)
	static const vk::Bool32 setting_validate_sync            = true;
	static const vk::Bool32 setting_validate_sync_heuristics = true;
	requested_layer_settings.push_back({"VK_LAYER_KHRONOS_validation", "validate_sync", vk::LayerSettingTypeEXT::eBool32, 1, &setting_validate_sync});
	requested_layer_settings.push_back(
	    {"VK_LAYER_KHRONOS_validation", "syncval_shader_accesses_heuristic", vk::LayerSettingTypeEXT::eBool32, 1, &setting_validate_sync_heuristics});
#endif
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::request_validation_feature_enables(std::vector<ValidationFeatureEnableType> &requested_layer_settings) const
{
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::reset_stats_view()
{
}

template <vkb::BindingType bindingType>
inline bool VulkanSample<bindingType>::resize(uint32_t width, uint32_t height)
{
	if (!Parent::resize(width, height))
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

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::select_physical_device()
{
	assert(instance && "Instance must be created before selecting a physical device");

	std::vector<vk::PhysicalDevice> physical_devices = instance->get_handle().enumeratePhysicalDevices();
	if (physical_devices.empty())
	{
		throw std::runtime_error("Couldn't find a physical device that supports Vulkan.");
	}

	// A GPU can be explicitly selected via the command line (see plugins/gpu_selection.cpp), this overrides the below GPU selection algorithm
	auto physical_devices_it = physical_devices.begin();
	if (selected_gpu_index != ~0)
	{
		LOGI("Explicitly selecting GPU {}", selected_gpu_index);
		if (selected_gpu_index > physical_devices.size() - 1)
		{
			throw std::runtime_error("Selected GPU index is not within no. of available GPUs");
		}
		physical_devices_it = physical_devices.begin() + selected_gpu_index;
	}
	else
	{
		std::vector<size_t> gpu_scores;
		gpu_scores.reserve(physical_devices.size());
		for (auto const &physical_device : physical_devices)
		{
			if constexpr (bindingType == BindingType::Cpp)
			{
				gpu_scores.push_back(determine_physical_device_score(physical_device));
			}
			else
			{
				gpu_scores.push_back(determine_physical_device_score(reinterpret_cast<VkPhysicalDevice const &>(physical_device)));
			}
		}
		auto max_score_it   = std::ranges::max_element(gpu_scores);
		physical_devices_it = physical_devices.begin() + std::distance(gpu_scores.begin(), max_score_it);
	}

	physical_device = std::make_unique<vkb::core::PhysicalDeviceCpp>(*instance, *physical_devices_it);
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::set_high_priority_graphics_queue_enable(bool enable)
{
	high_priority_graphics_queue = enable;
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::set_render_context(std::unique_ptr<vkb::rendering::RenderContext<bindingType>> &&rc)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		render_context.reset(rc.release());
	}
	else
	{
		render_context.reset(reinterpret_cast<vkb::rendering::RenderContextCpp *>(rc.release()));
	}
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::set_render_pipeline(std::unique_ptr<RenderPipelineType> &&rp)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		render_pipeline.reset(rp.release());
	}
	else
	{
		render_pipeline.reset(reinterpret_cast<vkb::rendering::HPPRenderPipeline *>(rp.release()));
	}
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::set_viewport_and_scissor(vkb::core::CommandBuffer<bindingType> const &command_buffer, Extent2DType const &extent)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		set_viewport_and_scissor_impl(command_buffer, extent);
	}
	else
	{
		set_viewport_and_scissor_impl(reinterpret_cast<vkb::core::CommandBufferCpp const &>(command_buffer),
		                              reinterpret_cast<vk::Extent2D const &>(extent));
	}
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::set_viewport_and_scissor_impl(vkb::core::CommandBufferCpp const &command_buffer,
                                                                     vk::Extent2D const                &extent)
{
	command_buffer.get_handle().setViewport(0, {{0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f}});
	command_buffer.get_handle().setScissor(0, vk::Rect2D{{}, extent});
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::update(float delta_time)
{
	vkb::Application::update(delta_time);

	update_scene(delta_time);

	update_gui(delta_time);

	auto command_buffer = render_context->begin();

	// Collect the performance data for the sample graphs
	update_stats(delta_time);

	command_buffer->begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	stats->begin_sampling(*command_buffer);

	if constexpr (bindingType == BindingType::Cpp)
	{
		draw(*command_buffer, render_context->get_active_frame().get_render_target());
	}
	else
	{
		draw(reinterpret_cast<vkb::core::CommandBufferC &>(*command_buffer),
		     reinterpret_cast<vkb::RenderTarget &>(render_context->get_active_frame().get_render_target()));
	}

	stats->end_sampling(*command_buffer);
	command_buffer->end();

	render_context->submit(command_buffer);
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::update_debug_window()
{
	auto        driver_version     = device->get_gpu().get_driver_version();
	std::string driver_version_str = fmt::format("major: {} minor: {} patch: {}", driver_version.major, driver_version.minor, driver_version.patch);

	get_debug_info().template insert<field::Static, std::string>("driver_version", driver_version_str);
	get_debug_info().template insert<field::Static, std::string>("resolution",
	                                                             to_string(static_cast<VkExtent2D const &>(render_context->get_swapchain().get_extent())));
	get_debug_info().template insert<field::Static, std::string>("surface_format",
	                                                             to_string(render_context->get_swapchain().get_format()) + " (" +
	                                                                 to_string(vkb::common::get_bits_per_pixel(render_context->get_swapchain().get_format())) +
	                                                                 "bpp)");

	if (scene != nullptr)
	{
		get_debug_info().template insert<field::Static, uint32_t>("mesh_count", to_u32(scene->get_components<sg::SubMesh>().size()));
		get_debug_info().template insert<field::Static, uint32_t>("texture_count", to_u32(scene->get_components<sg::Texture>().size()));

		if (auto camera = scene->get_components<vkb::sg::Camera>()[0])
		{
			if (auto camera_node = camera->get_node())
			{
				const glm::vec3 &pos = camera_node->get_transform().get_translation();
				get_debug_info().template insert<field::Vector, float>("camera_pos", pos.x, pos.y, pos.z);
			}
		}
	}
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::update_gui(float delta_time)
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

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::update_scene(float delta_time)
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

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::update_stats(float delta_time)
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

}        // namespace vkb
