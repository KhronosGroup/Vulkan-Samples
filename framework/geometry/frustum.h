/* Copyright (c) 2019-2022, Sascha Willems
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

#include <array>

#include "common/error.h"

VKBP_DISABLE_WARNINGS()
#include "common/glm_common.h"
VKBP_ENABLE_WARNINGS()

namespace vkb
{
enum Side
{
	LEFT   = 0,
	RIGHT  = 1,
	TOP    = 2,
	BOTTOM = 3,
	BACK   = 4,
	FRONT  = 5
};

/**
 * @brief Represents a matrix by extracting its planes. Responsible for doing 
 * intersection tests
 */
class Frustum
{
  public:
	/**
	 * @brief Updates the frustums planes based on a matrix
	 * @param matrix The matrix to update the frustum on
	 */
	void update(const glm::mat4 &matrix);

	/**
	 * @brief Checks if a sphere is inside the Frustum
	 * @param pos The center of the sphere
	 * @param radius The radius of the sphere
	 */
	bool check_sphere(glm::vec3 pos, float radius);

	const std::array<glm::vec4, 6> &get_planes() const;

  private:
	std::array<glm::vec4, 6> planes;
};
}        // namespace vkb
