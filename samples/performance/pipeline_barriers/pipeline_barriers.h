/* Copyright (c) 2019-2024, Arm Limited and Contributors
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

#include "common/utils.h"
#include "rendering/render_pipeline.h"
#include "scene_graph/components/camera.h"
#include "vulkan_sample.h"

/**
 * @brief Using pipeline barriers efficiently
 */
class PipelineBarriers : public vkb::VulkanSample<vkb::BindingType::C>
{
  public:
	PipelineBarriers();

	virtual ~PipelineBarriers() = default;

	virtual bool prepare(const vkb::ApplicationOptions &options) override;

  private:
	enum DependencyType : int
	{
		BOTTOM_TO_TOP,
		FRAG_TO_VERT,
		FRAG_TO_FRAG
	};

	vkb::sg::Camera *camera{nullptr};

	std::unique_ptr<vkb::RenderTarget> create_render_target(vkb::core::Image &&swapchain_image);

	virtual void prepare_render_context() override;

	void draw(vkb::core::CommandBuffer<vkb::BindingType::C> &command_buffer, vkb::RenderTarget &render_target) override;

	virtual void draw_gui() override;

	vkb::RenderPipeline gbuffer_pipeline;

	vkb::RenderPipeline lighting_pipeline;

	DependencyType dependency_type{DependencyType::BOTTOM_TO_TOP};
};

std::unique_ptr<vkb::VulkanSample<vkb::BindingType::C>> create_pipeline_barriers();
