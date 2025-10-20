/* Copyright (c) 2018-2025, Arm Limited and Contributors
 * Copyright (c) 2019-2025, Sascha Willems
 * Copyright (c) 2025, NVIDIA CORPORATION. All rights reserved.
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

#include "common/vk_common.h"
#include "core/command_buffer.h"
#include "core/hpp_debug.h"
#include "core/hpp_sampler.h"
#include "debug_info.h"
#include "drawer.h"
#include "filesystem/legacy.h"
#include "platform/input_events.h"
#include "platform/window.h"
#include "rendering/hpp_pipeline_state.h"
#include "stats/hpp_stats.h"
#include "stats/stats.h"
#include <glm/glm.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <numeric>

#if defined(PLATFORM__MACOS)
#	include <TargetConditionals.h>
#endif

namespace vkb
{

namespace
{
inline void reset_graph_max_value(StatGraphData &graph_data)
{
	// If it does not have a fixed max
	if (!graph_data.has_fixed_max)
	{
		// Reset it
		graph_data.max_value = 0.0f;
	}
}
}        // namespace

/**
 * @brief Helper structure for fonts loaded from TTF
 */
struct Font
{
	/**
	 * @brief Constructor
	 * @param name_ The name of the font file that exists within 'assets/fonts' (without extension)
	 * @param size_ The font size, scaled by DPI
	 */
	Font(const std::string &name_, float size_) :
	    name{name_}, data{vkb::fs::read_asset("fonts/" + name + ".ttf")}, size{size_}
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

	ImFont              *handle = nullptr;
	std::string          name;
	std::vector<uint8_t> data;
	float                size = {};
};

/**
 * @brief Vulkan helper class for Dear ImGui
 */
template <vkb::BindingType bindingType>
class Gui
{
  public:
	static inline bool visible = true;        // Used to show/hide the GUI

	using CommandBufferType                 = typename std::conditional<bindingType == BindingType::Cpp, vk::CommandBuffer, VkCommandBuffer>::type;
	using DescriptorSetType                 = typename std::conditional<bindingType == BindingType::Cpp, vk::DescriptorSet, VkDescriptorSet>::type;
	using PipelineType                      = typename std::conditional<bindingType == BindingType::Cpp, vk::Pipeline, VkPipeline>::type;
	using PipelineCacheType                 = typename std::conditional<bindingType == BindingType::Cpp, vk::PipelineCache, VkPipelineCache>::type;
	using PipelineLayoutType                = typename std::conditional<bindingType == BindingType::Cpp, vk::PipelineLayout, VkPipelineLayout>::type;
	using PipelineShaderStageCreateInfoType = typename std::conditional<bindingType == BindingType::Cpp, vk::PipelineShaderStageCreateInfo, VkPipelineShaderStageCreateInfo>::type;
	using RenderPassType                    = typename std::conditional<bindingType == BindingType::Cpp, vk::RenderPass, VkRenderPass>::type;
	using StatsType                         = typename std::conditional<bindingType == BindingType::Cpp, vkb::stats::HPPStats, vkb::Stats>::type;

  public:
	/**
	 * @brief Helper struct for rendering debug statistics in the GUI
	 */
	struct DebugView
	{
		bool     active             = false;
		float    scale              = 1.7f;
		uint32_t max_fields         = 8;
		float    label_column_width = 0;
	};

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
		StatsView(const StatsType *stats);

		float get_graph_height() const;

		StatGraphData const &get_stat_graph_data(StatIndex const &index) const;

		float get_top_padding() const;

		/**
		 * @brief Resets the max value for a specific stat
		 */
		void reset_max_value(const StatIndex index);

		/**
		 * @brief Resets the max values for the stats which do not have a fixed max
		 */
		void reset_max_values();

	  private:
		std::map<StatIndex, StatGraphData> graph_map;
		float                              graph_height = 50.0f;
		float                              top_padding  = 1.1f;
	};

  public:
	/**
	 * @brief Initializes the Gui
	 * @param render_context A vulkan render context
	 * @param window A Window object from which to draw DPI and content scaling information
	 * @param stats A statistics object (null if no statistics are used)
	 * @param font_size The font size
	 * @param explicit_update If true, update buffers every frame
	 */
	Gui(vkb::rendering::RenderContext<bindingType> &render_context,
	    Window const                               &window,
	    StatsType const                            *stats           = nullptr,
	    float                                       font_size       = 21.0f,
	    bool                                        explicit_update = false);

	/**
	 * @brief Destroys the Gui
	 */
	~Gui();

  public:
	/**
	 * @brief Draws the Gui
	 * @param command_buffer Command buffer to register draw-commands
	 */
	void draw(CommandBufferType command_buffer);

	/**
	 * @brief Draws the Gui using an external pipeline
	 * @param command_buffer Command buffer to register draw-commands
	 * @param pipeline Pipeline to bind to perform draw-commands
	 * @param pipeline_layout PipelineLayout for given pieline
	 * @param descriptor_set DescriptorSet to bind to perform draw-commands
	 */
	void draw(CommandBufferType command_buffer, PipelineType pipeline, PipelineLayoutType pipeline_layout, DescriptorSetType descriptor_set);

	/**
	 * @brief Draws the Gui
	 * @param command_buffer Command buffer to register draw-commands
	 */
	void draw(vkb::core::CommandBuffer<bindingType> &command_buffer);

	Drawer &get_drawer();

	vk::ImageView get_font_image_view() const;

	vk::Sampler get_sampler() const;

	/**
	 * @return The stats view
	 */
	StatsView &get_stats_view();

	bool input_event(const InputEvent &input_event);

	bool is_debug_view_active() const;

	/**
	 * @brief Starts a new ImGui frame
	 *        to be called before drawing any window
	 */
	void new_frame();

	void prepare(PipelineCacheType pipeline_cache, RenderPassType render_pass, std::vector<PipelineShaderStageCreateInfoType> const &shader_stages);

	/**
	 * @brief Handles resizing of the window
	 * @param width New width of the window
	 * @param height New height of the window
	 */
	void resize(const uint32_t width, const uint32_t height) const;

	void set_subpass(const uint32_t subpass);

	/**
	 * @brief Shows an options windows, to be filled by the sample,
	 *        which will be positioned at the top
	 * @param body ImGui commands defining the body of the window
	 * @param lines The number of lines of text to draw in the window
	 *        These will help the gui to calculate the height of the window
	 */
	void show_options_window(std::function<void()> body, const uint32_t lines = 3);

	void show_simple_window(const std::string &name, uint32_t last_fps, std::function<void()> body);

	/**
	 * @brief Shows an overlay top window with app info and maybe stats
	 * @param app_name Application name
	 * @param stats Statistics to show (can be null)
	 * @param debug_info Debug info to show (can be null)
	 */
	void show_top_window(const std::string &app_name, const StatsType *stats = nullptr, DebugInfo *debug_info = nullptr);

	/**
	 * @brief Updates the Gui
	 * @param delta_time Time passed since last update
	 */
	void update(const float delta_time);

	bool update_buffers();

	/**
	 * @brief Shows a child with statistics
	 * @param stats Statistics to show
	 */
	void show_stats(const StatsType &stats);

  private:
	static constexpr char const      *default_font        = "Roboto-Regular";            // The name of the default font file to use
	static constexpr char const      *default_window_font = "RobotoMono-Regular";        // The name of the default window font file to use
	static constexpr ImGuiWindowFlags common_flags        = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar |
	                                                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
	                                                 ImGuiWindowFlags_NoFocusOnAppearing;
	static constexpr float  overlay_alpha = 0.3f;
	static constexpr double press_time_ms = 200.0;

  private:
	void draw_impl(vk::CommandBuffer command_buffer, vk::Pipeline pipeline, vk::PipelineLayout pipeline_layout, vk::DescriptorSet descriptor_set);
	void draw_impl(vkb::core::CommandBufferCpp &command_buffer);
	void prepare_impl(vk::PipelineCache pipeline_cache, vk::RenderPass render_pass, std::vector<vk::PipelineShaderStageCreateInfo> const &shader_stages);

	Font &get_font(const std::string &font_name = Gui::default_font);

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
	 * @brief Updates Vulkan buffers
	 * @param command_buffer Command buffer to draw into
	 * @return Vertex buffer allocation
	 */
	BufferAllocationCpp update_buffers(vkb::core::CommandBufferCpp &command_buffer);

	void upload_draw_data(const ImDrawData *draw_data, uint8_t *vertex_data, uint8_t *index_data);

  private:
	float                                    content_scale_factor = 1.0f;        //  Scale factor to apply due to a difference between the window and GL pixel sizes
	DebugView                                debug_view;
	vk::DescriptorPool                       descriptor_pool;
	vk::DescriptorSet                        descriptor_set;
	vk::DescriptorSetLayout                  descriptor_set_layout;
	float                                    dpi_factor = 1.0f;        // Scale factor to apply to the size of gui elements (expressed in dp)
	Drawer                                   drawer;
	bool                                     explicit_update = false;
	std::vector<Font>                        fonts;
	std::unique_ptr<vkb::core::HPPImage>     font_image;
	std::unique_ptr<vkb::core::HPPImageView> font_image_view;
	std::unique_ptr<vkb::core::BufferCpp>    index_buffer;
	vk::Pipeline                             pipeline;
	vkb::core::HPPPipelineLayout            *pipeline_layout = nullptr;
	bool                                     prev_visible    = true;
	vkb::rendering::RenderContextCpp        &render_context;
	std::unique_ptr<vkb::core::HPPSampler>   sampler;
	StatsView                                stats_view;
	uint32_t                                 subpass = 0;
	Timer                                    timer;                         // Used to measure duration of input events
	bool                                     two_finger_tap = false;        // Whether or not the GUI has detected a multi touch gesture
	std::unique_ptr<vkb::core::BufferCpp>    vertex_buffer;
};

