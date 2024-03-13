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

#pragma once

#include <imgui.h>

#include "core/hpp_command_buffer.h"
#include "core/hpp_image_view.h"
#include "core/hpp_pipeline_layout.h"
#include "debug_info.h"
#include "drawer.h"
#include "filesystem/legacy.h"
#include "platform/input_events.h"
#include "stats/hpp_stats.h"

namespace vkb
{
template <vkb::BindingType bindingType>
class VulkanSample;

/**
 * @brief Helper structure for fonts loaded from TTF
 */
struct HPPFont
{
	/**
	 * @brief Constructor
	 * @param name The name of the font file that exists within 'assets/fonts' (without extension)
	 * @param size The font size, scaled by DPI
	 */
	HPPFont(const std::string &name, float size) :
	    name{name},
	    data{vkb::fs::read_asset("fonts/" + name + ".ttf")},
	    size{size}
	{
		// Keep ownership of the font data to avoid a double delete
		ImFontConfig font_config{};
		font_config.FontDataOwnedByAtlas = false;

		if (size < 1.0f)
		{
			size = 20.0f;
		}

		ImGuiIO &io = ImGui::GetIO();
		handle      = io.Fonts->AddFontFromMemoryTTF(data.data(), static_cast<int>(data.size()), size, &font_config);
	}

	std::vector<uint8_t> data;
	ImFont	          *handle = nullptr;
	std::string          name;
	float                size = 0.0f;
};

/**
 * @brief Vulkan helper class for Dear ImGui, based on vulkan.hpp
 */
class HPPGui
{
  public:
	/**
	 * @brief Helper class for drawing statistics
	 */
	class StatsView
	{
	  public:
		/**
		 * @brief Constructs a StatsView
		 * @param stats Const pointer to the Stats data object; may be null
		 */
		StatsView(const vkb::stats::HPPStats *stats);

		/**
		 * @brief Resets the max values for the stats
		 *        which do not have a fixed max
		 */
		void reset_max_values();

		/**
		 * @brief Resets the max value for a specific stat
		 */
		void reset_max_value(StatIndex index);

	  public:
		std::map<StatIndex, StatGraphData> graph_map;
		float                              graph_height = 50.0f;
		float                              top_padding  = 1.1f;
	};

  public:
	// The name of the default font file to use
	static const std::string default_font;
	// Used to show/hide the GUI
	static bool visible;

  public:
	/**
	 * @brief Initializes the HPPGui
	 * @param sample A vulkan render context
	 * @param window A Window object from which to draw DPI and content scaling information
	 * @param stats A statistics object (null if no statistics are used)
	 * @param font_size The font size
	 * @param explicit_update If true, update buffers every frame
	 */
	HPPGui(VulkanSample<vkb::BindingType::Cpp> &sample,
	       const vkb::Window                   &window,
	       const vkb::stats::HPPStats          *stats           = nullptr,
	       float                                font_size       = 21.0f,
	       bool                                 explicit_update = false);

	/**
	 * @brief Destroys the HPPGui
	 */
	~HPPGui();

	void prepare(vk::PipelineCache pipeline_cache, vk::RenderPass render_pass, const std::vector<vk::PipelineShaderStageCreateInfo> &shader_stages);

	/**
	 * @brief Handles resizing of the window
	 * @param width New width of the window
	 * @param height New height of the window
	 */
	void resize(uint32_t width, uint32_t height) const;

	/**
	 * @brief Starts a new ImGui frame
	 *        to be called before drawing any window
	 */
	void new_frame() const;

	/**
	 * @brief Updates the HPPGui
	 * @param delta_time Time passed since last update
	 */
	void update(float delta_time);

	bool update_buffers();

	/**
	 * @brief Draws the HPPGui
	 * @param command_buffer Command buffer to register draw-commands
	 */
	void draw(vkb::core::HPPCommandBuffer &command_buffer);

	/**
	 * @brief Draws the HPPGui
	 * @param command_buffer Command buffer to register draw-commands
	 */
	void draw(vk::CommandBuffer command_buffer) const;

	/**
	 * @brief Shows an overlay top window with app info and maybe stats
	 * @param app_name Application name
	 * @param stats Statistics to show (can be null)
	 * @param debug_info Debug info to show (can be null)
	 */
	void show_top_window(const std::string &app_name, const vkb::stats::HPPStats *stats = nullptr, const DebugInfo *debug_info = nullptr);

