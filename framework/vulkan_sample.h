/* Copyright (c) 2019-2024, Arm Limited and Contributors
 * Copyright (c) 2021-2024, NVIDIA CORPORATION. All rights reserved.
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
#include "hpp_gltf_loader.h"
#include "hpp_gui.h"
#include "platform/application.h"
#include "rendering/hpp_render_pipeline.h"
#include "scene_graph/components/camera.h"
#include "scene_graph/hpp_scene.h"
#include "scene_graph/scripts/animation.h"

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
 * - calling VulkanSample::draw_swapchain_renderpass (see below)
 * - setting up a barrier for the Swapchain transition to present
 * - submitting the CommandBuffer and end the Frame (present)
 *
 * @subsection draw_swapchain Draw swapchain renderpass
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

class Gui;
class RenderPipeline;

namespace core
{
class HPPCommandBuffer;
class HPPDebugUtils;
class HPPDevice;
class HPPInstance;
class HPPPhysicalDevice;
}        // namespace core

namespace rendering
{
class HPPRenderContext;
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

	using CommandBufferType  = typename std::conditional<bindingType == BindingType::Cpp, vkb::core::HPPCommandBuffer, vkb::CommandBuffer>::type;
	using DeviceType         = typename std::conditional<bindingType == BindingType::Cpp, vkb::core::HPPDevice, vkb::Device>::type;
	using GuiType            = typename std::conditional<bindingType == BindingType::Cpp, vkb::HPPGui, vkb::Gui>::type;
	using InstanceType       = typename std::conditional<bindingType == BindingType::Cpp, vkb::core::HPPInstance, vkb::Instance>::type;
	using PhysicalDeviceType = typename std::conditional<bindingType == BindingType::Cpp, vkb::core::HPPPhysicalDevice, vkb::PhysicalDevice>::type;
	using RenderContextType  = typename std::conditional<bindingType == BindingType::Cpp, vkb::rendering::HPPRenderContext, vkb::RenderContext>::type;
	using RenderPipelineType = typename std::conditional<bindingType == BindingType::Cpp, vkb::rendering::HPPRenderPipeline, vkb::RenderPipeline>::type;
	using RenderTargetType   = typename std::conditional<bindingType == BindingType::Cpp, vkb::rendering::HPPRenderTarget, vkb::RenderTarget>::type;
	using SceneType          = typename std::conditional<bindingType == BindingType::Cpp, vkb::scene_graph::HPPScene, vkb::sg::Scene>::type;
	using StatsType          = typename std::conditional<bindingType == BindingType::Cpp, vkb::stats::HPPStats, vkb::Stats>::type;
	using Extent2DType       = typename std::conditional<bindingType == BindingType::Cpp, vk::Extent2D, VkExtent2D>::type;
	using LayerSettingType   = typename std::conditional<bindingType == BindingType::Cpp, vk::LayerSettingEXT, VkLayerSettingEXT>::type;
	using SurfaceFormatType  = typename std::conditional<bindingType == BindingType::Cpp, vk::SurfaceFormatKHR, VkSurfaceFormatKHR>::type;
	using SurfaceType        = typename std::conditional<bindingType == BindingType::Cpp, vk::SurfaceKHR, VkSurfaceKHR>::type;

	Configuration           &get_configuration();
	RenderContextType       &get_render_context();
	RenderContextType const &get_render_context() const;
	bool                     has_render_context() const;

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
	virtual std::unique_ptr<DeviceType> create_device(PhysicalDeviceType &gpu);

	/**
	 * @brief Create the Vulkan instance used by this sample
	 * @note Can be overridden to implement custom instance creation
	 */
	virtual std::unique_ptr<InstanceType> create_instance();

	/**
	 * @brief Override this to customise the creation of the render_context
	 */
	virtual void create_render_context();

	/**
	 * @brief Prepares the render target and draws to it, calling draw_renderpass
	 * @param command_buffer The command buffer to record the commands to
	 * @param render_target The render target that is being drawn to
	 */
	virtual void draw(CommandBufferType &command_buffer, RenderTargetType &render_target);

	/**
	 * @brief Samples should override this function to draw their interface
	 */
	virtual void draw_gui();

	/**
	 * @brief Starts the render pass, executes the render pipeline, and then ends the render pass
	 * @param command_buffer The command buffer to record the commands to
	 * @param render_target The render target that is being drawn to
	 */
	virtual void draw_renderpass(CommandBufferType &command_buffer, RenderTargetType &render_target);

	/**
	 * @brief Get additional sample-specific instance layers.
	 *
	 * @return Vector of additional instance layers. Default is empty vector.
	 */
	virtual const std::vector<const char *> get_validation_layers();

	/**
	 * @brief Override this to customise the creation of the swapchain and render_context
	 */
	virtual void prepare_render_context();

	/**
	 * @brief Triggers the render pipeline, it can be overridden by samples to specialize their rendering logic
	 * @param command_buffer The command buffer to record the commands to
	 */
	virtual void render(CommandBufferType &command_buffer);

	/**
	 * @brief Request features from the gpu based on what is supported
	 */
	virtual void request_gpu_features(PhysicalDeviceType &gpu);

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

	/**
	 * @brief Add a sample-specific instance extension
	 * @param extension The extension name
	 * @param optional (Optional) Whether the extension is optional
	 */
	void add_instance_extension(const char *extension, bool optional = false);

	/**
	 * @brief Add a sample-specific layer setting
	 * @param layerSetting The layer setting
	 */
	void add_layer_setting(LayerSettingType const &layerSetting);

	void create_gui(const Window &window, StatsType const *stats = nullptr, const float font_size = 21.0f, bool explicit_update = false);

	/**
	 * @brief A helper to create a render context
	 */
	void create_render_context(const std::vector<SurfaceFormatType> &surface_priority_list);

	DeviceType                           &get_device();
	DeviceType const                     &get_device() const;
	GuiType                              &get_gui();
	GuiType const                        &get_gui() const;
	InstanceType                         &get_instance();
	InstanceType const                   &get_instance() const;
	RenderPipelineType                   &get_render_pipeline();
	RenderPipelineType const             &get_render_pipeline() const;
	SceneType                            &get_scene();
	StatsType                            &get_stats();
	SurfaceType                           get_surface() const;
	std::vector<SurfaceFormatType>       &get_surface_priority_list();
	std::vector<SurfaceFormatType> const &get_surface_priority_list() const;
	bool                                  has_device() const;
	bool                                  has_instance() const;
	bool                                  has_gui() const;
	bool                                  has_render_pipeline() const;
	bool                                  has_scene();

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

	/**
	 * @brief Set the Vulkan API version to request at instance creation time
	 */
	void set_api_version(uint32_t requested_api_version);

	/**
	 * @brief Sets whether or not the first graphics queue should have higher priority than other queues.
	 * Very specific feature which is used by async compute samples.
	 * Needs to be called before prepare().
	 * @param enable If true, present queue will have prio 1.0 and other queues have prio 0.5.
	 * Default state is false, where all queues have 0.5 priority.
	 */
	void set_high_priority_graphics_queue_enable(bool enable);

	void set_render_context(std::unique_ptr<RenderContextType> &&render_context);

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
	static void set_viewport_and_scissor(CommandBufferType const &command_buffer, Extent2DType const &extent);

	/// <summary>
	/// PRIVATE INTERFACE
	/// </summary>
  private:
	void        create_render_context_impl(const std::vector<vk::SurfaceFormatKHR> &surface_priority_list);
	void        draw_impl(vkb::core::HPPCommandBuffer &command_buffer, vkb::rendering::HPPRenderTarget &render_target);
	void        draw_renderpass_impl(vkb::core::HPPCommandBuffer &command_buffer, vkb::rendering::HPPRenderTarget &render_target);
	void        render_impl(vkb::core::HPPCommandBuffer &command_buffer);
	static void set_viewport_and_scissor_impl(vkb::core::HPPCommandBuffer const &command_buffer, vk::Extent2D const &extent);

	/**
	 * @brief Get sample-specific device extensions.
	 *
	 * @return Map of device extensions and whether or not they are optional. Default is empty map.
	 */
	std::unordered_map<const char *, bool> const &get_device_extensions() const;

	/**
	 * @brief Get sample-specific instance extensions.
	 *
	 * @return Map of instance extensions and whether or not they are optional. Default is empty map.
	 */
	std::unordered_map<const char *, bool> const &get_instance_extensions() const;

	/**
	 * @brief Get sample-specific layer settings.
	 *
	 * @return Vector of layer settings. Default is empty vector.
	 */
	std::vector<LayerSettingType> const &get_layer_settings() const;

	/// <summary>
	/// PRIVATE MEMBERS
	/// </summary>
  private:
	/**
	 * @brief The Vulkan instance
	 */
	std::unique_ptr<vkb::core::HPPInstance> instance;

	/**
	 * @brief The Vulkan device
	 */
	std::unique_ptr<vkb::core::HPPDevice> device;

	/**
	 * @brief Context used for rendering, it is responsible for managing the frames and their underlying images
	 */
	std::unique_ptr<vkb::rendering::HPPRenderContext> render_context;

	/**
	 * @brief Pipeline used for rendering, it should be set up by the concrete sample
	 */
	std::unique_ptr<vkb::rendering::HPPRenderPipeline> render_pipeline;

	/**
	 * @brief Holds all scene information
	 */
	std::unique_ptr<vkb::scene_graph::HPPScene> scene;

	std::unique_ptr<vkb::HPPGui> gui;

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

	/** @brief Set of instance extensions to be enabled for this example and whether they are optional (must be set in the derived constructor) */
	std::unordered_map<const char *, bool> instance_extensions;

	/** @brief Vector of layer settings to be enabled for this example (must be set in the derived constructor) */
	std::vector<vk::LayerSettingEXT> layer_settings;

	/** @brief The Vulkan API version to request for this sample at instance creation time */
	uint32_t api_version = VK_API_VERSION_1_0;

	/** @brief Whether or not we want a high priority graphics queue. */
	bool high_priority_graphics_queue{false};

	std::unique_ptr<vkb::core::HPPDebugUtils> debug_utils;
};

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

	instance.reset();
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::add_device_extension(const char *extension, bool optional)
{
	device_extensions[extension] = optional;
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::add_instance_extension(const char *extension, bool optional)
{
	instance_extensions[extension] = optional;
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::add_layer_setting(LayerSettingType const &layerSetting)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		layer_settings.push_back(layerSetting);
	}
	else
	{
		layer_settings.push_back(reinterpret_cast<VkLayerSettingEXT const &>(layerSetting));
	}
}