using GuiC   = Gui<vkb::BindingType::C>;
using GuiCpp = Gui<vkb::BindingType::Cpp>;

template <vkb::BindingType bindingType>
inline Gui<bindingType>::Gui(
    vkb::rendering::RenderContext<bindingType> &render_context_, Window const &window, StatsType const *stats, float font_size, bool explicit_update) :
    render_context{render_context_},
    content_scale_factor{window.get_content_scale_factor()},
    dpi_factor{window.get_dpi_factor() * content_scale_factor},
    explicit_update{explicit_update},
    stats_view(stats)
{
	ImGui::CreateContext();

	ImGuiStyle &style = ImGui::GetStyle();

	// Color scheme
	style.Colors[ImGuiCol_WindowBg]         = ImVec4(0.005f, 0.005f, 0.005f, 0.94f);
	style.Colors[ImGuiCol_TitleBg]          = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
	style.Colors[ImGuiCol_TitleBgActive]    = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
	style.Colors[ImGuiCol_MenuBarBg]        = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_Header]           = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_HeaderActive]     = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_HeaderHovered]    = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_FrameBg]          = ImVec4(0.0f, 0.0f, 0.0f, 0.8f);
	style.Colors[ImGuiCol_CheckMark]        = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_SliderGrab]       = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
	style.Colors[ImGuiCol_FrameBgHovered]   = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
	style.Colors[ImGuiCol_FrameBgActive]    = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
	style.Colors[ImGuiCol_Button]           = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_ButtonHovered]    = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
	style.Colors[ImGuiCol_ButtonActive]     = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);

	// Borderless window
	style.WindowBorderSize = 0.0f;

	// Global scale
	style.ScaleAllSizes(dpi_factor);

	// Dimensions
	ImGuiIO &io                = ImGui::GetIO();
	auto     extent            = render_context.get_surface_extent();
	io.DisplaySize.x           = static_cast<float>(extent.width);
	io.DisplaySize.y           = static_cast<float>(extent.height);
	io.FontGlobalScale         = 1.0f;
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

	// Enable keyboard navigation
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.KeyMap[ImGuiKey_Space]      = static_cast<int>(KeyCode::Space);
	io.KeyMap[ImGuiKey_Enter]      = static_cast<int>(KeyCode::Enter);
	io.KeyMap[ImGuiKey_LeftArrow]  = static_cast<int>(KeyCode::Left);
	io.KeyMap[ImGuiKey_RightArrow] = static_cast<int>(KeyCode::Right);
	io.KeyMap[ImGuiKey_UpArrow]    = static_cast<int>(KeyCode::Up);
	io.KeyMap[ImGuiKey_DownArrow]  = static_cast<int>(KeyCode::Down);
	io.KeyMap[ImGuiKey_Tab]        = static_cast<int>(KeyCode::Tab);
	io.KeyMap[ImGuiKey_Escape]     = static_cast<int>(KeyCode::Backspace);

	// Default font
	fonts.emplace_back(default_font, font_size * dpi_factor);

	// Debug window font
	fonts.emplace_back(default_window_font, (font_size / 2) * dpi_factor);

	// Create font texture
	unsigned char *font_data;
	int            tex_width, tex_height;
	io.Fonts->GetTexDataAsRGBA32(&font_data, &tex_width, &tex_height);
	size_t upload_size = tex_width * tex_height * 4 * sizeof(char);

	auto &device = render_context.get_device();

	// Create target image for copy
	vk::Extent3D font_extent{to_u32(tex_width), to_u32(tex_height), 1u};

	font_image = std::make_unique<vkb::core::HPPImage>(device, font_extent, vk::Format::eR8G8B8A8Unorm,
	                                                   vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
	                                                   VMA_MEMORY_USAGE_GPU_ONLY);
	font_image->set_debug_name("GUI font image");

	font_image_view = std::make_unique<vkb::core::HPPImageView>(*font_image, vk::ImageViewType::e2D);
	font_image_view->set_debug_name("View on GUI font image");

	// Upload font data into the vulkan image memory
	{
		vkb::core::BufferCpp stage_buffer = vkb::core::BufferCpp::create_staging_buffer(device, upload_size, font_data);

		auto command_buffer = device.get_command_pool().request_command_buffer();

		vkb::HPPFencePool fence_pool{device};

		// Begin recording
		command_buffer->begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit, 0);

		{
			// Prepare for transfer
			vkb::common::HPPImageMemoryBarrier memory_barrier = {.src_stage_mask  = vk::PipelineStageFlagBits::eHost,
			                                                     .dst_stage_mask  = vk::PipelineStageFlagBits::eTransfer,
			                                                     .dst_access_mask = vk::AccessFlagBits::eTransferWrite,
			                                                     .old_layout      = vk::ImageLayout::eUndefined,
			                                                     .new_layout      = vk::ImageLayout::eTransferDstOptimal};

			command_buffer->image_memory_barrier(*font_image_view, memory_barrier);
		}

		// Copy
		vk::BufferImageCopy buffer_copy_region = {.imageSubresource = {.aspectMask = font_image_view->get_subresource_range().aspectMask,
		                                                               .layerCount = font_image_view->get_subresource_range().layerCount},
		                                          .imageExtent      = font_image->get_extent()};

		command_buffer->copy_buffer_to_image(stage_buffer, *font_image, {buffer_copy_region});

		{
			// Prepare for fragment shader
			vkb::common::HPPImageMemoryBarrier memory_barrier = {.src_stage_mask  = vk::PipelineStageFlagBits::eTransfer,
			                                                     .dst_stage_mask  = vk::PipelineStageFlagBits::eFragmentShader,
			                                                     .src_access_mask = vk::AccessFlagBits::eTransferWrite,
			                                                     .dst_access_mask = vk::AccessFlagBits::eShaderRead,
			                                                     .old_layout      = vk::ImageLayout::eTransferDstOptimal,
			                                                     .new_layout      = vk::ImageLayout::eShaderReadOnlyOptimal};

			command_buffer->image_memory_barrier(*font_image_view, memory_barrier);
		}

		// End recording
		command_buffer->end();

		auto &queue = device.get_queue_by_flags(vk::QueueFlagBits::eGraphics, 0);

		queue.submit(*command_buffer, device.get_fence_pool().request_fence());

		// Wait for the command buffer to finish its work before destroying the staging buffer
		device.get_fence_pool().wait();
		device.get_fence_pool().reset();
		device.get_command_pool().reset_pool();
	}

	// Calculate valid filter
	vk::Filter filter = vk::Filter::eLinear;
	vkb::common::make_filters_valid(device.get_gpu().get_handle(), font_image->get_format(), &filter);

	// Load shaders
	vkb::core::HPPShaderSource vert_shader("imgui.vert.spv");
	vkb::core::HPPShaderSource frag_shader("imgui.frag.spv");

	std::vector<vkb::core::HPPShaderModule *> shader_modules;
	shader_modules.push_back(&device.get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eVertex, vert_shader, {}));
	shader_modules.push_back(&device.get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eFragment, frag_shader, {}));

	pipeline_layout = &device.get_resource_cache().request_pipeline_layout(shader_modules);

	// Create texture sampler
	vk::SamplerCreateInfo sampler_info = {.magFilter    = filter,
	                                      .minFilter    = filter,
	                                      .mipmapMode   = vk::SamplerMipmapMode::eNearest,
	                                      .addressModeU = vk::SamplerAddressMode::eClampToEdge,
	                                      .addressModeV = vk::SamplerAddressMode::eClampToEdge,
	                                      .addressModeW = vk::SamplerAddressMode::eClampToEdge,
	                                      .borderColor  = vk::BorderColor::eFloatOpaqueWhite};

	sampler = std::make_unique<vkb::core::HPPSampler>(device, sampler_info);
	sampler->set_debug_name("GUI sampler");

	if (explicit_update)
	{
		vertex_buffer =
		    std::make_unique<vkb::core::BufferCpp>(render_context.get_device(), 1, vk::BufferUsageFlagBits::eVertexBuffer, VMA_MEMORY_USAGE_GPU_TO_CPU);
		vertex_buffer->set_debug_name("GUI vertex buffer");

		index_buffer = std::make_unique<vkb::core::BufferCpp>(render_context.get_device(), 1, vk::BufferUsageFlagBits::eIndexBuffer, VMA_MEMORY_USAGE_GPU_TO_CPU);
		index_buffer->set_debug_name("GUI index buffer");
	}
}

