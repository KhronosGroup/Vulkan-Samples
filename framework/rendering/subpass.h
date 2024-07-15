/* Copyright (c) 2019-2024, Arm Limited and Contributors
 * Copyright (c) 2024, NVIDIA CORPORATION. All rights reserved.
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
#include "rendering/hpp_pipeline_state.h"
#include "rendering/hpp_render_target.h"
#include "rendering/pipeline_state.h"
#include "scene_graph/components/light.h"
#include "scene_graph/node.h"

namespace vkb
{
class CommandBuffer;
class RenderContext;
class RenderTarget;
class ShaderSource;

namespace core
{
class HPPCommandBuffer;
}

namespace rendering
{
class HPPRenderContext;

struct alignas(16) Light
{
	glm::vec4 position;         // position.w represents type of light
	glm::vec4 color;            // color.w represents light intensity
	glm::vec4 direction;        // direction.w represents range
	glm::vec2 info;             // (only used for spot lights) info.x represents light inner cone angle, info.y represents light outer cone angle
};

template <vkb::BindingType bindingType>
struct LightingState
{
	std::vector<Light>            directional_lights;
	std::vector<Light>            point_lights;
	std::vector<Light>            spot_lights;
	BufferAllocation<bindingType> light_buffer;
};

using LightingStateC   = LightingState<vkb::BindingType::C>;
using LightingStateCpp = LightingState<vkb::BindingType::Cpp>;

/**
 * @brief Calculates the vulkan style projection matrix
 * @param proj The projection matrix
 * @return The vulkan style projection matrix
 */
glm::mat4 vulkan_style_projection(const glm::mat4 &proj);

inline const std::vector<std::string> light_type_definitions = {
    "DIRECTIONAL_LIGHT " + std::to_string(static_cast<float>(sg::LightType::Directional)),
    "POINT_LIGHT " + std::to_string(static_cast<float>(sg::LightType::Point)),
    "SPOT_LIGHT " + std::to_string(static_cast<float>(sg::LightType::Spot))};

/**
 * @brief This class defines an interface for subpasses
 *        where they need to implement the draw function.
 *        It is used to construct a RenderPipeline
 */
template <vkb::BindingType bindingType>
class Subpass
{
	using ResolveModeFlagBitsType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::ResolveModeFlagBits, VkResolveModeFlagBits>::type;
	using SampleCountflagBitsType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::SampleCountFlagBits, VkSampleCountFlagBits>::type;

	using CommandBufferType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::core::HPPCommandBuffer, vkb::CommandBuffer>::type;
	using DepthStencilStateType =
	    typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::rendering::HPPDepthStencilState, vkb::DepthStencilState>::type;
	using RenderContextType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::rendering::HPPRenderContext, vkb::RenderContext>::type;
	using RenderTargetType  = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::rendering::HPPRenderTarget, vkb::RenderTarget>::type;

  public:
	Subpass(RenderContextType &render_context, ShaderSource &&vertex_shader, ShaderSource &&fragment_shader);

	Subpass(const Subpass &)            = delete;
	Subpass(Subpass &&)                 = default;
	virtual ~Subpass()                  = default;
	Subpass &operator=(const Subpass &) = delete;
	Subpass &operator=(Subpass &&)      = delete;

	/**
	 * @brief Draw virtual function
	 * @param command_buffer Command buffer to use to record draw commands
	 */
	virtual void draw(CommandBufferType &command_buffer) = 0;

	/**
	 * @brief Prepares the shaders and shader variants for a subpass
	 */
	virtual void prepare() = 0;

	/**
	 * @brief Prepares the lighting state to have its lights
	 *
	 * @tparam A light structure that has 'directional_lights', 'point_lights' and 'spot_light' array fields defined.
	 * @param scene_lights All of the light components from the scene graph
	 * @param max_lights_per_type The maximum amount of lights allowed for any given type of light.
	 */
	template <typename T>
	void allocate_lights(const std::vector<sg::Light *> &scene_lights,
	                     size_t                          max_lights_per_type);

