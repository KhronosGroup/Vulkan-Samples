/* Copyright (c) 2019-2023, Arm Limited and Contributors
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

#include "perspective_camera.h"

VKBP_DISABLE_WARNINGS()
#include <glm/gtc/matrix_transform.hpp>
VKBP_ENABLE_WARNINGS()

namespace vkb
{
namespace sg
{
PerspectiveCamera::PerspectiveCamera(const std::string &name) :
    Camera{name}
{}

void PerspectiveCamera::set_field_of_view(float new_fov)
{
	fov = new_fov;
}

float PerspectiveCamera::get_far_plane() const
{
	return far_plane;
}

void PerspectiveCamera::set_far_plane(float zfar)
{
	far_plane = zfar;
}

float PerspectiveCamera::get_near_plane() const
{
	return near_plane;
}

void PerspectiveCamera::set_near_plane(float znear)
{
	near_plane = znear;
}

void PerspectiveCamera::set_aspect_ratio(float new_aspect_ratio)
{
	aspect_ratio = new_aspect_ratio;
}

float PerspectiveCamera::get_field_of_view()
{
	return fov;
}

float PerspectiveCamera::get_aspect_ratio()
{
	return aspect_ratio;
}

glm::mat4 PerspectiveCamera::get_projection()
{
	// Note: Using reversed depth-buffer for increased precision, so Znear and Zfar are flipped
	return glm::perspective(fov, aspect_ratio, far_plane, near_plane);
}
}        // namespace sg
}        // namespace vkb