template <vkb::BindingType bindingType>
inline Gui<bindingType>::~Gui()
{
	vk::Device device = render_context.get_device().get_handle();
	device.destroyDescriptorPool(descriptor_pool);
	device.destroyDescriptorSetLayout(descriptor_set_layout);
	device.destroyPipeline(pipeline);

	ImGui::DestroyContext();
}

template <vkb::BindingType bindingType>
inline void Gui<bindingType>::draw(CommandBufferType command_buffer)
{
	draw(command_buffer, pipeline, pipeline_layout->get_handle(), descriptor_set);
}

template <vkb::BindingType bindingType>
inline void
    Gui<bindingType>::draw(CommandBufferType command_buffer, PipelineType pipeline, PipelineLayoutType pipeline_layout, DescriptorSetType descriptor_set)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		draw_impl(command_buffer, pipeline, pipeline_layout, descriptor_set);
	}
	else
	{
		draw_impl(static_cast<vk::CommandBuffer>(command_buffer),
		          static_cast<vk::Pipeline>(pipeline),
		          static_cast<vk::PipelineLayout>(pipeline_layout),
		          static_cast<vk::DescriptorSet>(descriptor_set));
	}
}

template <vkb::BindingType bindingType>
inline void
    Gui<bindingType>::draw_impl(vk::CommandBuffer command_buffer, vk::Pipeline pipeline, vk::PipelineLayout pipeline_layout, vk::DescriptorSet descriptor_set)
{
	if (!visible)
	{
		return;
	}

	ImDrawData *draw_data = ImGui::GetDrawData();
	if ((!draw_data) || (draw_data->CmdListsCount == 0))
	{
		return;
	}

	command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
	command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, descriptor_set, {});

	// Push constants
	auto &io             = ImGui::GetIO();
	auto  push_transform = glm::mat4(1.0f);
	push_transform       = glm::translate(push_transform, glm::vec3(-1.0f, -1.0f, 0.0f));
	push_transform       = glm::scale(push_transform, glm::vec3(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y, 0.0f));
	command_buffer.pushConstants(pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4), &push_transform);

	vk::DeviceSize vertex_offsets[1]    = {0};
	vk::Buffer     vertex_buffer_handle = vertex_buffer->get_handle();
	command_buffer.bindVertexBuffers(0, vertex_buffer_handle, vertex_offsets);

	command_buffer.bindIndexBuffer(index_buffer->get_handle(), 0, vk::IndexType::eUint16);

	int32_t vertex_offset = 0;
	int32_t index_offset  = 0;
	for (auto const *cmd_list : draw_data->CmdLists)
	{
		for (auto const &cmd : cmd_list->CmdBuffer)
		{
			vk::Rect2D scissor_rect;
			scissor_rect.offset.x      = std::max(static_cast<int32_t>(cmd.ClipRect.x), 0);
			scissor_rect.offset.y      = std::max(static_cast<int32_t>(cmd.ClipRect.y), 0);
			scissor_rect.extent.width  = static_cast<uint32_t>(cmd.ClipRect.z - cmd.ClipRect.x);
			scissor_rect.extent.height = static_cast<uint32_t>(cmd.ClipRect.w - cmd.ClipRect.y);

			command_buffer.setScissor(0, scissor_rect);
			command_buffer.drawIndexed(cmd.ElemCount, 1, index_offset, vertex_offset, 0);
			index_offset += cmd.ElemCount;
		}
#if defined(PLATFORM__MACOS) && TARGET_OS_IOS && TARGET_OS_SIMULATOR
		// iOS Simulator does not support vkCmdDrawIndexed() with vertex_offset > 0, so rebind vertex buffer instead
		vertex_offsets[0] += cmd_list->VtxBuffer.Size * sizeof(ImDrawVert);
		command_buffer.bindVertexBuffers(0, vertex_buffer_handle, vertex_offsets);
#else
		vertex_offset += cmd_list->VtxBuffer.Size;
#endif
	}
}

