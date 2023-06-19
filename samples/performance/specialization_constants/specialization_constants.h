/* Copyright (c) 2019-2023, Arm Limited and Contributors
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
#include "rendering/render_pipeline.h"
#include "rendering/subpasses/forward_subpass.h"
#include "scene_graph/components/light.h"
#include "scene_graph/components/mesh.h"
#include "scene_graph/components/perspective_camera.h"
#include "vulkan_sample.h"

#define LIGHT_COUNT 1

struct alignas(16) CustomForwardLights
{
	uint32_t   count;
	vkb::Light lights[LIGHT_COUNT];
};

/**
 * @brief Using specialization constants
 */
class SpecializationConstants : public vkb::VulkanSample
{
  public:
	SpecializationConstants();

	virtual ~SpecializationConstants() = default;

	virtual bool prepare(const vkb::ApplicationOptions &options) override;

	/**
	 * @brief This subpass is responsible for rendering a Scene
	 *		  It implements a custom draw function which passes a custom light count
	 */
	class ForwardSubpassCustomLights : public vkb::ForwardSubpass
	{
	  public:
		ForwardSubpassCustomLights(vkb::RenderContext &render_context,
		                           vkb::ShaderSource &&vertex_source, vkb::ShaderSource &&fragment_source,
		                           vkb::sg::Scene &scene, vkb::sg::Camera &camera);

		virtual void prepare() override;

		virtual void draw(vkb::CommandBuffer &command_buffer) override;

		/**
		 * @brief Create a buffer allocation from scene graph lights for the specialization constants sample
		 *		  Provides the specified number of lights, regardless of how many are within the scene
		 *
		 * @tparam T ForwardLights / DeferredLights
		 * @param command_buffer The command buffer that the returned light buffer allocation will be bound to
		 * @param scene_lights  Lights from the scene graph
		 * @param light_count Number of lights to render
		 * @return BufferAllocation A buffer allocation created for use in shaders
		 */
		template <typename T>
		vkb::BufferAllocation allocate_custom_lights(vkb::CommandBuffer &command_buffer, const std::vector<vkb::sg::Light *> &scene_lights, size_t light_count)
		{
			T light_info;
			light_info.count = vkb::to_u32(light_count);

			std::vector<vkb::Light> lights;
			for (auto &scene_light : scene_lights)
			{
				if (lights.size() < light_count)
				{
					const auto &properties = scene_light->get_properties();
					auto       &transform  = scene_light->get_node()->get_transform();

					vkb::Light light{{transform.get_translation(), static_cast<float>(scene_light->get_light_type())},
					                 {properties.color, properties.intensity},
					                 {transform.get_rotation() * properties.direction, properties.range},
					                 {properties.inner_cone_angle, properties.outer_cone_angle}};

					lights.push_back(light);
				}
			}

			std::copy(lights.begin(), lights.end(), light_info.lights);

			auto                 &render_frame = get_render_context().get_active_frame();
			vkb::BufferAllocation light_buffer = render_frame.allocate_buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(T));
			light_buffer.update(light_info);

			return light_buffer;
		};
	};

  private:
	vkb::sg::PerspectiveCamera *camera{nullptr};

	virtual void draw_gui() override;

	virtual void render(vkb::CommandBuffer &command_buffer) override;

	std::unique_ptr<vkb::RenderPipeline> create_specialization_renderpass();

	std::unique_ptr<vkb::RenderPipeline> create_standard_renderpass();

	std::unique_ptr<vkb::RenderPipeline> specialization_constants_pipeline{};

	std::unique_ptr<vkb::RenderPipeline> standard_pipeline{};

	int specialization_constants_enabled{0};
};

std::unique_ptr<vkb::VulkanSample> create_specialization_constants();
