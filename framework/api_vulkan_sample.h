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

#pragma once

#ifdef _WIN32
#	pragma comment(linker, "/subsystem:windows")
#	include <ShellScalingAPI.h>
#	include <fcntl.h>
#	include <io.h>
#	include <windows.h>
#endif

#include <chrono>
#include <iostream>
#include <random>
#include <sys/stat.h>

#include "camera.h"
#include "common/vk_common.h"
#include "common/vk_initializers.h"
#include "core/buffer.h"
#include "core/swapchain.h"
#include "gui.h"
#include "platform/platform.h"
#include "rendering/render_context.h"
#include "scene_graph/components/image.h"
#include "scene_graph/components/sampler.h"
#include "scene_graph/components/texture.h"
#include "vulkan_sample.h"

/**
 * @brief A swapchain buffer
 */
struct SwapchainBuffer
{
	VkImage     image;
	VkImageView view;
};

/**
 * @brief A texture wrapper that owns its image data and links it with a sampler
 */
struct Texture
{
	std::unique_ptr<vkb::sg::Image> image;
	VkSampler                       sampler;
};

/**
 * @brief The structure of a vertex
 */
struct Vertex
{
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 uv;
	glm::vec4 joint0;
	glm::vec4 weight0;
	glm::vec3 color;
};

/**
 * @brief The structure of a vertex for storage buffer
 * Simplified to position and normal for easier alignment
 */
struct AlignedVertex
{
	glm::vec4 pos;
	glm::vec4 normal;
};

/**
 * @brief The structure of a meshlet for mesh shader
 */
struct Meshlet
{
	uint32_t vertices[64];
	uint32_t indices[126];
	uint32_t vertex_count;
	uint32_t index_count;
};

/**
 * @brief Sascha Willems base class for use in his ported samples into the framework
 *
 * See vkb::VulkanSample for documentation
 */
class ApiVulkanSample : public vkb::VulkanSample
{
  public:
	ApiVulkanSample() = default;

	virtual ~ApiVulkanSample();

	virtual bool prepare(const vkb::ApplicationOptions &options) override;

	virtual void input_event(const vkb::InputEvent &input_event) override;

	virtual void update(float delta_time) override;

	virtual bool resize(const uint32_t width, const uint32_t height) override;

	virtual void render(float delta_time) = 0;

	vkb::Device &get_device();

	enum RenderPassCreateFlags
	{
		ColorAttachmentLoad = 0x00000001
	};

  protected:
	/// Stores the swapchain image buffers
	std::vector<SwapchainBuffer> swapchain_buffers;

	virtual void create_render_context() override;
	virtual void prepare_render_context() override;

	// Handle to the device graphics queue that command buffers are submitted to
	VkQueue queue;

	// Depth buffer format (selected during Vulkan initialization)
	VkFormat depth_format;

	// Command buffer pool
	VkCommandPool cmd_pool;

	/** @brief Pipeline stages used to wait at for graphics queue submissions */
	VkPipelineStageFlags submit_pipeline_stages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	// Contains command buffers and semaphores to be presented to the queue
	VkSubmitInfo submit_info;

	// Command buffers used for rendering
	std::vector<VkCommandBuffer> draw_cmd_buffers;

	// Global render pass for frame buffer writes
	VkRenderPass render_pass;

	// List of available frame buffers (same as number of swap chain images)
	std::vector<VkFramebuffer> framebuffers;

	// Active frame buffer index
	uint32_t current_buffer = 0;

	// Descriptor set pool
	VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;

	// List of shader modules created (stored for cleanup)
	std::vector<VkShaderModule> shader_modules;

	// Pipeline cache object
	VkPipelineCache pipeline_cache;

	// Synchronization semaphores
	struct
	{
		// Swap chain image presentation
		VkSemaphore acquired_image_ready;

		// Command buffer submission and execution
		VkSemaphore render_complete;
	} semaphores;

	// Synchronization fences
	std::vector<VkFence> wait_fences;

	/**
	 * @brief Populates the swapchain_buffers vector with the image and imageviews
	 */
	void create_swapchain_buffers();

	/**
	 * @brief Updates the swapchains image usage, if a swapchain exists and recreates all resources based on swapchain images
	 * @param image_usage_flags The usage flags the new swapchain images will have
	 */
	void update_swapchain_image_usage_flags(std::set<VkImageUsageFlagBits> image_usage_flags);

	/**
	 * @brief Handles changes to the surface, e.g. on resize
	 */
	void handle_surface_changes();