template <vkb::BindingType bindingType>
inline void Gui<bindingType>::draw(vkb::core::CommandBuffer<bindingType> &command_buffer)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		draw_impl(command_buffer);
	}
	else
	{
		draw_impl(reinterpret_cast<vkb::core::CommandBufferCpp &>(command_buffer));
	}
}

template <vkb::BindingType bindingType>
inline void Gui<bindingType>::draw_impl(vkb::core::CommandBufferCpp &command_buffer)
{
	if (!visible)
	{
		return;
	}

	// Render commands
	ImDrawData *draw_data = ImGui::GetDrawData();
	if (!draw_data || draw_data->CmdListsCount == 0)
	{
		return;
	}

	vkb::core::HPPScopedDebugLabel debug_label{command_buffer, "GUI"};

	// Vertex input state
	vk::VertexInputBindingDescription vertex_input_binding = {.stride = sizeof(ImDrawVert)};

	// Location 0: Position
	vk::VertexInputAttributeDescription pos_attr = {.location = 0, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(ImDrawVert, pos)};

	// Location 1: UV
	vk::VertexInputAttributeDescription uv_attr = {.location = 1, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(ImDrawVert, uv)};

	// Location 2: Color
	vk::VertexInputAttributeDescription col_attr = {.location = 2, .format = vk::Format::eR8G8B8A8Unorm, .offset = offsetof(ImDrawVert, col)};

	vkb::rendering::HPPVertexInputState vertex_input_state = {.bindings = {vertex_input_binding}, .attributes = {pos_attr, uv_attr, col_attr}};

	command_buffer.set_vertex_input_state(vertex_input_state);

	// Blend state
	vkb::rendering::HPPColorBlendAttachmentState color_attachment = {.blend_enable           = true,
	                                                                 .src_color_blend_factor = vk::BlendFactor::eSrcAlpha,
	                                                                 .dst_color_blend_factor = vk::BlendFactor::eOneMinusSrcAlpha,
	                                                                 .src_alpha_blend_factor = vk::BlendFactor::eOneMinusSrcAlpha,
	                                                                 .color_write_mask       = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
	                                                                                     vk::ColorComponentFlagBits::eB};

	vkb::rendering::HPPColorBlendState blend_state{.attachments = {color_attachment}};

	command_buffer.set_color_blend_state(blend_state);

	vkb::rendering::HPPRasterizationState rasterization_state = {.cull_mode = vk::CullModeFlagBits::eNone};
	command_buffer.set_rasterization_state(rasterization_state);

	vkb::rendering::HPPDepthStencilState depth_state = {.depth_test_enable = false, .depth_write_enable = false};
	command_buffer.set_depth_stencil_state(depth_state);

	// Bind pipeline layout
	command_buffer.bind_pipeline_layout(*pipeline_layout);

	command_buffer.bind_image(*font_image_view, *sampler, 0, 0, 0);

	// Pre-rotation
	auto push_transform = glm::mat4(1.0f);

	if (render_context.has_swapchain())
	{
		auto transform = render_context.get_swapchain().get_transform();

		glm::vec3 rotation_axis = glm::vec3(0.0f, 0.0f, 1.0f);
		if (transform & vk::SurfaceTransformFlagBitsKHR::eRotate90)
		{
			push_transform = glm::rotate(push_transform, glm::radians(90.0f), rotation_axis);
		}
		else if (transform & vk::SurfaceTransformFlagBitsKHR::eRotate270)
		{
			push_transform = glm::rotate(push_transform, glm::radians(270.0f), rotation_axis);
		}
		else if (transform & vk::SurfaceTransformFlagBitsKHR::eRotate180)
		{
			push_transform = glm::rotate(push_transform, glm::radians(180.0f), rotation_axis);
		}
	}

	// GUI coordinate space to screen space
	auto &io       = ImGui::GetIO();
	push_transform = glm::translate(push_transform, glm::vec3(-1.0f, -1.0f, 0.0f));
	push_transform = glm::scale(push_transform, glm::vec3(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y, 0.0f));

	// Push constants
	command_buffer.push_constants(push_transform);

	std::vector<std::reference_wrapper<const vkb::core::BufferCpp>> vertex_buffers;
	std::vector<vk::DeviceSize>                                     vertex_offsets;

	// If a render context is used, then use the frames buffer pools to allocate GUI vertex/index data from
	if (!explicit_update)
	{
		// Save vertex buffer allocation in case we need to rebind with vertex_offset, e.g. for iOS Simulator
		auto vertex_allocation = update_buffers(command_buffer);
		if (!vertex_allocation.empty())
		{
			vertex_buffers.push_back(vertex_allocation.get_buffer());
			vertex_offsets.push_back(vertex_allocation.get_offset());
		}
	}
	else
	{
		vertex_buffers.push_back(*vertex_buffer);
		vertex_offsets.push_back(0);
		command_buffer.bind_vertex_buffers(0, vertex_buffers, vertex_offsets);
		command_buffer.bind_index_buffer(*index_buffer, 0, vk::IndexType::eUint16);
	}

	int32_t  vertex_offset = 0;
	uint32_t index_offset  = 0;
	for (auto const *cmd_list : draw_data->CmdLists)
	{
		for (auto const &cmd : cmd_list->CmdBuffer)
		{
			vk::Rect2D scissor_rect;
			scissor_rect.offset.x      = std::max(static_cast<int32_t>(cmd.ClipRect.x), 0);
			scissor_rect.offset.y      = std::max(static_cast<int32_t>(cmd.ClipRect.y), 0);
			scissor_rect.extent.width  = static_cast<uint32_t>(cmd.ClipRect.z - cmd.ClipRect.x);
			scissor_rect.extent.height = static_cast<uint32_t>(cmd.ClipRect.w - cmd.ClipRect.y);

			// Adjust for pre-rotation if necessary
			if (render_context.has_swapchain())
			{
				auto transform = render_context.get_swapchain().get_transform();
				if (transform & vk::SurfaceTransformFlagBitsKHR::eRotate90)
				{
					scissor_rect.offset.x      = static_cast<uint32_t>(io.DisplaySize.y - cmd.ClipRect.w);
					scissor_rect.offset.y      = static_cast<uint32_t>(cmd.ClipRect.x);
					scissor_rect.extent.width  = static_cast<uint32_t>(cmd.ClipRect.w - cmd.ClipRect.y);
					scissor_rect.extent.height = static_cast<uint32_t>(cmd.ClipRect.z - cmd.ClipRect.x);
				}
				else if (transform & vk::SurfaceTransformFlagBitsKHR::eRotate180)
				{
					scissor_rect.offset.x      = static_cast<uint32_t>(io.DisplaySize.x - cmd.ClipRect.z);
					scissor_rect.offset.y      = static_cast<uint32_t>(io.DisplaySize.y - cmd.ClipRect.w);
					scissor_rect.extent.width  = static_cast<uint32_t>(cmd.ClipRect.z - cmd.ClipRect.x);
					scissor_rect.extent.height = static_cast<uint32_t>(cmd.ClipRect.w - cmd.ClipRect.y);
				}
				else if (transform & vk::SurfaceTransformFlagBitsKHR::eRotate270)
				{
					scissor_rect.offset.x      = static_cast<uint32_t>(cmd.ClipRect.y);
					scissor_rect.offset.y      = static_cast<uint32_t>(io.DisplaySize.x - cmd.ClipRect.z);
					scissor_rect.extent.width  = static_cast<uint32_t>(cmd.ClipRect.w - cmd.ClipRect.y);
					scissor_rect.extent.height = static_cast<uint32_t>(cmd.ClipRect.z - cmd.ClipRect.x);
				}
			}

			command_buffer.set_scissor(0, {scissor_rect});
			command_buffer.draw_indexed(cmd.ElemCount, 1, index_offset, vertex_offset, 0);
			index_offset += cmd.ElemCount;
		}
#if defined(PLATFORM__MACOS) && TARGET_OS_IOS && TARGET_OS_SIMULATOR
		// iOS Simulator does not support vkCmdDrawIndexed() with vertex_offset > 0, so rebind vertex buffer instead
		if (!vertex_offsets.empty())
		{
			vertex_offsets.back() += cmd_list->VtxBuffer.Size * sizeof(ImDrawVert);
			command_buffer.bind_vertex_buffers(0, vertex_buffers, vertex_offsets);
		}
#else
		vertex_offset += cmd_list->VtxBuffer.Size;
#endif
	}
}

