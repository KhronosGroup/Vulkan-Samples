/* Copyright (c) 2019, Sascha Willems
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

#include "frustum.h"

namespace vkb
{
void Frustum::update(const glm::mat4 &matrix)
{
	planes[LEFT].x = matrix[0].w + matrix[0].x;
	planes[LEFT].y = matrix[1].w + matrix[1].x;
	planes[LEFT].z = matrix[2].w + matrix[2].x;
	planes[LEFT].w = matrix[3].w + matrix[3].x;

	planes[RIGHT].x = matrix[0].w - matrix[0].x;
	planes[RIGHT].y = matrix[1].w - matrix[1].x;
	planes[RIGHT].z = matrix[2].w - matrix[2].x;
	planes[RIGHT].w = matrix[3].w - matrix[3].x;

	planes[TOP].x = matrix[0].w - matrix[0].y;
	planes[TOP].y = matrix[1].w - matrix[1].y;
	planes[TOP].z = matrix[2].w - matrix[2].y;
	planes[TOP].w = matrix[3].w - matrix[3].y;

	planes[BOTTOM].x = matrix[0].w + matrix[0].y;
	planes[BOTTOM].y = matrix[1].w + matrix[1].y;
	planes[BOTTOM].z = matrix[2].w + matrix[2].y;
	planes[BOTTOM].w = matrix[3].w + matrix[3].y;

	planes[BACK].x = matrix[0].w + matrix[0].z;
	planes[BACK].y = matrix[1].w + matrix[1].z;
	planes[BACK].z = matrix[2].w + matrix[2].z;
	planes[BACK].w = matrix[3].w + matrix[3].z;

	planes[FRONT].x = matrix[0].w - matrix[0].z;
	planes[FRONT].y = matrix[1].w - matrix[1].z;
	planes[FRONT].z = matrix[2].w - matrix[2].z;
	planes[FRONT].w = matrix[3].w - matrix[3].z;

	for (auto & plane : planes)
	{
		float length = sqrtf(plane.x * plane.x + plane.y * plane.y + plane.z * plane.z);
		plane /= length;
	}
}

bool Frustum::check_sphere(glm::vec3 pos, float radius)
{
	for (auto & plane : planes)
	{
		if ((plane.x * pos.x) + (plane.y * pos.y) + (plane.z * pos.z) + plane.w <= -radius)
		{
			return false;
		}
	}
	return true;
}
const std::array<glm::vec4, 6> &Frustum::get_planes() const
{
	return planes;
}
}        // namespace vkb
