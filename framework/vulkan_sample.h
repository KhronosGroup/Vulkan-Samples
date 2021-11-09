/* Copyright (c) 2019-2021, Arm Limited and Contributors
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

#include "common/error.h"
#include "common/utils.h"
#include "common/vk_common.h"
#include "core/instance.h"
#include "gui.h"
#include "platform/application.h"
#include "rendering/render_context.h"
#include "rendering/render_pipeline.h"
#include "scene_graph/node.h"
#include "scene_graph/scene.h"
#include "scene_graph/scripts/node_animation.h"
#include "stats/stats.h"

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

class VulkanSample : public Application
{
  public:
	VulkanSample() = default;

	virtual ~VulkanSample();

	/**
	 * @brief Additional sample initialization
	 */
	virtual bool prepare(Platform &platform) override;

	/**
	 * @brief Main loop sample events
	 */
	virtual void update(float delta_time) override;

	virtual void resize(const uint32_t width, const uint32_t height) override;

	virtual void input_event(const InputEvent &input_event) override;

	virtual void finish() override;

	/** 
	 * @brief Loads the scene
	 *
	 * @param path The path of the glTF file
	 */
	void load_scene(const std::string &path);

	VkSurfaceKHR get_surface();

	Device &get_device();

	RenderContext &get_render_context();

	void set_render_pipeline(RenderPipeline &&render_pipeline);

	RenderPipeline &get_render_pipeline();

	Configuration &get_configuration();

	sg::Scene &get_scene();

	bool has_scene();

  protected:
	/**
	 * @brief The Vulkan instance
	 */
	std::unique_ptr<Instance> instance{nullptr};

	/**
	 * @brief The Vulkan device
	 */
	std::unique_ptr<Device> device{nullptr};

	/**
	 * @brief Context used for rendering, it is responsible for managing the frames and their underlying images
	 */
	std::unique_ptr<RenderContext> render_context{nullptr};

	/**
	 * @brief Pipeline used for rendering, it should be set up by the concrete sample
	 */
	std::unique_ptr<RenderPipeline> render_pipeline{nullptr};

	/**
	 * @brief Holds all scene information
	 */
	std::unique_ptr<sg::Scene> scene{nullptr};

	std::unique_ptr<Gui> gui{nullptr};

	std::unique_ptr<Stats> stats{nullptr};

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
	 * @brief Update GUI
	 * @param delta_time
	 */
	void update_gui(float delta_time);

	/**
	 * @brief Prepares the render target and draws to it, calling draw_renderpass
	 * @param command_buffer The command buffer to record the commands to
	 * @param render_target The render target that is being drawn to
	 */
	virtual void draw(CommandBuffer &command_buffer, RenderTarget &render_target);

	/**
	 * @brief Starts the render pass, executes the render pipeline, and then ends the render pass
	 * @param command_buffer The command buffer to record the commands to
	 * @param render_target The render target that is being drawn to
	 */
	virtual void draw_renderpass(CommandBuffer &command_buffer, RenderTarget &render_target);

	/**
	 * @brief Triggers the render pipeline, it can be overriden by samples to specialize their rendering logic
	 * @param command_buffer The command buffer to record the commands to
	 */
	virtual void render(CommandBuffer &command_buffer);

	/**
	 * @brief Get additional sample-specific instance layers.
	 *
	 * @return Vector of additional instance layers. Default is empty vector.
	 */
	virtual const std::vector<const char *> get_validation_layers();

	/**
	 * @brief Get sample-specific instance extensions.
	 *
	 * @return Map of instance extensions and wether or not they are optional. Default is empty map.
	 */
	const std::unordered_map<const char *, bool> get_instance_extensions();

	/**
	 * @brief Get sample-specific device extensions.
	 *
	 * @return Map of device extensions and whether or not they are optional. Default is empty map.
	 */
	const std::unordered_map<const char *, bool> get_device_extensions();

	/**
	 * @brief Add a sample-specific device extension
	 * @param extension The extension name
	 * @param optional (Optional) Wether the extension is optional
	 */
	void add_device_extension(const char *extension, bool optional = false);

	/**
	 * @brief Add a sample-specific instance extension
	 * @param extension The extension name
	 * @param optional (Optional) Wether the extension is optional
	 */
	void add_instance_extension(const char *extension, bool optional = false);

	/**
	 * @brief Set the Vulkan API version to request at instance creation time
	 */
	void set_api_version(uint32_t requested_api_version);

	/**
	 * @brief Request features from the gpu based on what is supported
	 */
	virtual void request_gpu_features(PhysicalDevice &gpu);

	/** 
	 * @brief Override this to customise the creation of the render_context
	 */
	virtual void create_render_context(Platform &platform);

	/** 
	 * @brief Override this to customise the creation of the swapchain and render_context
	 */
	virtual void prepare_render_context();

	/**
	 * @brief Resets the stats view max values for high demanding configs
	 *        Should be overriden by the samples since they
	 *        know which configuration is resource demanding
	 */
	virtual void reset_stats_view(){};

	/**
	 * @brief Samples should override this function to draw their interface
	 */
	virtual void draw_gui();

	/**
	 * @brief Updates the debug window, samples can override this to insert their own data elements
	 */
	virtual void update_debug_window();

	/**
	 * @brief Set viewport and scissor state in command buffer for a given extent
	 */
	void set_viewport_and_scissor(vkb::CommandBuffer &command_buffer, const VkExtent2D &extent) const;

	static constexpr float STATS_VIEW_RESET_TIME{10.0f};        // 10 seconds

	/**
	 * @brief The Vulkan surface
	 */
	VkSurfaceKHR surface{VK_NULL_HANDLE};

	/**
	 * @brief The configuration of the sample
	 */
	Configuration configuration{};

	/**
	 * @brief Sets whether or not the first graphics queue should have higher priority than other queues.
	 * Very specific feature which is used by async compute samples.
	 * Needs to be called before prepare().
	 * @param enable If true, present queue will have prio 1.0 and other queues have prio 0.5.
	 * Default state is false, where all queues have 0.5 priority.
	 */
	void set_high_priority_graphics_queue_enable(bool enable)
	{
		high_priority_graphics_queue = enable;
	}

  private:
	/** @brief Set of device extensions to be enabled for this example and wether they are optional (must be set in the derived constructor) */
	std::unordered_map<const char *, bool> device_extensions;

	/** @brief Set of instance extensions to be enabled for this example and whether they are optional (must be set in the derived constructor) */
	std::unordered_map<const char *, bool> instance_extensions;

	/** @brief The Vulkan API version to request for this sample at instance creation time */
	uint32_t api_version = VK_API_VERSION_1_0;

	/** @brief Whether or not we want a high priority graphics queue. */
	bool high_priority_graphics_queue{false};
};
}        // namespace vkb
