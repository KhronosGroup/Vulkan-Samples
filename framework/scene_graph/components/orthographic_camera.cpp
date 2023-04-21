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

#include "orthographic_camera.h"

VKBP_DISABLE_WARNINGS()
#include <glm/gtc/matrix_transform.hpp>
VKBP_ENABLE_WARNINGS()

namespace vkb
{
namespace sg
{
OrthographicCamera::OrthographicCamera(const std::string &name) :
    Camera{name}
{}

OrthographicCamera::OrthographicCamera(const std::string &name, float left, float right, float bottom, float top, float near_plane, float far_plane) :
    Camera{name},
    left{left},
    right{right},
    top{top},
    bottom{bottom},
    near_plane{near_plane},
    far_plane{far_plane}
{
}

void OrthographicCamera::set_left(float new_left)
{
	left = new_left;
}

float OrthographicCamera::get_left() const
{
	return left;
}

void OrthographicCamera::set_right(float new_right)
{
	right = new_right;
}

float OrthographicCamera::get_right() const
{
	return right;
}

void OrthographicCamera::set_bottom(float new_bottom)
{
	bottom = new_bottom;
}

float OrthographicCamera::get_bottom() const
{
	return bottom;
}

void OrthographicCamera::set_top(float new_top)
{
	top = new_top;
}

float OrthographicCamera::get_top() const
{
	return top;
}

void OrthographicCamera::set_near_plane(float new_near_plane)
{
	near_plane = new_near_plane;
}

float OrthographicCamera::get_near_plane() const
{
	return near_plane;
}

void OrthographicCamera::set_far_plane(float new_far_plane)
{
	far_plane = new_far_plane;
}

float OrthographicCamera::get_far_plane() const
{
	return far_plane;
}

glm::mat4 OrthographicCamera::get_projection()
{
	// Note: Using Reversed depth-buffer for increased precision, so Znear and Zfar are flipped
	return glm::ortho(left, right, bottom, top, far_plane, near_plane);
}
}        // namespace sg
}        // namespace vkb
