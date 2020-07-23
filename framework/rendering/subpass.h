/* Copyright (c) 2019-2020, Arm Limited and Contributors
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

#include "buffer_pool.h"
#include "common/helpers.h"
#include "core/shader_module.h"
#include "rendering/pipeline_state.h"
#include "rendering/render_context.h"
#include "rendering/render_frame.h"
#include "scene_graph/components/light.h"
#include "scene_graph/node.h"

VKBP_DISABLE_WARNINGS()
#include "common/glm_common.h"
VKBP_ENABLE_WARNINGS()

namespace vkb
{
class CommandBuffer;

struct alignas(16) Light
{
	glm::vec4 position;         // position.w represents type of light
	glm::vec4 color;            // color.w represents light intensity
	glm::vec4 direction;        // direction.w represents range
	glm::vec2 info;             // (only used for spot lights) info.x represents light inner cone angle, info.y represents light outer cone angle
};

/**
 * @brief Calculates the vulkan style projection matrix
 * @param proj The projection matrix
 * @return The vulkan style projection matrix
 */
glm::mat4 vulkan_style_projection(const glm::mat4 &proj);

extern const std::vector<std::string> light_type_definitions;

/**
 * @brief This class defines an interface for subpasses
 *        where they need to implement the draw function.
 *        It is used to construct a RenderPipeline
 */
class Subpass
{
  public:
	Subpass(RenderContext &render_context, ShaderSource &&vertex_shader, ShaderSource &&fragment_shader);

	Subpass(const Subpass &) = delete;

	Subpass(Subpass &&) = default;

	virtual ~Subpass() = default;

	Subpass &operator=(const Subpass &) = delete;

	Subpass &operator=(Subpass &&) = delete;

	/**
	 * @brief Prepares the shaders and shader variants for a subpass
	 */
	virtual void prepare() = 0;

	/**
	 * @brief Updates the render target attachments with the ones stored in this subpass
	 *        This function is called by the RenderPipeline before beginning the render
	 *        pass and before proceeding with a new subpass.
	 */
	void update_render_target_attachments(RenderTarget &render_target);

	/**
	 * @brief Draw virtual function
	 * @param command_buffer Command buffer to use to record draw commands
	 */
	virtual void draw(CommandBuffer &command_buffer) = 0;

	RenderContext &get_render_context();

	const ShaderSource &get_vertex_shader() const;

	const ShaderSource &get_fragment_shader() const;

	DepthStencilState &get_depth_stencil_state();

	const std::vector<uint32_t> &get_input_attachments() const;

	void set_input_attachments(std::vector<uint32_t> input);

	const std::vector<uint32_t> &get_output_attachments() const;

	void set_output_attachments(std::vector<uint32_t> output);

	void clear_dynamic_resources();

	void add_dynamic_resources(const std::vector<std::string> &dynamic_resources);

	void set_sample_count(VkSampleCountFlagBits sample_count);

	const std::vector<uint32_t> &get_color_resolve_attachments() const;

	void set_color_resolve_attachments(std::vector<uint32_t> color_resolve);

	const bool &get_disable_depth_stencil_attachment() const;

	void set_disable_depth_stencil_attachment(bool disable_depth_stencil);

	const uint32_t &get_depth_stencil_resolve_attachment() const;

	void set_depth_stencil_resolve_attachment(uint32_t depth_stencil_resolve);

	const VkResolveModeFlagBits get_depth_stencil_resolve_mode() const;

	void set_depth_stencil_resolve_mode(VkResolveModeFlagBits mode);

