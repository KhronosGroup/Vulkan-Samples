/* Copyright (c) 2019-2025, Arm Limited and Contributors
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

#include "common/helpers.h"
#include "common/vk_common.h"
#include "rendering/pipeline_state.h"

namespace vkb
{
class Pipeline
{
  public:
	Pipeline(vkb::core::DeviceC &device);

	Pipeline(const Pipeline &) = delete;

	Pipeline(Pipeline &&other);

	virtual ~Pipeline();

	Pipeline &operator=(const Pipeline &) = delete;

	Pipeline &operator=(Pipeline &&) = delete;

	VkPipeline get_handle() const;

	const PipelineState &get_state() const;

  protected:
	vkb::core::DeviceC &device;

	VkPipeline handle = VK_NULL_HANDLE;

	PipelineState state;
};

class ComputePipeline : public Pipeline
{
  public:
	ComputePipeline(ComputePipeline &&) = default;

	virtual ~ComputePipeline() = default;

	ComputePipeline(vkb::core::DeviceC &device,
	                VkPipelineCache     pipeline_cache,
	                PipelineState      &pipeline_state);
};

class GraphicsPipeline : public Pipeline
{
  public:
	GraphicsPipeline(GraphicsPipeline &&) = default;

	virtual ~GraphicsPipeline() = default;

	GraphicsPipeline(vkb::core::DeviceC &device,
	                 VkPipelineCache     pipeline_cache,
	                 PipelineState      &pipeline_state);
};
}        // namespace vkb