	const std::vector<uint32_t>                               &get_color_resolve_attachments() const;
	const std::string                                         &get_debug_name() const;
	const uint32_t                                            &get_depth_stencil_resolve_attachment() const;
	ResolveModeFlagBitsType                                    get_depth_stencil_resolve_mode() const;
	DepthStencilStateType                                     &get_depth_stencil_state();
	const bool                                                &get_disable_depth_stencil_attachment() const;
	const ShaderSource                                        &get_fragment_shader() const;
	const std::vector<uint32_t>                               &get_input_attachments() const;
	LightingState<bindingType>                                &get_lighting_state();
	const std::vector<uint32_t>                               &get_output_attachments() const;
	RenderContextType                                         &get_render_context();
	std::unordered_map<std::string, ShaderResourceMode> const &get_resource_mode_map() const;
	SampleCountflagBitsType                                    get_sample_count() const;
	const ShaderSource                                        &get_vertex_shader() const;
	void                                                       set_color_resolve_attachments(std::vector<uint32_t> const &color_resolve);
	void                                                       set_debug_name(const std::string &name);
	void                                                       set_disable_depth_stencil_attachment(bool disable_depth_stencil);
	void                                                       set_depth_stencil_resolve_attachment(uint32_t depth_stencil_resolve);
	void                                                       set_depth_stencil_resolve_mode(ResolveModeFlagBitsType mode);
	void                                                       set_input_attachments(std::vector<uint32_t> const &input);
	void                                                       set_output_attachments(std::vector<uint32_t> const &output);
	void                                                       set_sample_count(SampleCountflagBitsType sample_count);

	/**
	 * @brief Updates the render target attachments with the ones stored in this subpass
	 *        This function is called by the RenderPipeline before beginning the render
	 *        pass and before proceeding with a new subpass.
	 */
	void update_render_target_attachments(RenderTargetType &render_target);

  private:
	/// Default to no color resolve attachments
	std::vector<uint32_t> color_resolve_attachments = {};

	std::string debug_name{};

	/**
	 * @brief When creating the renderpass, if not None, the resolve
	 *        of the multisampled depth attachment will be enabled,
	 *        with this mode, to depth_stencil_resolve_attachment
	 */
	vk::ResolveModeFlagBits depth_stencil_resolve_mode{vk::ResolveModeFlagBits::eNone};

	vkb::rendering::HPPDepthStencilState depth_stencil_state{};

	/**
	 * @brief When creating the renderpass, pDepthStencilAttachment will
	 *        be set to nullptr, which disables depth testing
	 */
	bool disable_depth_stencil_attachment{false};

	/// Default to no depth stencil resolve attachment
	uint32_t depth_stencil_resolve_attachment{VK_ATTACHMENT_UNUSED};

	/// The structure containing all the requested render-ready lights for the scene
	LightingStateCpp lighting_state{};

	ShaderSource fragment_shader;

	/// Default to no input attachments
	std::vector<uint32_t> input_attachments = {};

	/// Default to swapchain output attachment
	std::vector<uint32_t> output_attachments = {0};

	vkb::rendering::HPPRenderContext &render_context;

	// A map of shader resource names and the mode of constant data
	std::unordered_map<std::string, ShaderResourceMode> resource_mode_map;

	vk::SampleCountFlagBits sample_count{vk::SampleCountFlagBits::e1};
	ShaderSource            vertex_shader;
};

using SubpassC   = Subpass<vkb::BindingType::C>;
using SubpassCpp = Subpass<vkb::BindingType::Cpp>;
}        // namespace rendering
}        // namespace vkb

#include "rendering/hpp_render_context.h"

