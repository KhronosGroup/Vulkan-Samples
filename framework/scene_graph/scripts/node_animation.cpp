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

#include "node_animation.h"

#include "common/error.h"

VKBP_DISABLE_WARNINGS()
#include "common/glm_common.h"
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>
VKBP_ENABLE_WARNINGS()

#include "scene_graph/components/perspective_camera.h"
#include "scene_graph/components/transform.h"
#include "scene_graph/node.h"

namespace vkb
{
namespace sg
{
NodeAnimation::NodeAnimation(Node &node, TransformAnimFn animation_fn) :
    Script{node, ""},
    animation_fn{animation_fn}
{
}

void NodeAnimation::update(float delta_time)
{
	if (animation_fn)
	{
		animation_fn(get_node().get_component<Transform>(), delta_time);
	}
}

void NodeAnimation::set_animation(TransformAnimFn handle)
{
	animation_fn = handle;
}

void NodeAnimation::clear_animation()
{
	animation_fn = {};
}
}        // namespace sg
}        // namespace vkb