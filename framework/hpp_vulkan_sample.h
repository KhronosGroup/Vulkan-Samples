/* Copyright (c) 2021-2023, NVIDIA CORPORATION. All rights reserved.
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

#include <platform/application.h>

#include <core/hpp_command_buffer.h>
#include <core/hpp_device.h>
#include <core/hpp_physical_device.h>
#include <rendering/hpp_render_pipeline.h>
#include <rendering/hpp_render_target.h>
#include <stats/hpp_stats.h>
#include <vulkan/vulkan.hpp>

namespace vkb
{
class HPPGui;

/**
 * @brief vulkan.hpp version of the vkb::VulkanSample class
 *
 * See vkb::VulkanSample for documentation
 */
class HPPVulkanSample : public vkb::Application
{
  public:
	HPPVulkanSample() = default;

	~HPPVulkanSample() override;

	/**
	 * @brief Additional sample initialization
	 */
	bool prepare(const vkb::ApplicationOptions &options) override;

	/**
	 * @brief Main loop sample events
	 */
	void update(float delta_time) override;

	bool resize(uint32_t width, uint32_t height) override;

	void input_event(const InputEvent &input_event) override;

	/**
	 * @brief Returns the drawer object for the sample
	 */
	vkb::Drawer *get_drawer() override;

	void finish() override;

	/**
	 * @brief Loads the scene
	 *
	 * @param path The path of the glTF file
	 */
	void load_scene(const std::string &path);

	vk::SurfaceKHR get_surface() const;

	vkb::core::HPPInstance const &get_instance() const;

	std::unique_ptr<vkb::core::HPPDevice> const &get_device() const;

	inline bool has_render_context() const
	{
		return render_context != nullptr;
	}

	vkb::rendering::HPPRenderContext &get_render_context();

	void set_render_pipeline(vkb::rendering::HPPRenderPipeline &&render_pipeline);

	vkb::rendering::HPPRenderPipeline const &get_render_pipeline() const;

	Configuration &get_configuration();

	sg::Scene &get_scene();

	bool has_scene() const;

  protected:
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
	std::unique_ptr<sg::Scene> scene;

	std::unique_ptr<vkb::HPPGui> gui;

	std::unique_ptr<vkb::stats::HPPStats> stats;

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
	virtual void draw(vkb::core::HPPCommandBuffer &command_buffer, vkb::rendering::HPPRenderTarget const &render_target) const;

	/**
	 * @brief Starts the render pass, executes the render pipeline, and then ends the render pass
	 * @param command_buffer The command buffer to record the commands to
	 * @param render_target The render target that is being drawn to
	 */
	virtual void draw_renderpass(vkb::core::HPPCommandBuffer &command_buffer, vkb::rendering::HPPRenderTarget const &render_target) const;

	/**
	 * @brief Triggers the render pipeline, it can be overridden by samples to specialize their rendering logic
	 * @param command_buffer The command buffer to record the commands to
	 */
	virtual void render(vkb::core::HPPCommandBuffer &command_buffer) const;

	/**
	 * @brief Get additional sample-specific instance layers.
	 *
	 * @return Vector of additional instance layers. Default is empty vector.
	 */
	virtual std::vector<const char *> const &get_validation_layers() const;

	/**
	 * @brief Get sample-specific instance extensions.
	 *
	 * @return Map of instance extensions and whether or not they are optional. Default is empty map.
	 */
	std::unordered_map<const char *, bool> const &get_instance_extensions() const;

	/**
	 * @brief Get sample-specific device extensions.
	 *
	 * @return Map of device extensions and whether or not they are optional. Default is empty map.
	 */
	std::unordered_map<const char *, bool> const &get_device_extensions() const;

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
	 * @brief Set the Vulkan API version to request at instance creation time
	 */
	void set_api_version(uint32_t requested_api_version);

	/**
	 * @brief Request features from the gpu based on what is supported
	 */
	virtual void request_gpu_features(vkb::core::HPPPhysicalDevice &gpu);

	/**
	 * @brief Override this to customise the creation of the render_context
	 */
	virtual void create_render_context();

	/**
	 * @brief Override this to customise the creation of the swapchain and render_context
	 */
	virtual void prepare_render_context();

	/**
	 * @brief Resets the stats view max values for high demanding configs
	 *        Should be overridden by the samples since they
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
	 * @brief Returns stored shaders by sample
	 */
	const std::map<ShaderSourceLanguage, std::vector<std::pair<vk::ShaderStageFlagBits, std::string>>> &get_available_shaders() const;

	/**
	 * @brief Stores a list of shaders for the active sample, used by plugins to dynamically change the shader
	 * @param shader_language The shader language for which the shader list will be provided
	 * @param list_of_shaders The shader list, where paths and shader types are provided
	 */
	void store_shaders(const vkb::ShaderSourceLanguage &shader_language, const std::vector<std::pair<vk::ShaderStageFlagBits, std::string>> &list_of_shaders);

	/**
	 * @brief Set viewport and scissor state in command buffer for a given extent
	 */
	static void set_viewport_and_scissor(vkb::core::HPPCommandBuffer const &command_buffer, vk::Extent2D const &extent);

	static constexpr float STATS_VIEW_RESET_TIME{10.0f};        // 10 seconds

	/**
	 * @brief The Vulkan surface
	 */
	vk::SurfaceKHR surface;

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

	void create_render_context(const std::vector<vk::SurfaceFormatKHR> &surface_priority_list);

  private:
	/** @brief Set of device extensions to be enabled for this example and whether they are optional (must be set in the derived constructor) */
	std::unordered_map<const char *, bool> device_extensions;

	/** @brief Set of instance extensions to be enabled for this example and whether they are optional (must be set in the derived constructor) */
	std::unordered_map<const char *, bool> instance_extensions;

	/** @brief The Vulkan API version to request for this sample at instance creation time */
	uint32_t api_version = VK_API_VERSION_1_0;

	/** @brief Whether or not we want a high priority graphics queue. */
	bool high_priority_graphics_queue{false};
};
}        // namespace vkb
