/* Copyright (c) 2018-2021, Arm Limited and Contributors
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

#include "transform.h"

VKBP_DISABLE_WARNINGS()
#include "common/glm_common.h"
#include <glm/gtx/matrix_decompose.hpp>
VKBP_ENABLE_WARNINGS()

#include "scene_graph/node.h"

namespace vkb
{
namespace sg
{
Transform::Transform(Node &n) :
    node{n}
{
}

Node &Transform::get_node()
{
	return node;
}

std::type_index Transform::get_type()
{
	return typeid(Transform);
}

void Transform::set_translation(const glm::vec3 &new_translation)
{
	translation = new_translation;

	invalidate_world_matrix();
}

void Transform::set_rotation(const glm::quat &new_rotation)
{
	rotation = new_rotation;

	invalidate_world_matrix();
}

void Transform::set_scale(const glm::vec3 &new_scale)
{
	scale = new_scale;

	invalidate_world_matrix();
}

const glm::vec3 &Transform::get_translation() const
{
	return translation;
}

const glm::quat &Transform::get_rotation() const
{
	return rotation;
}

const glm::vec3 &Transform::get_scale() const
{
	return scale;
}

void Transform::set_matrix(const glm::mat4 &matrix)
{
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(matrix, scale, rotation, translation, skew, perspective);

	invalidate_world_matrix();
}

glm::mat4 Transform::get_matrix() const
{
	return glm::translate(glm::mat4(1.0), translation) *
	       glm::mat4_cast(rotation) *
	       glm::scale(glm::mat4(1.0), scale);
}

glm::mat4 Transform::get_world_matrix()
{
	update_world_transform();

	return world_matrix;
}

void Transform::invalidate_world_matrix()
{
	update_world_matrix = true;
}

void Transform::update_world_transform()
{
	if (!update_world_matrix)
	{
		return;
	}

	world_matrix = get_matrix();

	auto parent = node.get_parent();

	if (parent)
	{
		auto &transform = parent->get_component<Transform>();
		world_matrix    = transform.get_world_matrix() * world_matrix;
	}

	update_world_matrix = false;
}

}        // namespace sg
}        // namespace vkb