template <vkb::BindingType bindingType>
inline Drawer &Gui<bindingType>::get_drawer()
{
	return drawer;
}

template <vkb::BindingType bindingType>
inline Font &Gui<bindingType>::get_font(const std::string &font_name)
{
	assert(!fonts.empty() && "No fonts exist");

	auto it = std::ranges::find_if(fonts, [&font_name](Font &font) { return font.name == font_name; });

	if (it != fonts.end())
	{
		return *it;
	}
	else
	{
		LOGW("Couldn't find font with name {}", font_name);
		return *fonts.begin();
	}
}

template <vkb::BindingType bindingType>
inline vk::ImageView Gui<bindingType>::get_font_image_view() const
{
	return font_image_view->get_handle();
}

template <vkb::BindingType bindingType>
inline vk::Sampler Gui<bindingType>::get_sampler() const
{
	return sampler->get_handle();
}

template <vkb::BindingType bindingType>
inline typename Gui<bindingType>::StatsView &Gui<bindingType>::get_stats_view()
{
	return stats_view;
}

template <vkb::BindingType bindingType>
inline bool Gui<bindingType>::input_event(const InputEvent &input_event)
{
	auto &io                 = ImGui::GetIO();
	auto  capture_move_event = false;

	if (input_event.get_source() == EventSource::Keyboard)
	{
		const auto &key_event = static_cast<const KeyInputEvent &>(input_event);

		if (key_event.get_action() == KeyAction::Down)
		{
			io.KeysDown[static_cast<int>(key_event.get_code())] = true;
		}
		else if (key_event.get_action() == KeyAction::Up)
		{
			io.KeysDown[static_cast<int>(key_event.get_code())] = false;
		}
	}
	else if (input_event.get_source() == EventSource::Mouse)
	{
		const auto &mouse_button = static_cast<const MouseButtonInputEvent &>(input_event);

		io.MousePos = ImVec2{mouse_button.get_pos_x() * content_scale_factor,
		                     mouse_button.get_pos_y() * content_scale_factor};

		auto button_id = static_cast<int>(mouse_button.get_button());

		if (mouse_button.get_action() == MouseAction::Down)
		{
			io.MouseDown[button_id] = true;
		}
		else if (mouse_button.get_action() == MouseAction::Up)
		{
			io.MouseDown[button_id] = false;
		}
		else if (mouse_button.get_action() == MouseAction::Move)
		{
			capture_move_event = io.WantCaptureMouse;
		}
	}
	else if (input_event.get_source() == EventSource::Touchscreen)
	{
		const auto &touch_event = static_cast<const TouchInputEvent &>(input_event);

		io.MousePos = ImVec2{touch_event.get_pos_x(), touch_event.get_pos_y()};

		if (touch_event.get_action() == TouchAction::Down)
		{
			io.MouseDown[touch_event.get_pointer_id()] = true;
		}
		else if (touch_event.get_action() == TouchAction::Up)
		{
			io.MouseDown[touch_event.get_pointer_id()] = false;
		}
		else if (touch_event.get_action() == TouchAction::Move)
		{
			capture_move_event = io.WantCaptureMouse;
		}
	}

	// Toggle debug UI view when tap or clicking outside the GUI windows
	if (!io.WantCaptureMouse)
	{
		bool press_down =
		    (input_event.get_source() == EventSource::Mouse && static_cast<const MouseButtonInputEvent &>(input_event).get_action() == MouseAction::Down) ||
		    (input_event.get_source() == EventSource::Touchscreen && static_cast<const TouchInputEvent &>(input_event).get_action() == TouchAction::Down);
		bool press_up =
		    (input_event.get_source() == EventSource::Mouse && static_cast<const MouseButtonInputEvent &>(input_event).get_action() == MouseAction::Up) ||
		    (input_event.get_source() == EventSource::Touchscreen && static_cast<const TouchInputEvent &>(input_event).get_action() == TouchAction::Up);
		assert(!(press_down && press_up));        // can't be both, up and down at the same time

		if (press_down)
		{
			timer.start();
			if (input_event.get_source() == EventSource::Touchscreen)
			{
				const auto &touch_event = static_cast<const TouchInputEvent &>(input_event);
				if (touch_event.get_touch_points() == 2)
				{
					two_finger_tap = true;
				}
			}
		}
		else if (press_up)
		{
			auto press_delta = timer.stop<Timer::Milliseconds>();
			if (press_delta < press_time_ms)
			{
				if (input_event.get_source() == EventSource::Mouse)
				{
					const auto &mouse_button = static_cast<const MouseButtonInputEvent &>(input_event);
					if (mouse_button.get_button() == MouseButton::Right)
					{
						debug_view.active = !debug_view.active;
					}
				}
				else if (input_event.get_source() == EventSource::Touchscreen)
				{
					const auto &touch_event = static_cast<const TouchInputEvent &>(input_event);
					if (two_finger_tap && touch_event.get_touch_points() == 2)
					{
						debug_view.active = !debug_view.active;
					}
					else
					{
						two_finger_tap = false;
					}
				}
			}
		}
	}

	return capture_move_event;
}

