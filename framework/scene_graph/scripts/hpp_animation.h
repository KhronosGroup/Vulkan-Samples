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

#include "animation.h"

namespace vkb
{
namespace scene_graph
{
namespace scripts
{
/**
 * @brief facade class around vkb::sg::Scene, providing a vulkan.hpp-based interface
 *
 * See vkb::sg::Scene for documentation
 */
class HPPAnimation : private vkb::sg::Animation
{
  public:
	using ComponentType = vkb::sg::Animation;

	using vkb::sg::Animation::update_times;

	HPPAnimation(const std::string &name = "") :
	    vkb::sg::Animation(name)
	{}

	void add_channel(vkb::scene_graph::HPPNode &node, const vkb::sg::AnimationTarget &target, const vkb::sg::AnimationSampler &sampler)
	{
		vkb::sg::Animation::add_channel(reinterpret_cast<vkb::sg::Node &>(node), target, sampler);
	}
};
}        // namespace scripts
}        // namespace scene_graph
}        // namespace vkb
