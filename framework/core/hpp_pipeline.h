/* Copyright (c) 2023, NVIDIA CORPORATION. All rights reserved.
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

#include "core/pipeline.h"

#include "core/hpp_device.h"

namespace vkb
{
namespace rendering
{
class HPPPipelineState;
}

namespace core
{
/**
 * @brief facade class around vkb::Pipeline, providing a vulkan.hpp-based interface
 *
 * See vkb::Pipeline for documentation
 */
class HPPPipeline : private vkb::Pipeline
{
  public:
	vk::Pipeline get_handle() const
	{
		return static_cast<vk::Pipeline>(vkb::Pipeline::get_handle());
	}
};

class HPPComputePipeline : private vkb::ComputePipeline
{
  public:
	HPPComputePipeline(vkb::core::HPPDevice &device, vk::PipelineCache pipeline_cache, vkb::rendering::HPPPipelineState &pipeline_state) :
	    vkb::ComputePipeline(
	        reinterpret_cast<vkb::Device &>(device), static_cast<VkPipelineCache>(pipeline_cache), reinterpret_cast<vkb::PipelineState &>(pipeline_state))
	{}

	vk::Pipeline get_handle() const
	{
		return static_cast<vk::Pipeline>(vkb::ComputePipeline::get_handle());
	}
};

class HPPGraphicsPipeline : private vkb::GraphicsPipeline
{
  public:
	HPPGraphicsPipeline(vkb::core::HPPDevice &device, vk::PipelineCache pipeline_cache, vkb::rendering::HPPPipelineState &pipeline_state) :
	    vkb::GraphicsPipeline(
	        reinterpret_cast<vkb::Device &>(device), static_cast<VkPipelineCache>(pipeline_cache), reinterpret_cast<vkb::PipelineState &>(pipeline_state))
	{}

	vk::Pipeline get_handle() const
	{
		return static_cast<vk::Pipeline>(vkb::GraphicsPipeline::get_handle());
	}
};
}        // namespace core
}        // namespace vkb
