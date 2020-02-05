/* Copyright (c) 2018-2020, Arm Limited and Contributors
 * Copyright (c) 2019-2020, Sascha Willems
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
#include "platform/filesystem.h"
#include "platform/input_events.h"
#include "rendering/render_context.h"
#include "stats.h"

namespace vkb
{
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

		ImGuiIO &io = ImGui::GetIO();
		handle      = io.Fonts->AddFontFromMemoryTTF(data.data(), static_cast<int>(data.size()), size, &font_config);
	}

	ImFont *handle{nullptr};

	std::string name;

	std::vector<uint8_t> data;

	float size{};
};

/**
 * @brief Responsible for drawing new elements into the gui
 */
class Drawer
{
  public:
	Drawer() = default;

	/** 
	 * @brief Clears the dirty bit set
	 */
	void clear();

	/** 
	 * @brief Returns true if the drawer has been updated
	 */
	bool is_dirty();

	/**
	 * @brief May be used to force drawer update
	 */
	void set_dirty(bool dirty);

	/**
	 * @brief Adds a collapsable header item to the gui
	 * @param caption The text to display
	 * @returns True if adding item was successful
	 */
	bool header(const char *caption);

	/**
	 * @brief Adds a checkbox to the gui
	 * @param caption The text to display
	 * @param value The boolean value to map the checkbox to
	 * @returns True if adding item was successful
	 */
	bool checkbox(const char *caption, bool *value);

	/**
	 * @brief Adds a checkbox to the gui
	 * @param caption The text to display
	 * @param value The integer value to map the checkbox to
	 * @returns True if adding item was successful
	 */
	bool checkbox(const char *caption, int32_t *value);

	/**
	 * @brief Adds a number input field to the gui
	 * @param caption The text to display
	 * @param value The value to map to
	 * @param step The step increment
	 * @param precision The precision
	 * @returns True if adding item was successful
	 */
	bool input_float(const char *caption, float *value, float step, uint32_t precision);

	/**
	 * @brief Adds a slide bar to the gui for floating points to the gui
	 * @param caption The text to display
	 * @param value The value to map to
	 * @param min The minimum value
	 * @param max The maximum value
	 * @returns True if adding item was successful
	 */
	bool slider_float(const char *caption, float *value, float min, float max);

	/**
	 * @brief Adds a slide bar to the gui for integers to the gui
	 * @param caption The text to display
	 * @param value The value to map to
	 * @param min The minimum value
	 * @param max The maximum value
	 * @returns True if adding item was successful
	 */
	bool slider_int(const char *caption, int32_t *value, int32_t min, int32_t max);

	/**
	 * @brief Adds a multiple choice drop box to the gui
	 * @param caption The text to display
	 * @param itemindex The item index to display
	 * @param items The items to display in the box
	 * @returns True if adding item was successful
	 */
	bool combo_box(const char *caption, int32_t *itemindex, std::vector<std::string> items);

	/**
	 * @brief Adds a clickable button to the gui
	 * @param caption The text to display
	 * @returns True if adding item was successful
	 */
	bool button(const char *caption);

	/**
	 * @brief Adds a label to the gui
	 * @param formatstr The format string
	 */
	void text(const char *formatstr, ...);

  private:
	bool dirty{false};
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
		 * @brief Per-statistic graph data
		 */
		class GraphData
		{
		  public:
			/**
			 * @brief Constructs data for the graph
			 * @param name Name of the Stat
			 * @param format Format of the label
			 * @param scale_factor Any scaling to apply to the data
			 * @param has_fixed_max Whether the data should have a fixed max value
			 * @param max_value The maximum value to use
			 */
			GraphData(const std::string &name,
			          const std::string &format,
			          float              scale_factor  = 1.0f,
			          bool               has_fixed_max = false,
			          float              max_value     = 0.0f);

			std::string name;
			std::string format;
			float       scale_factor;
			bool        has_fixed_max;
			float       max_value;
		};

		/**
		 * @brief Resets the max values for the stats
		 *        which do not have a fixed max
		 */
		void reset_max_values();

		/**
		 * @brief Resets the max value for a specific stat
		 */
		void reset_max_value(const StatIndex index);

