/* Copyright (c) 2022-2023, NVIDIA CORPORATION. All rights reserved.
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

#include <hpp_vulkan_sample.h>

#include "imgui.h"
#include "scene_graph/components/camera.h"

#include "hpp_resource_cache.h"
#include "hpp_resource_record.h"
#include "hpp_resource_replay.h"

// A cache which hydrates its resources from an external source
class HPPPipelineCacheResourceCache : public vkb::HPPResourceCache
{
  public:
	vkb::core::HPPComputePipeline     &request_compute_pipeline(vkb::rendering::HPPPipelineState &pipeline_state) override;
	vkb::core::HPPDescriptorSet       &request_descriptor_set(vkb::core::HPPDescriptorSetLayout &descriptor_set_layout, const BindingMap<vk::DescriptorBufferInfo> &buffer_infos, const BindingMap<vk::DescriptorImageInfo> &image_infos) override;
	vkb::core::HPPDescriptorSetLayout &request_descriptor_set_layout(const uint32_t set_index, const std::vector<vkb::core::HPPShaderModule *> &shader_modules, const std::vector<vkb::core::HPPShaderResource> &set_resources) override;
	vkb::core::HPPFramebuffer         &request_framebuffer(const vkb::rendering::HPPRenderTarget &render_target, const vkb::core::HPPRenderPass &render_pass) override;
	vkb::core::HPPGraphicsPipeline    &request_graphics_pipeline(vkb::rendering::HPPPipelineState &pipeline_state) override;
	vkb::core::HPPPipelineLayout      &request_pipeline_layout(const std::vector<vkb::core::HPPShaderModule *> &shader_modules) override;
	vkb::core::HPPRenderPass          &request_render_pass(const std::vector<vkb::rendering::HPPAttachment> &attachments, const std::vector<vkb::common::HPPLoadStoreInfo> &load_store_infos, const std::vector<vkb::core::HPPSubpassInfo> &subpasses) override;
	vkb::core::HPPShaderModule        &request_shader_module(vk::ShaderStageFlagBits stage, const vkb::ShaderSource &glsl_source, const vkb::ShaderVariant &shader_variant = {}) override;

	void                 clear_pipelines();
	void                 set_pipeline_cache(vk::PipelineCache pipeline_cache);
	void                 warmup(std::vector<uint8_t> &&data);
	std::vector<uint8_t> serialize() const;

  private:
	vk::PipelineCache pipeline_cache{VK_NULL_HANDLE};

	vkb::HPPResourceRecord recorder;

	vkb::HPPResourceReplay replayer;
};

/**
 * @brief Pipeline creation and caching
 */
class HPPPipelineCache : public vkb::HPPVulkanSample
{
  public:
	HPPPipelineCache();
	virtual ~HPPPipelineCache();

  private:
	// from vkb::HPPVulkanSample
	virtual void draw_gui() override;
	virtual bool prepare(const vkb::ApplicationOptions &options) override;
	virtual void update(float delta_time) override;

  private:
	ImVec2            button_size           = ImVec2(150, 30);
	vkb::sg::Camera  *camera                = nullptr;
	bool              enable_pipeline_cache = true;
	vk::PipelineCache pipeline_cache;
	float             rebuild_pipelines_frame_time_ms = 0.0f;
	bool              record_frame_time_next_frame    = false;
};

std::unique_ptr<vkb::HPPVulkanSample> create_hpp_pipeline_cache();
