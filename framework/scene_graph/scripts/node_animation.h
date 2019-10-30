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

#include <functional>
#include <memory>
#include <string>
#include <typeinfo>
#include <vector>

#include "scene_graph/components/transform.h"
#include "scene_graph/script.h"

/**
 *	@param std::shared_ptr<vkb::sg::Transform> The transform to animate
 *  @param float The delta time of the frame to scale animations
 */
using TransformAnimFn = std::function<void(vkb::sg::Transform &, float)>;

namespace vkb
{
namespace sg
{
class NodeAnimation : public Script
{
  public:
	NodeAnimation(Node &node, TransformAnimFn animation_fn);

	virtual ~NodeAnimation() = default;

	virtual void update(float delta_time) override;

	void set_animation(TransformAnimFn handle);

	void clear_animation();

  private:
	TransformAnimFn animation_fn{};
};
}        // namespace sg
}        // namespace vkb
