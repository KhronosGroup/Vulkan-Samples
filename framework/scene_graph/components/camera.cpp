/* Copyright (c) 2019-2020, Arm Limited and Contributors
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

#include "camera.h"

#include "scene_graph/components/transform.h"
#include "scene_graph/node.h"

namespace vkb
{
namespace sg
{
Camera::Camera(const std::string &name) :
    Component{name}
{}

std::type_index Camera::get_type()
{
	return typeid(Camera);
}

glm::mat4 Camera::get_view()
{
	if (!node)
	{
		throw std::runtime_error{"Camera component is not attached to a node"};
	}

	auto &transform = node->get_component<Transform>();
	return glm::inverse(transform.get_world_matrix());
}

void Camera::set_node(Node &n)
{
	node = &n;
}

Node *Camera::get_node()
{
	return node;
}

glm::mat4 Camera::get_pre_rotation()
{
	return pre_rotation;
}

void Camera::set_pre_rotation(const glm::mat4 &pr)
{
	pre_rotation = pr;
}
}        // namespace sg
}        // namespace vkb