template <vkb::BindingType bindingType>
inline bool Gui<bindingType>::is_debug_view_active() const
{
	return debug_view.active;
}

template <vkb::BindingType bindingType>
inline void Gui<bindingType>::new_frame()
{
	ImGui::NewFrame();
}

template <vkb::BindingType bindingType>
inline void Gui<bindingType>::prepare(PipelineCacheType                                     pipeline_cache,
                                      RenderPassType                                        render_pass,
                                      std::vector<PipelineShaderStageCreateInfoType> const &shader_stages)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		prepare_impl(pipeline_cache, render_pass, shader_stages);
	}
	else
	{
		prepare_impl(static_cast<vk::PipelineCache>(pipeline_cache),
		             static_cast<vk::RenderPass>(render_pass),
		             reinterpret_cast<std::vector<vk::PipelineShaderStageCreateInfo> const &>(shader_stages));
	}
}

template <vkb::BindingType bindingType>
inline void Gui<bindingType>::prepare_impl(vk::PipelineCache                                     pipeline_cache,
                                           vk::RenderPass                                        render_pass,
                                           std::vector<vk::PipelineShaderStageCreateInfo> const &shader_stages)
{
	vk::Device const &device = render_context.get_device().get_handle();

	// Descriptor pool
	vk::DescriptorPoolSize       pool_size          = {.type = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = 1};
	vk::DescriptorPoolCreateInfo descriptorPoolInfo = {.maxSets = 2, .poolSizeCount = 1, .pPoolSizes = &pool_size};
	descriptor_pool                                 = device.createDescriptorPool(descriptorPoolInfo);

	// Descriptor set layout
	vk::DescriptorSetLayoutBinding layout_binding = {
	    .binding = 0, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eFragment};
	vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {.bindingCount = 1, .pBindings = &layout_binding};
	descriptor_set_layout                                               = device.createDescriptorSetLayout(descriptor_set_layout_create_info);

	// Descriptor set
	vk::DescriptorSetAllocateInfo descriptor_allocation = {.descriptorPool = descriptor_pool, .descriptorSetCount = 1, .pSetLayouts = &descriptor_set_layout};
	descriptor_set                                      = device.allocateDescriptorSets(descriptor_allocation).front();

	// Update descriptor set with font image
	vk::DescriptorImageInfo font_descriptor      = {.sampler     = sampler->get_handle(),
	                                                .imageView   = font_image_view->get_handle(),
	                                                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};
	vk::WriteDescriptorSet  write_descriptor_set = {
	     .dstSet = descriptor_set, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .pImageInfo = &font_descriptor};
	device.updateDescriptorSets(write_descriptor_set, nullptr);

	// Setup graphics pipeline for UI rendering
	vk::PipelineInputAssemblyStateCreateInfo input_assembly_state = {.topology = vk::PrimitiveTopology::eTriangleList};

	vk::PipelineRasterizationStateCreateInfo rasterization_state = {
	    .polygonMode = vk::PolygonMode::eFill, .cullMode = vk::CullModeFlagBits::eNone, .frontFace = vk::FrontFace::eCounterClockwise, .lineWidth = 1.0f};

	// Enable blending
	vk::PipelineColorBlendAttachmentState blend_attachment_state = {.blendEnable         = true,
	                                                                .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
	                                                                .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
	                                                                .colorBlendOp        = vk::BlendOp::eAdd,
	                                                                .srcAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
	                                                                .dstAlphaBlendFactor = vk::BlendFactor::eZero,
	                                                                .alphaBlendOp        = vk::BlendOp::eAdd,
	                                                                .colorWriteMask      = vk::FlagTraits<vk::ColorComponentFlagBits>::allFlags};
	vk::PipelineColorBlendStateCreateInfo color_blend_state      = {.attachmentCount = 1, .pAttachments = &blend_attachment_state};

	vk::PipelineDepthStencilStateCreateInfo depth_stencil_state = {.depthTestEnable  = false,
	                                                               .depthWriteEnable = false,
	                                                               .depthCompareOp   = vk::CompareOp::eAlways,
	                                                               .back             = {.compareOp = vk::CompareOp::eAlways}};

	vk::PipelineViewportStateCreateInfo viewport_state = {.viewportCount = 1, .scissorCount = 1};

	vk::PipelineMultisampleStateCreateInfo multisample_state = {.rasterizationSamples = vk::SampleCountFlagBits::e1};

	std::array<vk::DynamicState, 2>    dynamic_state_enables = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
	vk::PipelineDynamicStateCreateInfo dynamic_state         = {.dynamicStateCount = static_cast<uint32_t>(dynamic_state_enables.size()),
	                                                            .pDynamicStates    = dynamic_state_enables.data()};

	// Vertex bindings an attributes based on ImGui vertex definition
	vk::VertexInputBindingDescription                  vertex_input_binding    = {.binding = 0, .stride = sizeof(ImDrawVert), .inputRate = vk::VertexInputRate::eVertex};
	std::array<vk::VertexInputAttributeDescription, 3> vertex_input_attributes = {
	    {{.location = 0, .binding = 0, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(ImDrawVert, pos)},
	     {.location = 1, .binding = 0, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(ImDrawVert, uv)},
	     {.location = 2, .binding = 0, .format = vk::Format::eR8G8B8A8Unorm, .offset = offsetof(ImDrawVert, col)}}};
	vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info = {.vertexBindingDescriptionCount = 1,
	                                                                         .pVertexBindingDescriptions    = &vertex_input_binding,
	                                                                         .vertexAttributeDescriptionCount =
	                                                                             static_cast<uint32_t>(vertex_input_attributes.size()),
	                                                                         .pVertexAttributeDescriptions = vertex_input_attributes.data()};

	vk::GraphicsPipelineCreateInfo pipeline_create_info = {.stageCount          = static_cast<uint32_t>(shader_stages.size()),
	                                                       .pStages             = shader_stages.data(),
	                                                       .pVertexInputState   = &vertex_input_state_create_info,
	                                                       .pInputAssemblyState = &input_assembly_state,
	                                                       .pViewportState      = &viewport_state,
	                                                       .pRasterizationState = &rasterization_state,
	                                                       .pMultisampleState   = &multisample_state,
	                                                       .pDepthStencilState  = &depth_stencil_state,
	                                                       .pColorBlendState    = &color_blend_state,
	                                                       .pDynamicState       = &dynamic_state,
	                                                       .layout              = pipeline_layout->get_handle(),
	                                                       .renderPass          = render_pass,
	                                                       .subpass             = subpass,
	                                                       .basePipelineIndex   = -1};

	vk::Result result;
	std::tie(result, pipeline) = device.createGraphicsPipeline(pipeline_cache, pipeline_create_info);
	assert(result == vk::Result::eSuccess);
}