namespace vkb
{
namespace rendering
{
inline glm::mat4 vulkan_style_projection(const glm::mat4 &proj)
{
	// Flip Y in clipspace. X = -1, Y = -1 is topLeft in Vulkan.
	glm::mat4 mat = proj;
	mat[1][1] *= -1;

	return mat;
}

template <vkb::BindingType bindingType>
inline Subpass<bindingType>::Subpass(RenderContextType &render_context, ShaderSource &&vertex_source, ShaderSource &&fragment_source) :
    render_context{reinterpret_cast<vkb::rendering::HPPRenderContext &>(render_context)},
    vertex_shader{std::move(vertex_source)},
    fragment_shader{std::move(fragment_source)}
{
}

template <vkb::BindingType bindingType>
inline const std::vector<uint32_t> &Subpass<bindingType>::get_input_attachments() const
{
	return input_attachments;
}

template <vkb::BindingType bindingType>
inline LightingState<bindingType> &Subpass<bindingType>::get_lighting_state()
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return lighting_state;
	}
	else
	{
		return reinterpret_cast<LightingState<bindingType> &>(lighting_state);
	}
}

template <vkb::BindingType bindingType>
inline const std::vector<uint32_t> &Subpass<bindingType>::get_output_attachments() const
{
	return output_attachments;
}

template <vkb::BindingType bindingType>
inline typename vkb::rendering::Subpass<bindingType>::RenderContextType &Subpass<bindingType>::get_render_context()
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return render_context;
	}
	else
	{
		return reinterpret_cast<vkb::RenderContext &>(render_context);
	}
}

template <vkb::BindingType bindingType>
std::unordered_map<std::string, ShaderResourceMode> const &Subpass<bindingType>::get_resource_mode_map() const
{
	return resource_mode_map;
}

template <vkb::BindingType bindingType>
inline typename Subpass<bindingType>::SampleCountflagBitsType Subpass<bindingType>::get_sample_count() const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return sample_count;
	}
	else
	{
		return static_cast<VkSampleCountFlagBits>(sample_count);
	}
}

template <vkb::BindingType bindingType>
inline const ShaderSource &Subpass<bindingType>::get_vertex_shader() const
{
	return vertex_shader;
}

template <vkb::BindingType bindingType>
template <typename T>
void Subpass<bindingType>::allocate_lights(const std::vector<sg::Light *> &scene_lights,
                                           size_t                          max_lights_per_type)
{
	lighting_state.directional_lights.clear();
	lighting_state.point_lights.clear();
	lighting_state.spot_lights.clear();

	for (auto &scene_light : scene_lights)
	{
		const auto &properties = scene_light->get_properties();
		auto       &transform  = scene_light->get_node()->get_transform();

		Light light{{transform.get_translation(), static_cast<float>(scene_light->get_light_type())},
		            {properties.color, properties.intensity},
		            {transform.get_rotation() * properties.direction, properties.range},
		            {properties.inner_cone_angle, properties.outer_cone_angle}};

		switch (scene_light->get_light_type())
		{
			case sg::LightType::Directional:
			{
				if (lighting_state.directional_lights.size() < max_lights_per_type)
				{
					lighting_state.directional_lights.push_back(light);
				}
				else
				{
					LOGE("Subpass::allocate_lights: exceeding max_lights_per_type of {} for directional lights", max_lights_per_type);
				}
				break;
			}
			case sg::LightType::Point:
			{
				if (lighting_state.point_lights.size() < max_lights_per_type)
				{
					lighting_state.point_lights.push_back(light);
				}
				else
				{
					LOGE("Subpass::allocate_lights: exceeding max_lights_per_type of {} for point lights", max_lights_per_type);
				}
				break;
			}
			case sg::LightType::Spot:
			{
				if (lighting_state.spot_lights.size() < max_lights_per_type)
				{
					lighting_state.spot_lights.push_back(light);
				}
				else
				{
					LOGE("Subpass::allocate_lights: exceeding max_lights_per_type of {} for spot lights", max_lights_per_type);
				}
				break;
			}
			default:
				LOGE("Subpass::allocate_lights: encountered unknown light type {}", to_string(scene_light->get_light_type()));
				break;
		}
	}

	T light_info;

	std::copy(lighting_state.directional_lights.begin(), lighting_state.directional_lights.end(), light_info.directional_lights);
	std::copy(lighting_state.point_lights.begin(), lighting_state.point_lights.end(), light_info.point_lights);
	std::copy(lighting_state.spot_lights.begin(), lighting_state.spot_lights.end(), light_info.spot_lights);

	auto &render_frame          = render_context.get_active_frame();
	lighting_state.light_buffer = render_frame.allocate_buffer(vk::BufferUsageFlagBits::eUniformBuffer, sizeof(T));
	lighting_state.light_buffer.update(light_info);
}

