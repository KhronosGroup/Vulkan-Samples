/* Copyright (c) 2020, Arm Limited and Contributors
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

#include <functional>
#include <memory>
#include <string>
#include <typeinfo>
#include <vector>

#include "scene_graph/components/transform.h"
#include "scene_graph/script.h"

namespace vkb
{
namespace sg
{
enum AnimationType
{
	Linear,
	Step,
	CubicSpline
};

enum AnimationTarget
{
	Translation,
	Rotation,
	Scale
};

struct AnimationSampler
{
	AnimationType type{Linear};

	std::vector<float> inputs{};

	std::vector<glm::vec4> outputs{};
};

struct AnimationChannel
{
	Node &node;

	AnimationTarget target;

	AnimationSampler sampler;
};

class Animation : public Script
{
  public:
	Animation(const std::string &name = "");

	Animation(const Animation &);

	virtual void update(float delta_time) override;

	void update_times(float start_time, float end_time);

	void add_channel(Node &node, const AnimationTarget &target, const AnimationSampler &sampler);

  private:
	std::vector<AnimationChannel> channels;

	float current_time{0.0f};

	float start_time{std::numeric_limits<float>::max()};

	float end_time{std::numeric_limits<float>::min()};
};
}        // namespace sg
}        // namespace vkb
