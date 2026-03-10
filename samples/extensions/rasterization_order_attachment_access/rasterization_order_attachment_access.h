/* Copyright (c) 2026, Arm Limited and Contributors
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

#include "api_vulkan_sample.h"
#include <vector>

/**
 * @brief Demonstrates VK_EXT_rasterization_order_attachment_access
 *
 * This sample shows how ROAA enables efficient deterministic transparency by
 * guaranteeing per-fragment ordering for attachment reads/writes. Without ROAA,
 * achieving correct blending requires splitting draws with barriers between each,
 * resulting in significant performance overhead.
 *
 * The sample renders overlapping transparent spheres using framebuffer fetch
 * (VK_KHR_dynamic_rendering_local_read) and compares two approaches:
 * - ROAA ON:  Single instanced draw call (extension guarantees ordering)
 * - ROAA OFF: N draw calls with N-1 barriers (manual synchronization)
 */
class RasterizationOrderAttachmentAccess : public ApiVulkanSample
{
  public:
	// ============================================================================
	// Initialization
	// ============================================================================

	RasterizationOrderAttachmentAccess();
	bool prepare(const vkb::ApplicationOptions &options) override;
	void request_gpu_features(vkb::core::PhysicalDeviceC &gpu) override;

	// Dynamic rendering bypasses render passes and framebuffers
	void setup_render_pass() override
	{}

	void setup_framebuffer() override
	{}

	// ============================================================================
	// Runtime
	// ============================================================================

	void render(float delta_time) override;
	void build_command_buffers() override;
	bool resize(const uint32_t width, const uint32_t height) override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;

	uint32_t get_api_version() const override
	{
		return VK_API_VERSION_1_2;
	}

	// ============================================================================
	// Teardown
	// ============================================================================

	~RasterizationOrderAttachmentAccess() override;

  private:
	// ============================================================================
	// Initialization
	// ============================================================================

	void load_assets();
	void create_buffers();
	void create_descriptors();
	void create_pipelines();
	void create_timestamp_query_pool();

	// ============================================================================
	// Runtime
	// ============================================================================

	void update_scene_uniforms();
	void update_instance_buffer();
	void retrieve_timestamp_results();

	// ============================================================================
	// Constants
	// ============================================================================

	static constexpr uint32_t kInstanceRowCount    = 4;
	static constexpr uint32_t kInstanceColumnCount = 4;
	static constexpr uint32_t kInstanceLayerCount  = 4;
	static constexpr uint32_t kInstanceCount       = kInstanceRowCount * kInstanceColumnCount * kInstanceLayerCount;

	// Cull mode for both blend pipelines. Switch to VK_CULL_MODE_NONE to render both faces
	static constexpr VkCullModeFlags kBlendCullMode = VK_CULL_MODE_BACK_BIT;

	// ============================================================================
	// GPU data structures
	// ============================================================================

	/// Scene-wide uniforms: camera matrices and background settings
	struct SceneUniforms
	{
		glm::mat4 projection;
		glm::mat4 view;
		glm::f32  background_grayscale;
	};

	/// Per-instance data: transform and color for each sphere
	struct InstanceData
	{
		glm::mat4 model;
		glm::vec4 color;
	};

	// ============================================================================
	// Vulkan resources
	// ============================================================================

	std::unique_ptr<vkb::sg::SubMesh> sphere_mesh;
	Texture                           background_texture;

	std::unique_ptr<vkb::core::BufferC> scene_uniform_buffer;
	std::unique_ptr<vkb::core::BufferC> instance_buffer;
	std::vector<InstanceData>           instances;

	VkDescriptorSetLayout        descriptor_set_layout = VK_NULL_HANDLE;
	VkDescriptorPool             descriptor_pool       = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet> descriptor_sets;

	VkPipelineLayout pipeline_layout        = VK_NULL_HANDLE;
	VkPipeline       background_pipeline    = VK_NULL_HANDLE;
	VkPipeline       blend_pipeline_roaa    = VK_NULL_HANDLE;
	VkPipeline       blend_pipeline_no_roaa = VK_NULL_HANDLE;

	// Maps color attachment 0 to input_attachment_index 0 in the fragment shader
	static constexpr uint32_t                               COLOR_ATTACHMENT_INPUT_INDEX = 0;
	static inline const VkRenderingInputAttachmentIndexInfo INPUT_ATTACHMENT_INDEX_INFO{
	    VK_STRUCTURE_TYPE_RENDERING_INPUT_ATTACHMENT_INDEX_INFO_KHR, nullptr, 1, &COLOR_ATTACHMENT_INPUT_INDEX, nullptr, nullptr};

	// ============================================================================
	// Performance measurement
	// ============================================================================

	VkQueryPool timestamp_query_pool = VK_NULL_HANDLE;

	bool supports_timestamp_queries() const
	{
		return timestamp_query_pool != VK_NULL_HANDLE;
	}

	std::vector<uint64_t> timestamp_results;
	float                 gpu_draw_time_ms = 0.0f;
	uint32_t              draw_call_count  = 0;
	uint32_t              barrier_count    = 0;

	// ============================================================================
	// UI state
	// ============================================================================

	bool roaa_supported = false;
	bool roaa_enabled   = false;
};

std::unique_ptr<vkb::VulkanSampleC> create_rasterization_order_attachment_access();
