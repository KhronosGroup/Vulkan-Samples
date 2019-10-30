/* Copyright (c) 2018-2019, Arm Limited and Contributors
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

#include <memory>
#include <string>
#include <typeinfo>
#include <vector>

#include "common/error.h"

VKBP_DISABLE_WARNINGS()
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
VKBP_ENABLE_WARNINGS()

#include "scene_graph/component.h"

namespace vkb
{
namespace sg
{
class Node;

class Transform : public Component
{
  public:
	Transform(Node &node);

	virtual ~Transform() = default;

	Node &get_node();

	virtual std::type_index get_type() override;

	void set_translation(const glm::vec3 &translation);

	void set_rotation(const glm::quat &rotation);

	void set_scale(const glm::vec3 &scale);

	const glm::vec3 &get_translation() const;

	const glm::quat &get_rotation() const;

	const glm::vec3 &get_scale() const;

	void set_matrix(const glm::mat4 &matrix);

	glm::mat4 get_matrix() const;

	glm::mat4 get_world_matrix();

	/**
	 * @brief Marks the world transform invalid if any of
	 *        the local transform are changed or the parent
	 *        world transform has changed.
	 */
	void invalidate_world_matrix();

  private:
	Node &node;

	glm::vec3 translation = glm::vec3(0.0, 0.0, 0.0);

	glm::quat rotation = glm::quat(1.0, 0.0, 0.0, 0.0);

	glm::vec3 scale = glm::vec3(1.0, 1.0, 1.0);

	glm::mat4 world_matrix = glm::mat4(1.0);

	bool update_world_matrix = false;

	void update_world_transform();
};

}        // namespace sg
}        // namespace vkb
