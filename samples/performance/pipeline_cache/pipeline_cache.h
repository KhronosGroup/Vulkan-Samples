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

#include "common/utils.h"
#include "common/vk_common.h"
#include "rendering/render_pipeline.h"
#include "scene_graph/components/camera.h"
#include "vulkan_sample.h"

#include "resource_cache.h"
#include "resource_record.h"
#include "resource_replay.h"

class PipelineCacheResourceCache : public vkb::ResourceCache
{
  public:
	vkb::ShaderModule        &request_shader_module(VkShaderStageFlagBits stage, const vkb::ShaderSource &glsl_source, const vkb::ShaderVariant &shader_variant = {}) override;
	vkb::PipelineLayout      &request_pipeline_layout(const std::vector<vkb::ShaderModule *> &shader_modules) override;
	vkb::DescriptorSetLayout &request_descriptor_set_layout(const uint32_t set_index, const std::vector<vkb::ShaderModule *> &shader_modules, const std::vector<vkb::ShaderResource> &set_resources) override;
	vkb::GraphicsPipeline    &request_graphics_pipeline(vkb::PipelineState &pipeline_state) override;
	vkb::ComputePipeline     &request_compute_pipeline(vkb::PipelineState &pipeline_state) override;
	vkb::DescriptorSet       &request_descriptor_set(vkb::DescriptorSetLayout &descriptor_set_layout, const BindingMap<VkDescriptorBufferInfo> &buffer_infos, const BindingMap<VkDescriptorImageInfo> &image_infos) override;
	vkb::RenderPass          &request_render_pass(const std::vector<vkb::Attachment> &attachments, const std::vector<vkb::LoadStoreInfo> &load_store_infos, const std::vector<vkb::SubpassInfo> &subpasses) override;
	vkb::Framebuffer         &request_framebuffer(const vkb::RenderTarget &render_target, const vkb::RenderPass &render_pass) override;

	void                 clear_pipelines();
	void                 set_pipeline_cache(VkPipelineCache pipeline_cache);
	void                 warmup(std::vector<uint8_t> &&data);
	std::vector<uint8_t> serialize() const;

  private:
	VkPipelineCache pipeline_cache{VK_NULL_HANDLE};

	vkb::ResourceRecord recorder;

	vkb::ResourceReplay replayer;
};

/**
 * @brief Pipeline creation and caching
 */
class PipelineCache : public vkb::VulkanSample
{
  public:
	PipelineCache();

	virtual ~PipelineCache();

	virtual bool prepare(const vkb::ApplicationOptions &options) override;

	virtual void update(float delta_time) override;

  private:
	vkb::sg::Camera *camera{nullptr};

	VkPipelineCache pipeline_cache{VK_NULL_HANDLE};

	ImVec2 button_size{150, 30};

	bool enable_pipeline_cache{true};

	bool record_frame_time_next_frame{false};

	float rebuild_pipelines_frame_time_ms{0.0f};

	virtual void draw_gui() override;
};

std::unique_ptr<vkb::VulkanSample> create_pipeline_cache();