template <vkb::BindingType bindingType>
inline void Gui<bindingType>::resize(const uint32_t width, const uint32_t height) const
{
	auto &io         = ImGui::GetIO();
	io.DisplaySize.x = static_cast<float>(width);
	io.DisplaySize.y = static_cast<float>(height);
}

template <vkb::BindingType bindingType>
inline void Gui<bindingType>::set_subpass(const uint32_t subpass)
{
	this->subpass = subpass;
}

template <vkb::BindingType bindingType>
inline void Gui<bindingType>::show_app_info(const std::string &app_name)
{
	// Sample name
	ImGui::Text("%s", app_name.c_str());

	// GPU name
	auto &device            = render_context.get_device();
	auto  device_name_label = "GPU: " + std::string(device.get_gpu().get_properties().deviceName.data());
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - ImGui::CalcTextSize(device_name_label.c_str()).x);
	ImGui::Text("%s", device_name_label.c_str());
}

template <vkb::BindingType bindingType>
inline void Gui<bindingType>::show_debug_window(DebugInfo &debug_info, const ImVec2 &position)
{
	auto &io    = ImGui::GetIO();
	auto &style = ImGui::GetStyle();
	auto &font  = get_font(default_window_font);

	// Calculate only once
	if (debug_view.label_column_width == 0)
	{
		debug_view.label_column_width = style.ItemInnerSpacing.x + debug_info.get_longest_label() * font.size / debug_view.scale;
	}

	ImGui::SetNextWindowBgAlpha(overlay_alpha);
	ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowContentSize(ImVec2{io.DisplaySize.x, 0.0f});

	bool                   is_open = true;
	const ImGuiWindowFlags flags   = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
	                               ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

	ImGui::Begin("Debug Window", &is_open, flags);
	ImGui::PushFont(font.handle);

	auto field_count = debug_info.get_fields().size() > debug_view.max_fields ? debug_view.max_fields : debug_info.get_fields().size();

	ImGui::BeginChild("Table", ImVec2(0, field_count * (font.size + style.ItemSpacing.y)), false);
	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, debug_view.label_column_width);
	ImGui::SetColumnWidth(1, io.DisplaySize.x - debug_view.label_column_width);
	for (auto &field : debug_info.get_fields())
	{
		const std::string &label = field->label;
		const std::string &value = field->to_string();
		ImGui::Text("%s", label.c_str());
		ImGui::NextColumn();
		ImGui::Text(" %s", value.c_str());
		ImGui::NextColumn();
	}
	ImGui::Columns(1);
	ImGui::EndChild();

	ImGui::PopFont();
	ImGui::End();
}

template <vkb::BindingType bindingType>
inline void Gui<bindingType>::show_options_window(std::function<void()> body, const uint32_t lines)
{
	// Add padding around the text so that the options are not
	// too close to the edges and are easier to interact with.
	// Also add double vertical padding to avoid rounded corners.
	const float window_padding = ImGui::CalcTextSize("T").x;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{window_padding, window_padding * 2.0f});
	auto window_height = lines * ImGui::GetTextLineHeightWithSpacing() + ImGui::GetStyle().WindowPadding.y * 2.0f;
	auto window_width  = ImGui::GetIO().DisplaySize.x;
	ImGui::SetNextWindowBgAlpha(overlay_alpha);
	const ImVec2 size = ImVec2(window_width, 0);
	ImGui::SetNextWindowSize(size, ImGuiCond_Always);
	const ImVec2 pos = ImVec2(0.0f, ImGui::GetIO().DisplaySize.y - window_height);
	ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
	const ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
	                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoSavedSettings |
	                               ImGuiWindowFlags_NoFocusOnAppearing;
	bool is_open = true;
	ImGui::Begin("Options", &is_open, flags);
	body();
	ImGui::End();
	ImGui::PopStyleVar();
}

template <vkb::BindingType bindingType>
inline void Gui<bindingType>::show_simple_window(const std::string &name, uint32_t last_fps, std::function<void()> body)
{
	ImGuiIO &io = ImGui::GetIO();

	ImGui::NewFrame();
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
	ImGui::SetNextWindowPos(ImVec2(10, 10));
	ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_FirstUseEver);
	ImGui::Begin("Vulkan Example", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
	ImGui::TextUnformatted(name.c_str());
	ImGui::TextUnformatted(std::string(render_context.get_device().get_gpu().get_properties().deviceName.data()).c_str());
	ImGui::Text("%.2f ms/frame (%.1d fps)", (1000.0f / last_fps), last_fps);
	ImGui::PushItemWidth(110.0f * dpi_factor);

	body();

	ImGui::PopItemWidth();
	ImGui::End();
	ImGui::PopStyleVar();
}

template <vkb::BindingType bindingType>
inline void Gui<bindingType>::show_stats(const StatsType &stats)
{
	for (const auto &stat_index : stats.get_requested_stats())
	{
		// Draw graph
		auto       &graph_data     = stats_view.get_stat_graph_data(stat_index);
		const auto &graph_elements = stats.get_data(stat_index);
		float       graph_min      = 0.0f;
		float       graph_max      = graph_data.max_value;

		if (!graph_data.has_fixed_max)
		{
			auto new_max = *std::max_element(graph_elements.begin(), graph_elements.end()) * stats_view.get_top_padding();
			if (new_max > graph_max)
			{
				graph_max = new_max;
			}
		}

		const ImVec2 graph_size = ImVec2{ImGui::GetIO().DisplaySize.x, stats_view.get_graph_height() /* dpi */ * dpi_factor};

		std::stringstream graph_label;
		float             avg = std::accumulate(graph_elements.begin(), graph_elements.end(), 0.0f) / graph_elements.size();

		// Check if the stat is available in the current platform
		if (stats.is_available(stat_index))
		{
			auto graph_value = avg * graph_data.scale_factor;
			graph_label << fmt::vformat(graph_data.name + ": " + graph_data.format, fmt::make_format_args(graph_value));
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PlotLines("", &graph_elements[0], static_cast<int>(graph_elements.size()), 0, graph_label.str().c_str(), graph_min, graph_max, graph_size);
			ImGui::PopItemFlag();
		}
		else
		{
			graph_label << graph_data.name << ": not available";
			ImGui::Text("%s", graph_label.str().c_str());
		}
	}
}