		/// Per-statistic max values
		std::map<StatIndex, GraphData>
		    graph_map{
		        {StatIndex::frame_times,
		         {/* name = */ "Frame Times",
		          /* format = */ "{:3.1f} ms",
		          /* scale_factor = */ 1000.0f}},
		        {StatIndex::cpu_cycles,
		         {/* name = */ "CPU Cycles",
		          /* format = */ "{:4.1f} M/s",
		          /* scale_factor = */ float(1e-6)}},
		        {StatIndex::cpu_instructions,
		         {/* name = */ "CPU Instructions",
		          /* format = */ "{:4.1f} M/s",
		          /* scale_factor = */ float(1e-6)}},
		        {StatIndex::cache_miss_ratio,
		         {/* name = */ "Cache Miss Ratio",
		          /* format = */ "{:3.1f}%",
		          /* scale_factor = */ 100.0f,
		          /* has_fixed_max = */ true,
		          /* max_value = */ 100.0f}},
		        {StatIndex::branch_miss_ratio,
		         {/* name = */ "Branch Miss Ratio",
		          /* format = */ "{:3.1f}%",
		          /* scale_factor = */ 100.0f,
		          /* has_fixed_max = */ true,
		          /* max_value = */ 100.0f}},
		        {StatIndex::gpu_cycles,
		         {/* name = */ "GPU Cycles",
		          /* format = */ "{:4.1f} M/s",
		          /* scale_factor = */ float(1e-6)}},
		        {StatIndex::vertex_compute_cycles,
		         {/* name = */ "Vertex Compute Cycles",
		          /* format = */ "{:4.1f} M/s",
		          /* scale_factor = */ float(1e-6)}},
		        {StatIndex::tiles,
		         {/* name = */ "Tiles",
		          /* format = */ "{:4.1f} M/s",
		          /* scale_factor = */ float(1e-6)}},
		        {StatIndex::fragment_cycles,
		         {/* name = */ "Fragment Cycles",
		          /* format = */ "{:4.1f} M/s",
		          /* scale_factor = */ float(1e-6)}},
		        {StatIndex::fragment_jobs,
		         {/* name = */ "Fragment Jobs",
		          /* format = */ "{:4.0f}/s"}},
		        {StatIndex::tex_cycles,
		         {/* name = */ "Shader Texture Cycles",
		          /* format = */ "{:4.0f} k/s",
		          /* scale_factor = */ float(1e-3)}},
		        {StatIndex::l2_ext_reads,
		         {/* name = */ "External Reads",
		          /* format = */ "{:4.1f} M/s",
		          /* scale_factor = */ float(1e-6)}},
		        {StatIndex::l2_ext_writes,
		         {/* name = */ "External Writes",
		          /* format = */ "{:4.1f} M/s",
		          /* scale_factor = */ float(1e-6)}},
		        {StatIndex::l2_ext_read_stalls,
		         {/* name = */ "External Read Stalls",
		          /* format = */ "{:4.1f} M/s",
		          /* scale_factor = */ float(1e-6)}},
		        {StatIndex::l2_ext_write_stalls,
		         {/* name = */ "External Write Stalls",
		          /* format = */ "{:4.1f} M/s",
		          /* scale_factor = */ float(1e-6)}},
		        {StatIndex::l2_ext_read_bytes,
		         {/* name = */ "External Read Bytes",
		          /* format = */ "{:4.1f} MiB/s",
		          /* scale_factor = */ 1.0f / (1024.0f * 1024.0f)}},
		        {StatIndex::l2_ext_write_bytes,
		         {/* name = */ "External Write Bytes",
		          /* format = */ "{:4.1f} MiB/s",
		          /* scale_factor = */ 1.0f / (1024.0f * 1024.0f)}}};

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

	/**
	 * @brief Initializes the Gui
	 * @param sample A vulkan render context
	 * @param dpi_factor The dpi scale factor
	 * @param font_size The font size
	 * @param explicit_update If true, update buffers every frame
	 */
	Gui(VulkanSample &sample, const float dpi_factor = 1.0, const float font_size = 21.0f, bool explicit_update = false);

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

	/// Used to show/hide the GUI
	bool visible{true};

	bool prev_visible{true};

	/// Whether or not the GUI has detected a multi touch gesture
	bool two_finger_tap = false;

	bool show_graph_file_output = false;
};

void Gui::new_frame()
{
	ImGui::NewFrame();
}
}        // namespace vkb
