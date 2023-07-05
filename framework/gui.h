/* Copyright (c) 2018-2023, Arm Limited and Contributors
 * Copyright (c) 2019-2023, Sascha Willems
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

#include <cstdint>
#include <functional>
#include <future>
#include <imgui.h>
#include <imgui_internal.h>
#include <thread>

#include "core/buffer.h"
#include "core/command_buffer.h"
#include "core/sampler.h"
#include "debug_info.h"
#include "drawer.h"
#include "platform/filesystem.h"
#include "platform/input_events.h"
#include "rendering/render_context.h"
#include "stats/stats.h"

namespace vkb
{
class Window;

/**
 * @brief Helper structure for fonts loaded from TTF
 */
struct Font
{
	/**
	 * @brief Constructor
	 * @param name The name of the font file that exists within 'assets/fonts' (without extension)
	 * @param size The font size, scaled by DPI
	 */
	Font(const std::string &name, float size) :
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

	ImFont *handle{nullptr};

	std::string name;

	std::vector<uint8_t> data;

	float size{};
};

class VulkanSample;

/**
 * @brief Vulkan helper class for Dear ImGui
 */
class Gui
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
		StatsView(const Stats *stats);

		/**
		 * @brief Resets the max values for the stats
		 *        which do not have a fixed max
		 */
		void reset_max_values();

		/**
		 * @brief Resets the max value for a specific stat
		 */
		void reset_max_value(const StatIndex index);

		std::map<StatIndex, StatGraphData> graph_map;

		float graph_height{50.0f};

		float top_padding{1.1f};
	};

	/**
	 * @brief Helper class for rendering debug statistics in the GUI
	 */
	class DebugView
	{
	  public:
		bool active{false};

		float scale{1.7f};

		uint32_t max_fields{8};

		float label_column_width{0};
	};

	// The name of the default font file to use
	static const std::string default_font;

	struct PushConstBlock
	{
		glm::vec2 scale;
		glm::vec2 translate;
	} push_const_block;

	/// Used to show/hide the GUI
	static bool visible;

	/**
	 * @brief Initializes the Gui
	 * @param sample A vulkan render context
	 * @param window A Window object from which to draw DPI and content scaling information
	 * @param stats A statistics object (null if no statistics are used)
	 * @param font_size The font size
	 * @param explicit_update If true, update buffers every frame
	 */
	Gui(VulkanSample &sample, const Window &window, const Stats *stats = nullptr,
	    const float font_size = 21.0f, bool explicit_update = false);

	/**
	 * @brief Destroys the Gui
	 */
	~Gui();

	void prepare(const VkPipelineCache pipeline_cache, const VkRenderPass render_pass, const std::vector<VkPipelineShaderStageCreateInfo> &shader_stages);

	/**
	 * @brief Handles resizing of the window
	 * @param width New width of the window
	 * @param height New height of the window
	 */
	void resize(const uint32_t width, const uint32_t height) const;

	/**
	 * @brief Starts a new ImGui frame
	 *        to be called before drawing any window
	 */
	inline void new_frame();

	/**
	 * @brief Updates the Gui
	 * @param delta_time Time passed since last update
	 */
	void update(const float delta_time);

	bool update_buffers();

	/**
	 * @brief Draws the Gui
	 * @param command_buffer Command buffer to register draw-commands
	 */
	void draw(CommandBuffer &command_buffer);

	/**
	 * @brief Draws the Gui
	 * @param command_buffer Command buffer to register draw-commands
	 */
	void draw(VkCommandBuffer command_buffer);

	/**
	 * @brief Shows an overlay top window with app info and maybe stats
	 * @param app_name Application name
	 * @param stats Statistics to show (can be null)
	 * @param debug_info Debug info to show (can be null)
	 */
	void show_top_window(const std::string &app_name, const Stats *stats = nullptr, DebugInfo *debug_info = nullptr);

	/**
	 * @brief Shows the ImGui Demo window
	 */
	void show_demo_window();

	/**
	 * @brief Shows an child with app info
	 * @param app_name Application name
	 */
	void show_app_info(const std::string &app_name);

	/**
	 * @brief Shows a moveable window with debug information
	 * @param debug_info The object holding the data fields to be displayed
	 * @param position The absolute position to set
	 */
	void show_debug_window(DebugInfo &debug_info, const ImVec2 &position);

	/**
	 * @brief Shows a child with statistics
	 * @param stats Statistics to show
	 */
	void show_stats(const Stats &stats);

	/**
	 * @brief Shows an options windows, to be filled by the sample,
	 *        which will be positioned at the top
	 * @param body ImGui commands defining the body of the window
	 * @param lines The number of lines of text to draw in the window
	 *        These will help the gui to calculate the height of the window
	 */
	void show_options_window(std::function<void()> body, const uint32_t lines = 3);

	void show_simple_window(const std::string &name, uint32_t last_fps, std::function<void()> body);

	bool input_event(const InputEvent &input_event);

	/**
	 * @return The stats view
	 */
	StatsView &get_stats_view();

	Drawer &get_drawer();

	Font &get_font(const std::string &font_name = Gui::default_font);

	bool is_debug_view_active() const;

	void set_subpass(const uint32_t subpass);

  private:
	/**
	 * @brief Block size of a buffer pool in kilobytes
	 */
	static constexpr uint32_t BUFFER_POOL_BLOCK_SIZE = 256;

	/**
	 * @brief Updates Vulkan buffers
	 * @param frame Frame to render into
	 */
	void update_buffers(CommandBuffer &command_buffer, RenderFrame &render_frame);

	static const double press_time_ms;

	static const float overlay_alpha;

	static const ImGuiWindowFlags common_flags;

	static const ImGuiWindowFlags options_flags;

	static const ImGuiWindowFlags info_flags;

	VulkanSample &sample;

	std::unique_ptr<core::Buffer> vertex_buffer;

	std::unique_ptr<core::Buffer> index_buffer;

	size_t last_vertex_buffer_size;

	size_t last_index_buffer_size;

	///  Scale factor to apply due to a difference between the window and GL pixel sizes
	float content_scale_factor{1.0f};

	/// Scale factor to apply to the size of gui elements (expressed in dp)
	float dpi_factor{1.0f};

	bool explicit_update{false};

	Drawer drawer;

	std::vector<Font> fonts;

	std::unique_ptr<core::Image>     font_image;
	std::unique_ptr<core::ImageView> font_image_view;

	std::unique_ptr<core::Sampler> sampler{nullptr};

	PipelineLayout *pipeline_layout{nullptr};

	StatsView stats_view;

	DebugView debug_view;

	VkDescriptorPool descriptor_pool{VK_NULL_HANDLE};

	VkDescriptorSetLayout descriptor_set_layout{VK_NULL_HANDLE};

	VkDescriptorSet descriptor_set{VK_NULL_HANDLE};

	VkPipeline pipeline{VK_NULL_HANDLE};

	/// Used to measure duration of input events
	Timer timer;

	bool prev_visible{true};

	/// Whether or not the GUI has detected a multi touch gesture
	bool two_finger_tap = false;

	bool show_graph_file_output = false;

	uint32_t subpass = 0;
};

void Gui::new_frame()
{
	ImGui::NewFrame();
}
}        // namespace vkb
