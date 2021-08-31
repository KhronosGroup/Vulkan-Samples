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

#include "common/glm.h"
#include "scene_graph/component.h"
#include "scene_graph/components/sub_mesh.h"

namespace vkb
{
namespace sg
{
/**
 * @brief Axis Aligned Bounding Box
 */
class AABB : public Component
{
  public:
	AABB();

	AABB(const glm::vec3 &min, const glm::vec3 &max);

	~AABB() override = default;

	std::type_index get_type() override;

	/**
	 * @brief Update the bounding box based on the given vertex position
	 * @param point The 3D position of a point
	 */
	void update(const glm::vec3 &point);

	/**
	 * @brief Update the bounding box based on the given submesh vertices
	 * @param vertex_data The position vertex data
	 * @param index_data The index vertex data
	 */
	void update(const std::vector<glm::vec3> &vertex_data, const std::vector<uint16_t> &index_data);

	/**
	 * @brief Apply a given matrix transformation to the bounding box
	 * @param transform The matrix transform to apply
	 */
	void transform(glm::mat4 &transform);

	/**
	 * @brief Scale vector of the bounding box
	 * @return vector in 3D space
	 */
	glm::vec3 get_scale() const;

	/**
	 * @brief Center position of the bounding box
	 * @return vector in 3D space
	 */
	glm::vec3 get_center() const;

	/**
	 * @brief Minimum position of the bounding box
	 * @return vector in 3D space
	 */
	glm::vec3 get_min() const;

	/**
	 * @brief Maximum position of the bounding box
	 * @return vector in 3D space
	 */
	glm::vec3 get_max() const;

	/**
	 * @brief Resets the min and max position coordinates
	 */
	void reset();

  private:
	glm::vec3 min;

	glm::vec3 max;
};
}        // namespace sg
}        // namespace vkb