template <vkb::BindingType bindingType>
inline std::unique_ptr<typename VulkanSample<bindingType>::DeviceType> VulkanSample<bindingType>::create_device(PhysicalDeviceType &gpu)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return std::make_unique<vkb::core::HPPDevice>(gpu, surface, std::move(debug_utils), get_device_extensions());
	}
	else
	{
		return std::make_unique<vkb::Device>(gpu,
		                                     static_cast<VkSurfaceKHR>(surface),
		                                     std::unique_ptr<vkb::DebugUtils>(reinterpret_cast<vkb::DebugUtils *>(debug_utils.release())),
		                                     get_device_extensions());
	}
}

template <vkb::BindingType bindingType>
inline std::unique_ptr<typename VulkanSample<bindingType>::InstanceType> VulkanSample<bindingType>::create_instance()
{
	return std::make_unique<InstanceType>(get_name(), get_instance_extensions(), get_validation_layers(), get_layer_settings(), api_version);
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
	vk::PresentModeKHR              present_mode = (window->get_properties().vsync == Window::Vsync::ON) ? vk::PresentModeKHR::eFifo : vk::PresentModeKHR::eMailbox;
	std::vector<vk::PresentModeKHR> present_mode_priority_list{vk::PresentModeKHR::eMailbox, vk::PresentModeKHR::eFifo, vk::PresentModeKHR::eImmediate};
#endif

	render_context =
	    std::make_unique<vkb::rendering::HPPRenderContext>(*device, surface, *window, present_mode, present_mode_priority_list, surface_priority_list);
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::draw(CommandBufferType &command_buffer, RenderTargetType &render_target)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		draw_impl(command_buffer, render_target);
	}
	else
	{
		draw_impl(reinterpret_cast<vkb::core::HPPCommandBuffer &>(command_buffer), reinterpret_cast<vkb::rendering::HPPRenderTarget &>(render_target));
	}
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::draw_impl(vkb::core::HPPCommandBuffer &command_buffer, vkb::rendering::HPPRenderTarget &render_target)
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

	if constexpr (bindingType == BindingType::Cpp)
	{
		draw_renderpass(command_buffer, render_target);
	}
	else
	{
		draw_renderpass(reinterpret_cast<vkb::CommandBuffer &>(command_buffer), reinterpret_cast<vkb::RenderTarget &>(render_target));
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
inline void VulkanSample<bindingType>::draw_renderpass(CommandBufferType &command_buffer, RenderTargetType &render_target)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		draw_renderpass_impl(command_buffer, render_target);
	}
	else
	{
		draw_renderpass_impl(reinterpret_cast<vkb::core::HPPCommandBuffer &>(command_buffer),
		                     reinterpret_cast<vkb::rendering::HPPRenderTarget &>(render_target));
	}
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::draw_renderpass_impl(vkb::core::HPPCommandBuffer &command_buffer, vkb::rendering::HPPRenderTarget &render_target)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		set_viewport_and_scissor(command_buffer, render_target.get_extent());
		render(command_buffer);
	}
	else
	{
		set_viewport_and_scissor(reinterpret_cast<vkb::CommandBuffer const &>(command_buffer),
		                         reinterpret_cast<VkExtent2D const &>(render_target.get_extent()));
		render(reinterpret_cast<vkb::CommandBuffer &>(command_buffer));
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
inline Configuration &VulkanSample<bindingType>::get_configuration()
{
	return configuration;
}

template <vkb::BindingType bindingType>
inline typename VulkanSample<bindingType>::DeviceType const &VulkanSample<bindingType>::get_device() const
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return reinterpret_cast<vkb::core::HPPDevice const &>(*device);
	}
	else
	{
		return *device;
	}
}

