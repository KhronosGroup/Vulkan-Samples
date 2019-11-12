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
#include "rendering/render_pipeline.h"
#include "rendering/subpasses/forward_subpass.h"
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

	virtual bool prepare(vkb::Platform &platform) override;

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
