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

namespace vkb
{
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

class HPPComputePipeline : public HPPPipeline
{
};

class HPPGraphicsPipeline : public HPPPipeline
{
};
}        // namespace core
}        // namespace vkb
