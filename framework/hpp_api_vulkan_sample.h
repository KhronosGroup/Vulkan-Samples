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

#pragma once

#include <hpp_vulkan_sample.h>

#include <camera.h>
#include <scene_graph/components/hpp_image.h>
#include <scene_graph/components/hpp_sub_mesh.h>

/**
 * @brief A swapchain buffer
 */
struct HPPSwapchainBuffer
{
	vk::Image     image;
	vk::ImageView view;
};

/**
 * @brief A texture wrapper that owns its image data and links it with a sampler
 */
struct HPPTexture
{
	std::unique_ptr<vkb::scene_graph::components::HPPImage> image;
	vk::Sampler                                             sampler;
};

/**
 * @brief The structure of a vertex
 */
struct HPPVertex
{
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 uv;
	glm::vec4 joint0;
	glm::vec4 weight0;
};

/**
 * @brief vulkan.hpp version of the vkb::ApiVulkanSample class
 *
 * See vkb::ApiVulkanSample for documentation
 */
class HPPApiVulkanSample : public vkb::HPPVulkanSample
{
  public:
	HPPApiVulkanSample() = default;

	virtual ~HPPApiVulkanSample();

	virtual bool prepare(vkb::platform::HPPPlatform &platform) override;

	virtual void input_event(const vkb::InputEvent &input_event) override;

	virtual void update(float delta_time) override;

	virtual bool resize(const uint32_t width, const uint32_t height) override;

	virtual void render(float delta_time) = 0;

	enum RenderPassCreateFlags
	{
		ColorAttachmentLoad = 0x00000001
	};

  protected:
	/// Stores the swapchain image buffers
	std::vector<HPPSwapchainBuffer> swapchain_buffers;

	virtual void create_render_context(vkb::Platform &platform) override;
	virtual void prepare_render_context() override;

	// Handle to the device graphics queue that command buffers are submitted to
	vk::Queue queue;

	// Depth buffer format (selected during Vulkan initialization)
	vk::Format depth_format;

	// Command buffer pool
	vk::CommandPool cmd_pool;

	/** @brief Pipeline stages used to wait at for graphics queue submissions */
	vk::PipelineStageFlags submit_pipeline_stages = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	// Contains command buffers and semaphores to be presented to the queue
	vk::SubmitInfo submit_info;

	// Command buffers used for rendering
	std::vector<vk::CommandBuffer> draw_cmd_buffers;

	// Global render pass for frame buffer writes
	vk::RenderPass render_pass;

	// List of available frame buffers (same as number of swap chain images)
	std::vector<vk::Framebuffer> framebuffers;

	// Active frame buffer index
	uint32_t current_buffer = 0;

	// Descriptor set pool
	vk::DescriptorPool descriptor_pool;

	// List of shader modules created (stored for cleanup)
	std::vector<vk::ShaderModule> shader_modules;

	// Pipeline cache object
	vk::PipelineCache pipeline_cache;

	// Synchronization semaphores
	struct
	{
		// Swap chain image presentation
		vk::Semaphore acquired_image_ready;

		// Command buffer submission and execution
		vk::Semaphore render_complete;
	} semaphores;

	// Synchronization fences
	std::vector<vk::Fence> wait_fences;

	/**
   * @brief Populates the swapchain_buffers vector with the image and imageviews
   */
	void create_swapchain_buffers();

	/**
   * @brief Updates the swapchains image usage, if a swapchain exists and recreates all resources based on swapchain images
   * @param image_usage_flags The usage flags the new swapchain images will have
   */
	void update_swapchain_image_usage_flags(std::set<vk::ImageUsageFlagBits> const &image_usage_flags);

	/**
   * @brief Handles changes to the surface, e.g. on resize
   */
	void handle_surface_changes();

	/**
   * @brief Loads in a ktx 2D texture
   * @param file The filename of the texture to load
   */
	HPPTexture load_texture(const std::string &file);

	/**
   * @brief Laods in a ktx 2D texture array
   * @param file The filename of the texture to load
   */
	HPPTexture load_texture_array(const std::string &file);

	/**
   * @brief Loads in a ktx 2D texture cubemap
   * @param file The filename of the texture to load
   */
	HPPTexture load_texture_cubemap(const std::string &file);

	/**
   * @brief Loads in a single model from a GLTF file
   * @param file The filename of the model to load
   * @param index The index of the model to load from the GLTF file (default: 0)
   */
	std::unique_ptr<vkb::scene_graph::components::HPPSubMesh> load_model(const std::string &file, uint32_t index = 0);

	/**
   * @brief Records the necessary drawing commands to a command buffer
   * @param model The model to draw
   * @param command_buffer The command buffer to record to
   */
	void draw_model(std::unique_ptr<vkb::scene_graph::components::HPPSubMesh> &model, vk::CommandBuffer command_buffer);

	/**
   * @brief Synchronously execute a block code within a command buffer, then submit the command buffer and wait for completion.
   * @param f a block of code which is passed a command buffer which is already in the begin state.
   * @param signalSemaphore An optional semaphore to signal when the commands have completed execution.
   */
	void with_command_buffer(const std::function<void(vk::CommandBuffer command_buffer)> &f, vk::Semaphore signalSemaphore = nullptr);