template <vkb::BindingType bindingType>
inline const std::vector<uint32_t> &Subpass<bindingType>::get_color_resolve_attachments() const
{
	return color_resolve_attachments;
}

template <vkb::BindingType bindingType>
inline const std::string &Subpass<bindingType>::get_debug_name() const
{
	return debug_name;
}

template <vkb::BindingType bindingType>
inline const uint32_t &Subpass<bindingType>::get_depth_stencil_resolve_attachment() const
{
	return depth_stencil_resolve_attachment;
}

template <vkb::BindingType bindingType>
inline typename Subpass<bindingType>::ResolveModeFlagBitsType Subpass<bindingType>::get_depth_stencil_resolve_mode() const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return depth_stencil_resolve_mode;
	}
	else
	{
		return static_cast<VkResolveModeFlagBits>(depth_stencil_resolve_mode);
	}
}

template <vkb::BindingType bindingType>
inline typename Subpass<bindingType>::DepthStencilStateType &Subpass<bindingType>::get_depth_stencil_state()
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return depth_stencil_state;
	}
	else
	{
		return reinterpret_cast<vkb::DepthStencilState &>(depth_stencil_state);
	}
}

template <vkb::BindingType bindingType>
inline const bool &Subpass<bindingType>::get_disable_depth_stencil_attachment() const
{
	return disable_depth_stencil_attachment;
}

template <vkb::BindingType bindingType>
inline const ShaderSource &Subpass<bindingType>::get_fragment_shader() const
{
	return fragment_shader;
}

template <vkb::BindingType bindingType>
inline void Subpass<bindingType>::set_color_resolve_attachments(std::vector<uint32_t> const &color_resolve)
{
	color_resolve_attachments = color_resolve;
}

template <vkb::BindingType bindingType>
inline void Subpass<bindingType>::set_depth_stencil_resolve_attachment(uint32_t depth_stencil_resolve)
{
	depth_stencil_resolve_attachment = depth_stencil_resolve;
}

template <vkb::BindingType bindingType>
inline void Subpass<bindingType>::set_debug_name(const std::string &name)
{
	debug_name = name;
}

template <vkb::BindingType bindingType>
inline void Subpass<bindingType>::set_disable_depth_stencil_attachment(bool disable_depth_stencil)
{
	disable_depth_stencil_attachment = disable_depth_stencil;
}

template <vkb::BindingType bindingType>
inline void Subpass<bindingType>::set_depth_stencil_resolve_mode(ResolveModeFlagBitsType mode)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		depth_stencil_resolve_mode = mode;
	}
	else
	{
		depth_stencil_resolve_mode = static_cast<vk::ResolveModeFlagBits>(mode);
	}
}

template <vkb::BindingType bindingType>
inline void Subpass<bindingType>::set_input_attachments(std::vector<uint32_t> const &input)
{
	input_attachments = input;
}

template <vkb::BindingType bindingType>
inline void Subpass<bindingType>::set_output_attachments(std::vector<uint32_t> const &output)
{
	output_attachments = output;
}

template <vkb::BindingType bindingType>
inline void Subpass<bindingType>::set_sample_count(SampleCountflagBitsType sample_count_)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		sample_count = sample_count_;
	}
	else
	{
		sample_count = static_cast<vk::SampleCountFlagBits>(sample_count_);
	}
}

template <vkb::BindingType bindingType>
inline void Subpass<bindingType>::update_render_target_attachments(RenderTargetType &render_target)
{
	render_target.set_input_attachments(input_attachments);
	render_target.set_output_attachments(output_attachments);
}
}        // namespace rendering
}        // namespace vkb
