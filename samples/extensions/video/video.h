/* Copyright (c) 2023, Holochip Inc.
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

#include <VkVideoRefCountBase.h>
#include <VulkanDeviceContext.h>

class FrameProcessor;
class video : public ApiVulkanSample
{
	VkPipeline            pipeline;
	VkPipelineLayout      pipeline_layout;
	VkDescriptorSet       descriptor_set;
	VkDescriptorSetLayout descriptor_set_layout;

	// Resources for the workers
	struct Resources
	{
		VkQueue         queue;
		VkCommandPool   command_pool;
		VkCommandBuffer command_buffer;

		VkPipelineLayout pipeline_layout;
		VkPipeline       init_pipeline;
		VkPipeline       update_pipeline;
		VkPipeline       mutate_pipeline;

		vkb::Timer timer;
		uint32_t   queue_family_index;
	} decode, encode, transfer, compute;


	VkQueueFlags           m_videoDecodeQueueFlags{}, m_videoEncodeQueueFlags{};
	int                    m_videoDecodeQueueFamily{};
	int					  m_videoDecodeNumQueues{};
	int                    m_videoEncodeQueueFamily{};
	int					  m_videoEncodeNumQueues{};
	int 					m_videoDecodeEncodeComputeQueueFamily{};
	int 					m_videoDecodeEncodeComputeNumQueues{};
	bool                   m_videoDecodeQueryResultStatusSupport{};
	bool                   m_videoEncodeQueryResultStatusSupport{};
	VkSharedBaseObj<FrameProcessor> m_frameProcessor;
	std::vector<BackBuffer> m_backBuffers;
	std::queue<AcquireBuffer*> m_acquireBuffers;

  public:
	video();
	~video() override;
	void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	bool prepare(const vkb::ApplicationOptions &options) override;
	void         build_command_buffers() override;

	void         draw();
	void render(float delta_time) override;
};

std::unique_ptr<vkb::VulkanSampleC> create_video();