  public:
	/**
   * @brief Called when a view change occurs, can be overriden in derived samples to handle updating uniforms
   */
	virtual void view_changed();

	/**
   * @brief Called after the mouse cursor is moved and before internal events (like camera rotation) is handled
   * @param x The width from the origin
   * @param y The height from the origin
   * @param handled Whether the event was handled
   */
	virtual void mouse_moved(double x, double y, bool &handled);

	/**
   * @brief To be overridden by the derived class. Records the relevant commands to the rendering command buffers
   *        Called when the framebuffers need to be rebuilt
   */
	virtual void build_command_buffers() = 0;

	/**
   * @brief Creates the fences for rendering
   */
	void create_synchronization_primitives();

	/**
   * @brief Creates a new (graphics) command pool object storing command buffers
   */
	void create_command_pool();

	/**
   * @brief Setup default depth and stencil views
   */
	virtual void setup_depth_stencil();

	/**
   * @brief Create framebuffers for all requested swap chain images
   *        Can be overriden in derived class to setup a custom framebuffer (e.g. for MSAA)
   */
	virtual void setup_framebuffer();

	/**
   * @brief Setup a default render pass
   *        Can be overriden in derived class to setup a custom render pass (e.g. for MSAA)
   */
	virtual void setup_render_pass();

	/**
   * @brief Update flags for the default render pass and recreate it
   * @param flags Optional flags for render pass creation
   */
	void update_render_pass_flags(RenderPassCreateFlags flags = {});

	/**
   * @brief Check if command buffers are valid (!= VK_NULL_HANDLE)
   */
	bool check_command_buffers();

	/**
   * @brief Create command buffers for drawing commands
   */
	void create_command_buffers();

	/**
   * @brief Destroy all command buffers, may be necessary during runtime if options are toggled
   */
	void destroy_command_buffers();

	/**
   * @brief Create a cache pool for rendering pipelines
   */
	void create_pipeline_cache();

	/**
   * @brief Load a SPIR-V shader
   * @param file The file location of the shader relative to the shaders folder
   * @param stage The shader stage
   */
	vk::PipelineShaderStageCreateInfo load_shader(const std::string &file, vk::ShaderStageFlagBits stage);

	/**
   * @brief Updates the overlay
   * @param delta_time The time taken since the last frame
   */
	void update_overlay(float delta_time);

	/**
   * @brief If the gui is enabled, then record the drawing commands to a command buffer
   * @param command_buffer A valid command buffer that is ready to be recorded to
   */
	void draw_ui(const vk::CommandBuffer command_buffer);

	/**
   * @brief Prepare the frame for workload submission, acquires the next image from the swap chain and
   *        sets the default wait and signal semaphores
   */
	void prepare_frame();

	/**
   * @brief Submit the frames' workload
   */
	void submit_frame();

	/**
   * @brief Called when the UI overlay is updating, can be used to add custom elements to the overlay
   * @param drawer The drawer from the gui to draw certain elements
   */
	virtual void on_update_ui_overlay(vkb::Drawer &drawer);

  private:
	/** brief Indicates that the view (position, rotation) has changed and buffers containing camera matrices need to be updated */
	bool view_updated = false;
	// Destination dimensions for resizing the window
	vk::Extent2D dest_extent;
	bool         resizing = false;

	void handle_mouse_move(int32_t x, int32_t y);

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	/// The debug report callback
	vk::DebugReportCallbackEXT debug_report_callback;
#endif

  public:
	bool         prepared = false;
	vk::Extent2D extent;

	/** @brief Example settings that can be changed e.g. by command line arguments */
	struct
	{
		/** @brief Set to true if fullscreen mode has been requested via command line */
		bool fullscreen = false;
		/** @brief Set to true if v-sync will be forced for the swapchain */
		bool vsync = false;
	} settings;

	vk::ClearColorValue default_clear_color = std::array<float, 4>({{0.025f, 0.025f, 0.025f, 1.0f}});

	float zoom = 0;

	// Defines a frame rate independent timer value clamped from -1.0...1.0
	// For use in animations, rotations, etc.
	float timer = 0.0f;
	// Multiplier for speeding up (or slowing down) the global timer
	float timer_speed = 0.0025f;

	bool paused = false;

	// Use to adjust mouse rotation speed
	float rotation_speed = 1.0f;
	// Use to adjust mouse zoom speed
	float zoom_speed = 1.0f;

	vkb::Camera camera;

	glm::vec3 rotation   = glm::vec3();
	glm::vec3 camera_pos = glm::vec3();
	glm::vec2 mouse_pos;

	std::string title = "HPP Vulkan API Example";
	std::string name  = "HPPAPIVulkanExample";

	struct
	{
		vk::Image        image;
		vk::DeviceMemory mem;
		vk::ImageView    view;
	} depth_stencil;

	struct
	{
		glm::vec2 axis_left  = glm::vec2(0.0f);
		glm::vec2 axis_right = glm::vec2(0.0f);
	} game_pad_state;

	struct
	{
		bool left   = false;
		bool right  = false;
		bool middle = false;
	} mouse_buttons;

	// true if application has focused, false if moved to background
	bool focused = false;
	struct TouchPos
	{
		int32_t x;
		int32_t y;
	} touch_pos;
	bool    touch_down    = false;
	double  touch_timer   = 0.0;
	int64_t last_tap_time = 0;
};
