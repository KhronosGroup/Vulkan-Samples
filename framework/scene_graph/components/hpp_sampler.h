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

#include "sampler.h"

namespace vkb
{
namespace scene_graph
{
namespace components
{
/**
 * @brief facade class around vkb::sg::Sampler, providing a vulkan.hpp-based interface
 *
 * See vkb::sg::Sampler for documentation
 */
class HPPSampler : private vkb::sg::Sampler
{
  public:
	using ComponentType = vkb::sg::Sampler;

	using vkb::sg::Sampler::get_name;

	HPPSampler(const std::string &name, vkb::core::HPPSampler &&vk_sampler) :
	    vkb::sg::Sampler(name, reinterpret_cast<vkb::core::Sampler &&>(std::forward<vkb::core::HPPSampler>(vk_sampler)))
	{}
};
}        // namespace components
}        // namespace scene_graph
}        // namespace vkb
