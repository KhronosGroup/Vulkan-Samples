/* Copyright (c) 2020-2021, Arm Limited and Contributors
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

#include "vulkan_sample.h"
#include <memory>

/**
 * @brief Using 16-bit arithmetic extension to improve arithmetic throughput
 */
class KHR16BitArithmeticSample : public vkb::VulkanSample
{
  public:
	KHR16BitArithmeticSample();

	virtual ~KHR16BitArithmeticSample() = default;

	virtual bool prepare(const vkb::ApplicationOptions &options) override;

	virtual void request_gpu_features(vkb::PhysicalDevice &gpu) override;

	virtual void draw_renderpass(vkb::CommandBuffer &cmd, vkb::RenderTarget &render_target) override;

  private:
	virtual void draw_gui() override;

	bool khr_16bit_arith_enabled{false};

	bool supported_extensions{false};
	bool supports_push_constant16{false};

	std::vector<vkb::LoadStoreInfo>            load_store_infos;
	std::vector<std::unique_ptr<vkb::Subpass>> subpasses;
	std::vector<VkClearValue>                  clear_values;

	std::unique_ptr<vkb::core::Buffer>    blob_buffer;
	std::unique_ptr<vkb::core::Image>     image;
	std::unique_ptr<vkb::core::ImageView> image_view;
	std::unique_ptr<vkb::core::Sampler>   sampler;
	vkb::PipelineLayout *                 compute_layout{nullptr};
	vkb::PipelineLayout *                 compute_layout_fp16{nullptr};

	unsigned frame_count{0};

	struct VisualizationSubpass : vkb::Subpass
	{
		VisualizationSubpass(vkb::RenderContext &context, vkb::ShaderSource &&vertex_source, vkb::ShaderSource &&fragment_source);
		virtual void prepare() override;
		virtual void draw(vkb::CommandBuffer &command_buffer) override;

		vkb::PipelineLayout *       layout{nullptr};
		const vkb::core::ImageView *view{nullptr};
		const vkb::core::Sampler *  sampler{nullptr};
	};
};

std::unique_ptr<vkb::VulkanSample> create_16bit_arithmetic();