	/**
	 * @brief Create a buffer allocation from scene graph lights to be bound to shaders
	 * 
	 * @tparam T ForwardLights / DeferredLights
	 * @param scene_lights  Lights from the scene graph
	 * @param max_lights MAX_FORWARD_LIGHT_COUNT / MAX_DEFERRED_LIGHT_COUNT
	 * @return BufferAllocation A buffer allocation created for use in shaders
	 */
	template <typename T>
	BufferAllocation allocate_lights(const std::vector<sg::Light *> &scene_lights, size_t max_lights)
	{
		assert(scene_lights.size() <= max_lights && "Exceeding Max Light Capacity");

		T light_info;
		light_info.count = to_u32(scene_lights.size());

		std::transform(scene_lights.begin(), scene_lights.end(), light_info.lights, [](sg::Light *light) -> Light {
			const auto &properties = light->get_properties();
			auto &      transform  = light->get_node()->get_transform();

			return {{transform.get_translation(), static_cast<float>(light->get_light_type())},
			        {properties.color, properties.intensity},
			        {transform.get_rotation() * properties.direction, properties.range},
			        {properties.inner_cone_angle, properties.outer_cone_angle}};
		});

		auto &           render_frame = get_render_context().get_active_frame();
		BufferAllocation light_buffer = render_frame.allocate_buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(T));
		light_buffer.update(light_info);

		return light_buffer;
	}

	/**
	 * @brief Create a buffer allocation from scene graph lights to be bound to shaders
	 *		  Provides the specified number of lights, regardless of how many are within the scene
	 *
	 * @tparam T ForwardLights / DeferredLights
	 * @param scene_lights  Lights from the scene graph
	 * @param num_lights Number of lights to render
	 * @return BufferAllocation A buffer allocation created for use in shaders
	 */
	template <typename T>
	BufferAllocation allocate_set_num_lights(const std::vector<sg::Light *> &scene_lights, size_t num_lights)
	{
		T light_info;
		light_info.count = to_u32(num_lights);

		std::vector<Light> lights_vector;
		for (uint32_t i = 0U; i < num_lights; ++i)
		{
			auto        light      = i < scene_lights.size() ? scene_lights.at(i) : scene_lights.back();
			const auto &properties = light->get_properties();
			auto &      transform  = light->get_node()->get_transform();

			lights_vector.push_back(Light({{transform.get_translation(), static_cast<float>(light->get_light_type())},
			                               {properties.color, properties.intensity},
			                               {transform.get_rotation() * properties.direction, properties.range},
			                               {properties.inner_cone_angle, properties.outer_cone_angle}}));
		}

		std::copy(lights_vector.begin(), lights_vector.end(), light_info.lights);

		auto &           render_frame = get_render_context().get_active_frame();
		BufferAllocation light_buffer = render_frame.allocate_buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(T));
		light_buffer.update(light_info);

		return light_buffer;
	}

  protected:
	RenderContext &render_context;

	VkSampleCountFlagBits sample_count{VK_SAMPLE_COUNT_1_BIT};

	// A map of shader resource names and the mode of constant data
	std::unordered_map<std::string, ShaderResourceMode> resource_mode_map;

  private:
	ShaderSource vertex_shader;

	ShaderSource fragment_shader;

	DepthStencilState depth_stencil_state{};

	/**
	 * @brief When creating the renderpass, pDepthStencilAttachment will
	 *        be set to nullptr, which disables depth testing
	 */
	bool disable_depth_stencil_attachment{false};

	/**
	 * @brief When creating the renderpass, if not None, the resolve
	 *        of the multisampled depth attachment will be enabled,
	 *        with this mode, to depth_stencil_resolve_attachment
	 */
	VkResolveModeFlagBits depth_stencil_resolve_mode{VK_RESOLVE_MODE_NONE};

	/// Default to no input attachments
	std::vector<uint32_t> input_attachments = {};

	/// Default to swapchain output attachment
	std::vector<uint32_t> output_attachments = {0};

	/// Default to no color resolve attachments
	std::vector<uint32_t> color_resolve_attachments = {};

	/// Default to no depth stencil resolve attachment
	uint32_t depth_stencil_resolve_attachment{VK_ATTACHMENT_UNUSED};
};

}        // namespace vkb