template <vkb::BindingType bindingType>
inline typename VulkanSample<bindingType>::DeviceType &VulkanSample<bindingType>::get_device()
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return *device;
	}
	else
	{
		return reinterpret_cast<vkb::Device &>(*device);
	}
}

template <vkb::BindingType bindingType>
inline std::unordered_map<const char *, bool> const &VulkanSample<bindingType>::get_device_extensions() const
{
	return device_extensions;
}

template <vkb::BindingType bindingType>
inline typename VulkanSample<bindingType>::GuiType &VulkanSample<bindingType>::get_gui()
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return *gui;
	}
	else
	{
		return reinterpret_cast<vkb::Gui &>(*gui);
	}
}

template <vkb::BindingType bindingType>
inline typename VulkanSample<bindingType>::GuiType const &VulkanSample<bindingType>::get_gui() const
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return *gui;
	}
	else
	{
		return reinterpret_cast<vkb::Gui const &>(*gui);
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
inline typename VulkanSample<bindingType>::InstanceType &VulkanSample<bindingType>::get_instance()
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return *instance;
	}
	else
	{
		return reinterpret_cast<vkb::Instance &>(*instance);
	}
}

template <vkb::BindingType bindingType>
inline typename VulkanSample<bindingType>::InstanceType const &VulkanSample<bindingType>::get_instance() const
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return *instance;
	}
	else
	{
		return reinterpret_cast<vkb::Instance const &>(*instance);
	}
}