	/**
	 * @brief Shows the ImGui Demo window
	 */
	void show_demo_window() const;

	/**
	 * @brief Shows an child with app info
	 * @param app_name Application name
	 */
	void show_app_info(const std::string &app_name) const;

	/**
	 * @brief Shows a moveable window with debug information
	 * @param debug_info The object holding the data fields to be displayed
	 * @param position The absolute position to set
	 */
	void show_debug_window(const DebugInfo &debug_info, const ImVec2 &position);

	/**
	 * @brief Shows a child with statistics
	 * @param stats Statistics to show
	 */
	void show_stats(const vkb::stats::HPPStats &stats);

	/**
	 * @brief Shows an options windows, to be filled by the sample,
	 *        which will be positioned at the top
	 * @param body ImGui commands defining the body of the window
	 * @param lines The number of lines of text to draw in the window
	 *        These will help the gui to calculate the height of the window
	 */
	void show_options_window(std::function<void()> body, const uint32_t lines = 3) const;

	void show_simple_window(const std::string &name, uint32_t last_fps, std::function<void()> body) const;

	bool input_event(const InputEvent &input_event);

	/**
	 * @return The stats view
	 */
	const StatsView &get_stats_view() const;

	Drawer &get_drawer();

	HPPFont const &get_font(const std::string &font_name = HPPGui::default_font) const;

	bool is_debug_view_active() const;

  private:
	/**
	 * @brief Updates Vulkan buffers
	 * @param frame Frame to render into
	 */
	void update_buffers(vkb::core::HPPCommandBuffer &command_buffer) const;

  private:
	/**
	 * @brief Helper class for rendering debug statistics in the GUI
	 */
	struct DebugView
	{
		bool     active             = false;
		uint32_t max_fields         = 8;
		float    label_column_width = 0;
		float    scale              = 1.7f;
	};

	struct PushConstBlock
	{
		glm::vec2 scale;
		glm::vec2 translate;
	};

  private:
	/**
	 * @brief Block size of a buffer pool in kilobytes
	 */
	static constexpr uint32_t BUFFER_POOL_BLOCK_SIZE = 256;

	static const double           press_time_ms;
	static const float            overlay_alpha;
	static const ImGuiWindowFlags common_flags;
	static const ImGuiWindowFlags options_flags;
	static const ImGuiWindowFlags info_flags;

  private:
	PushConstBlock                           push_const_block;
	VulkanSample<vkb::BindingType::Cpp>     &sample;
	std::unique_ptr<vkb::core::HPPBuffer>    vertex_buffer;
	std::unique_ptr<vkb::core::HPPBuffer>    index_buffer;
	size_t                                   last_vertex_buffer_size = 0;
	size_t                                   last_index_buffer_size  = 0;
	float                                    content_scale_factor    = 1.0f;        // Scale factor to apply due to a difference between the window and GL pixel sizes
	float                                    dpi_factor              = 1.0f;        // Scale factor to apply to the size of gui elements (expressed in dp)
	bool                                     explicit_update         = false;
	Drawer                                   drawer;
	std::vector<HPPFont>                     fonts;
	std::unique_ptr<vkb::core::HPPImage>     font_image;
	std::unique_ptr<vkb::core::HPPImageView> font_image_view;
	std::unique_ptr<vkb::core::HPPSampler>   sampler;
	vkb::core::HPPPipelineLayout            *pipeline_layout = nullptr;
	StatsView                                stats_view;
	DebugView                                debug_view;
	vk::DescriptorPool                       descriptor_pool       = nullptr;
	vk::DescriptorSetLayout                  descriptor_set_layout = nullptr;
	vk::DescriptorSet                        descriptor_set        = nullptr;
	vk::Pipeline                             pipeline              = nullptr;
	Timer                                    timer;        // Used to measure duration of input events
	bool                                     prev_visible           = true;
	bool                                     two_finger_tap         = false;        // Whether or not the GUI has detected a multi touch gesture
	bool                                     show_graph_file_output = false;
	uint32_t                                 subpass                = 0;
};
}        // namespace vkb