	/**
	 * @brief Creates a buffer descriptor
	 * @param buffer The buffer from which to create the descriptor from
	 * @param size The size of the descriptor (default: VK_WHOLE_SIZE)
	 * @param offset The offset of the descriptor (default: 0)
	 */
	VkDescriptorBufferInfo create_descriptor(vkb::core::Buffer &buffer, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

	/**
	 * @brief Creates an image descriptor
	 * @param texture The texture from which to create the descriptor from
	 * @param descriptor_type The type of image descriptor (default: VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
	 */
	VkDescriptorImageInfo create_descriptor(Texture &texture, VkDescriptorType descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	/**
	 * @brief Loads in a ktx 2D texture
	 * @param file The filename of the texture to load
	 * @param content_type The type of content in the image file
	 */
	Texture load_texture(const std::string &file, vkb::sg::Image::ContentType content_type);

	/**
	 * @brief Loads in a ktx 2D texture array
	 * @param file The filename of the texture to load
	 * @param content_type The type of content in the image file
	 */
	Texture load_texture_array(const std::string &file, vkb::sg::Image::ContentType content_type);

	/**
	 * @brief Loads in a ktx 2D texture cubemap
	 * @param file The filename of the texture to load
	 * @param content_type The type of content in the image file
	 */
	Texture load_texture_cubemap(const std::string &file, vkb::sg::Image::ContentType content_type);

	/**
	 * @brief Loads in a single model from a GLTF file
	 * @param file The filename of the model to load
	 * @param index The index of the model to load from the GLTF file (default: 0)
	 * @param storage_buffer Set true to store model in SSBO
	 */
	std::unique_ptr<vkb::sg::SubMesh> load_model(const std::string &file, uint32_t index = 0, bool storage_buffer = false);

	/**
	 * @brief Records the necessary drawing commands to a command buffer
	 * @param model The model to draw
	 * @param command_buffer The command buffer to record to
	 * @param instance_count The number of instances (default: 1)
	 */
	void draw_model(std::unique_ptr<vkb::sg::SubMesh> &model, VkCommandBuffer command_buffer, uint32_t instance_count = 1);

	/**
	 * @brief Synchronously execute a block code within a command buffer, then submit the command buffer and wait for completion.
	 * @param f a block of code which is passed a command buffer which is already in the begin state.
	 * @param signalSemaphore An optional semaphore to signal when the commands have completed execution.
	 */
	void with_command_buffer(const std::function<void(VkCommandBuffer command_buffer)> &f, VkSemaphore signalSemaphore = VK_NULL_HANDLE);

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
	 * @brief Rebuild the command buffers by first resetting the corresponding command pool and then building the command buffers.
	 */
	void rebuild_command_buffers();

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
	void update_render_pass_flags(uint32_t flags = 0);

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
	 * @brief Recreate the current command buffer draw_cmd_buffer[current_buffer]
	 */
	void recreate_current_command_buffer();

	/**
	 * @brief Create a cache pool for rendering pipelines
	 */
	void create_pipeline_cache();

	/**
	 * @brief Load a SPIR-V shader
	 * @param file The file location of the shader relative to the shaders folder
	 * @param stage The shader stage
	 */
	VkPipelineShaderStageCreateInfo load_shader(const std::string &file, VkShaderStageFlagBits stage);

	/**
	 * @brief Updates the overlay
	 * @param delta_time The time taken since the last frame
	 */
	void update_overlay(float delta_time);

	/**
	 * @brief If the gui is enabled, then record the drawing commands to a command buffer
	 * @param command_buffer A valid command buffer that is ready to be recorded to
	 */
	void draw_ui(const VkCommandBuffer command_buffer);

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

	/**
	 * @brief Initializes the UI. Can be overridden to customize the way it is displayed.
	 */
	virtual void prepare_gui();

  private:
	/** brief Indicates that the view (position, rotation) has changed and buffers containing camera matrices need to be updated */
	bool view_updated = false;
	// Destination dimensions for resizing the window
	uint32_t dest_width;
	uint32_t dest_height;
	bool     resizing = false;

	void handle_mouse_move(int32_t x, int32_t y);

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	/// The debug report callback
	VkDebugReportCallbackEXT debug_report_callback{VK_NULL_HANDLE};
#endif

  public:
	bool     prepared = false;
	uint32_t width    = 1280;
	uint32_t height   = 720;

	VkClearColorValue default_clear_color = {{0.002f, 0.002f, 0.002f, 1.0f}};

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

	std::string title = "Vulkan Example";
	std::string name  = "vulkanExample";

	struct
	{
		VkImage        image;
		VkDeviceMemory mem;
		VkImageView    view;
	} depth_stencil;

	struct
	{
		bool left   = false;
		bool right  = false;
		bool middle = false;
	} mouse_buttons;

	struct TouchPos
	{
		int32_t x;
		int32_t y;
	} touch_pos;
	bool   touch_down  = false;
	double touch_timer = 0.0;
};