template <vkb::BindingType bindingType>
inline std::unordered_map<const char *, bool> const &VulkanSample<bindingType>::get_instance_extensions() const
{
	return instance_extensions;
}

template <vkb::BindingType bindingType>
inline std::vector<typename VulkanSample<bindingType>::LayerSettingType> const &VulkanSample<bindingType>::get_layer_settings() const
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return layer_settings;
	}
	else
	{
		return reinterpret_cast<std::vector<VkLayerSettingEXT> const &>(layer_settings);
	}
}

template <vkb::BindingType bindingType>
inline typename VulkanSample<bindingType>::RenderContextType const &VulkanSample<bindingType>::get_render_context() const
{
	assert(render_context && "Render context is not valid");
	if constexpr (bindingType == BindingType::Cpp)
	{
		return reinterpret_cast<vkb::rendering::HPPRenderContext const &>(*render_context);
	}
	else
	{
		return *render_context;
	}
}

template <vkb::BindingType bindingType>
inline typename VulkanSample<bindingType>::RenderContextType &VulkanSample<bindingType>::get_render_context()
{
	assert(render_context && "Render context is not valid");
	if constexpr (bindingType == BindingType::Cpp)
	{
		return *render_context;
	}
	else
	{
		return reinterpret_cast<vkb::RenderContext &>(*render_context);
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
inline const std::vector<const char *> VulkanSample<bindingType>::get_validation_layers()
{
	return {};
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
#if TARGET_OS_IPHONE
	static vk::DynamicLoader dl("vulkan.framework/vulkan");
#else
	static vk::DynamicLoader        dl;
#endif
	VULKAN_HPP_DEFAULT_DISPATCHER.init(dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));

	bool headless = window->get_window_mode() == Window::Mode::Headless;

	// for a while we're running on mixed C- and C++-bindings, needing volk for the C-bindings!
	VkResult result = volkInitialize();
	if (result)
	{
		throw VulkanException(result, "Failed to initialize volk.");
	}

	// Creating the vulkan instance
	for (const char *extension_name : window->get_required_surface_extensions())
	{
		add_instance_extension(extension_name);
	}

#ifdef VKB_VULKAN_DEBUG
	{
		std::vector<vk::ExtensionProperties> available_instance_extensions = vk::enumerateInstanceExtensionProperties();
		auto                                 debugExtensionIt =
		    std::find_if(available_instance_extensions.begin(),
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

	if constexpr (bindingType == BindingType::Cpp)
	{
		instance = create_instance();
	}
	else
	{
		instance.reset(reinterpret_cast<vkb::core::HPPInstance *>(create_instance().release()));
	}

	// initialize C++-Bindings default dispatcher, second step
	VULKAN_HPP_DEFAULT_DISPATCHER.init(instance->get_handle());

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
	if constexpr (bindingType == BindingType::Cpp)
	{
		request_gpu_features(gpu);
	}
	else
	{
		request_gpu_features(reinterpret_cast<vkb::PhysicalDevice &>(gpu));
	}

	// Creating vulkan device, specifying the swapchain extension always
	if (!headless || get_instance().is_enabled(VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME))
	{
		add_device_extension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		if (instance_extensions.find(VK_KHR_DISPLAY_EXTENSION_NAME) != instance_extensions.end())
		{
			add_device_extension(VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME, /*optional=*/true);
		}
	}

#ifdef VKB_ENABLE_PORTABILITY
	// VK_KHR_portability_subset must be enabled if present in the implementation (e.g on macOS/iOS with beta extensions enabled)
	add_device_extension(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME, /*optional=*/true);
#endif

#ifdef VKB_VULKAN_DEBUG
	if (!debug_utils)
	{
		std::vector<vk::ExtensionProperties> available_device_extensions = gpu.get_handle().enumerateDeviceExtensionProperties();
		auto                                 debugExtensionIt =
		    std::find_if(available_device_extensions.begin(),
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

	if constexpr (bindingType == BindingType::Cpp)
	{
		device = create_device(gpu);
	}
	else
	{
		device.reset(reinterpret_cast<vkb::core::HPPDevice *>(create_device(reinterpret_cast<vkb::PhysicalDevice &>(gpu)).release()));
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
		gui = std::make_unique<vkb::HPPGui>(*this, window, stats, font_size, explicit_update);
	}
	else
	{
		gui = std::make_unique<vkb::HPPGui>(
		    *reinterpret_cast<VulkanSampleCpp *>(this), window, reinterpret_cast<vkb::stats::HPPStats const *>(stats), font_size, explicit_update);
	}
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::prepare_render_context()
{
	render_context->prepare();
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::render(CommandBufferType &command_buffer)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		render_impl(command_buffer);
	}
	else
	{
		render_impl(reinterpret_cast<vkb::core::HPPCommandBuffer &>(command_buffer));
	}
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::render_impl(vkb::core::HPPCommandBuffer &command_buffer)
{
	if (render_pipeline)
	{
		render_pipeline->draw(command_buffer, render_context->get_active_frame().get_render_target());
	}
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::request_gpu_features(PhysicalDeviceType &gpu)
{
	// To be overridden by sample
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
inline void VulkanSample<bindingType>::set_api_version(uint32_t requested_api_version)
{
	api_version = requested_api_version;
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::set_high_priority_graphics_queue_enable(bool enable)
{
	high_priority_graphics_queue = enable;
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::set_render_context(std::unique_ptr<RenderContextType> &&rc)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		render_context.reset(rc.release());
	}
	else
	{
		render_context.reset(reinterpret_cast<vkb::rendering::HPPRenderContext *>(rc.release()));
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
inline void VulkanSample<bindingType>::set_viewport_and_scissor(CommandBufferType const &command_buffer, Extent2DType const &extent)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		set_viewport_and_scissor_impl(command_buffer, extent);
	}
	else
	{
		set_viewport_and_scissor_impl(reinterpret_cast<vkb::core::HPPCommandBuffer const &>(command_buffer), reinterpret_cast<vk::Extent2D const &>(extent));
	}
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::set_viewport_and_scissor_impl(vkb::core::HPPCommandBuffer const &command_buffer, vk::Extent2D const &extent)
{
	command_buffer.get_handle().setViewport(0, {{0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f}});
	command_buffer.get_handle().setScissor(0, vk::Rect2D({}, extent));
}

template <vkb::BindingType bindingType>
inline void VulkanSample<bindingType>::update(float delta_time)
{
	update_scene(delta_time);

	update_gui(delta_time);

	auto &command_buffer = render_context->begin();

	// Collect the performance data for the sample graphs
	update_stats(delta_time);

	command_buffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	stats->begin_sampling(command_buffer);

	if constexpr (bindingType == BindingType::Cpp)
	{
		draw(command_buffer, render_context->get_active_frame().get_render_target());
	}
	else
	{
		draw(reinterpret_cast<vkb::CommandBuffer &>(command_buffer),
		     reinterpret_cast<vkb::RenderTarget &>(render_context->get_active_frame().get_render_target()));
	}

	stats->end_sampling(command_buffer);
	command_buffer.end();

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

using VulkanSampleC   = VulkanSample<vkb::BindingType::C>;
using VulkanSampleCpp = VulkanSample<vkb::BindingType::Cpp>;

}        // namespace vkb