template <vkb::BindingType bindingType>
inline void Gui<bindingType>::show_top_window(const std::string &app_name, const StatsType *stats, DebugInfo *debug_info)
{
	// Transparent background
	ImGui::SetNextWindowBgAlpha(overlay_alpha);
	ImVec2 size{ImGui::GetIO().DisplaySize.x, 0.0f};
	ImGui::SetNextWindowSize(size, ImGuiCond_Always);

	// Top left
	ImVec2 pos{0.0f, 0.0f};
	ImGui::SetNextWindowPos(pos, ImGuiCond_Always);

	bool is_open = true;
	ImGui::Begin("Top", &is_open, common_flags);

	show_app_info(app_name);

	if (stats)
	{
		show_stats(*stats);

		// Reset max values if user taps on this window
		if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0 /* left */))
		{
			stats_view.reset_max_values();
		}
	}

	if (debug_info)
	{
		if (debug_view.active)
		{
			show_debug_window(*debug_info, ImVec2{0, ImGui::GetWindowSize().y});
		}
	}

	ImGui::End();
}

template <vkb::BindingType bindingType>
inline void Gui<bindingType>::update(const float delta_time)
{
	if (visible != prev_visible)
	{
		drawer.set_dirty(true);
		prev_visible = visible;
	}

	if (!visible)
	{
		ImGui::EndFrame();
		return;
	}

	// Update imGui
	ImGuiIO &io     = ImGui::GetIO();
	auto     extent = render_context.get_surface_extent();
	resize(extent.width, extent.height);
	io.DeltaTime = delta_time;

	// Render to generate draw buffers
	ImGui::Render();
}

template <vkb::BindingType bindingType>
inline bool Gui<bindingType>::update_buffers()
{
	ImDrawData *draw_data = ImGui::GetDrawData();

	if (!draw_data)
	{
		return false;
	}

	size_t vertex_buffer_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
	size_t index_buffer_size  = draw_data->TotalIdxCount * sizeof(ImDrawIdx);

	if ((vertex_buffer_size == 0) || (index_buffer_size == 0))
	{
		return false;
	}

	bool updated = false;
	if (!vertex_buffer->get_handle() || (vertex_buffer_size != vertex_buffer->get_size()))
	{
		vertex_buffer.reset();
		vertex_buffer = std::make_unique<vkb::core::BufferCpp>(render_context.get_device(), vertex_buffer_size,
		                                                       vk::BufferUsageFlagBits::eVertexBuffer,
		                                                       VMA_MEMORY_USAGE_GPU_TO_CPU);
		vertex_buffer->set_debug_name("GUI vertex buffer");
		updated = true;
	}

	if (!index_buffer->get_handle() || (index_buffer_size != index_buffer->get_size()))
	{
		index_buffer.reset();
		index_buffer = std::make_unique<vkb::core::BufferCpp>(render_context.get_device(), index_buffer_size,
		                                                      vk::BufferUsageFlagBits::eIndexBuffer,
		                                                      VMA_MEMORY_USAGE_GPU_TO_CPU);
		index_buffer->set_debug_name("GUI index buffer");
		updated = true;
	}

	// Upload data
	upload_draw_data(draw_data, vertex_buffer->map(), index_buffer->map());

	vertex_buffer->flush();
	index_buffer->flush();

	vertex_buffer->unmap();
	index_buffer->unmap();

	return updated;
}

template <vkb::BindingType bindingType>
inline vkb::BufferAllocationCpp Gui<bindingType>::update_buffers(vkb::core::CommandBufferCpp &command_buffer)
{
	ImDrawData *draw_data = ImGui::GetDrawData();

	if (!draw_data)
	{
		return vkb::BufferAllocationCpp{};
	}

	size_t vertex_buffer_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
	size_t index_buffer_size  = draw_data->TotalIdxCount * sizeof(ImDrawIdx);

	if ((vertex_buffer_size == 0) || (index_buffer_size == 0))
	{
		return vkb::BufferAllocationCpp{};
	}

	std::vector<uint8_t> vertex_data(vertex_buffer_size);
	std::vector<uint8_t> index_data(index_buffer_size);

	upload_draw_data(draw_data, vertex_data.data(), index_data.data());

	auto vertex_allocation = render_context.get_active_frame().allocate_buffer(vk::BufferUsageFlagBits::eVertexBuffer, vertex_buffer_size);

	vertex_allocation.update(vertex_data);

	std::vector<std::reference_wrapper<const vkb::core::BufferCpp>> buffers;
	buffers.emplace_back(std::ref(vertex_allocation.get_buffer()));

	std::vector<VkDeviceSize> offsets{vertex_allocation.get_offset()};

	command_buffer.bind_vertex_buffers(0, buffers, offsets);

	auto index_allocation = render_context.get_active_frame().allocate_buffer(vk::BufferUsageFlagBits::eIndexBuffer, index_buffer_size);

	index_allocation.update(index_data);

	command_buffer.bind_index_buffer(index_allocation.get_buffer(), index_allocation.get_offset(), vk::IndexType::eUint16);

	return vertex_allocation;
}

template <vkb::BindingType bindingType>
inline void Gui<bindingType>::upload_draw_data(const ImDrawData *draw_data, uint8_t *vertex_data, uint8_t *index_data)
{
	ImDrawVert *vtx_dst = reinterpret_cast<ImDrawVert *>(vertex_data);
	ImDrawIdx  *idx_dst = reinterpret_cast<ImDrawIdx *>(index_data);

	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList *cmd_list = draw_data->CmdLists[n];
		memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		vtx_dst += cmd_list->VtxBuffer.Size;
		idx_dst += cmd_list->IdxBuffer.Size;
	}
}

template <vkb::BindingType bindingType>
inline Gui<bindingType>::StatsView::StatsView(const StatsType *stats)
{
	if (stats == nullptr)
	{
		return;
	}

	// Request graph data information for each stat and record it in graph_map
	const std::set<StatIndex> &indices = stats->get_requested_stats();

	for (StatIndex i : indices)
	{
		graph_map[i] = stats->get_graph_data(i);
	}
}

template <vkb::BindingType bindingType>
inline float Gui<bindingType>::StatsView::get_graph_height() const
{
	return graph_height;
}

template <vkb::BindingType bindingType>
inline StatGraphData const &Gui<bindingType>::StatsView::get_stat_graph_data(StatIndex const &stat_index) const
{
	auto graph_map_it = graph_map.find(stat_index);

	assert(graph_map_it != graph_map.end() && "StatIndex not implemented in gui graph_map");

	return graph_map_it->second;
}

template <vkb::BindingType bindingType>
inline float Gui<bindingType>::StatsView::get_top_padding() const
{
	return top_padding;
}

template <vkb::BindingType bindingType>
inline void Gui<bindingType>::StatsView::reset_max_value(const StatIndex index)
{
	auto pr = graph_map.find(index);
	if (pr != graph_map.end())
	{
		reset_graph_max_value(pr->second);
	}
}

template <vkb::BindingType bindingType>
inline void Gui<bindingType>::StatsView::reset_max_values()
{
	// For every entry in the map
	std::ranges::for_each(graph_map, [](auto &pr) { reset_graph_max_value(pr.second); });
}

}        // namespace vkb
