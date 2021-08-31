/* Copyright (c) 2019, Arm Limited and Contributors
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
class Device;

class Pipeline
{
  public:
	explicit Pipeline(Device &device);

	Pipeline(const Pipeline &) = delete;

	Pipeline(Pipeline &&other) noexcept ;

	virtual ~Pipeline();

	Pipeline &operator=(const Pipeline &) = delete;

	Pipeline &operator=(Pipeline &&) = delete;

	VkPipeline get_handle() const;

	const PipelineState &get_state() const;

  protected:
	Device &device;

	VkPipeline handle = VK_NULL_HANDLE;

	PipelineState state;
};

class ComputePipeline : public Pipeline
{
  public:
	ComputePipeline(ComputePipeline &&) = default;

	~ComputePipeline() override = default;

	ComputePipeline(Device &        device,
	                VkPipelineCache pipeline_cache,
	                PipelineState & pipeline_state);
};

class GraphicsPipeline : public Pipeline
{
  public:
	GraphicsPipeline(GraphicsPipeline &&) = default;

	~GraphicsPipeline() override = default;

	GraphicsPipeline(Device &        device,
	                 VkPipelineCache pipeline_cache,
	                 PipelineState & pipeline_state);
};
}        // namespace vkb
