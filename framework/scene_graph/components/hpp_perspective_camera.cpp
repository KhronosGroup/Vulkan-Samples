/* Copyright (c) 2024, NVIDIA CORPORATION. All rights reserved.
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

#include "hpp_perspective_camera.h"

namespace vkb
{
namespace scene_graph
{
namespace components
{
HPPPerspectiveCamera::HPPPerspectiveCamera(const std::string &name) :
    vkb::scene_graph::components::HPPCamera(name)
{}

glm::mat4 HPPPerspectiveCamera::get_projection()
{
	// Note: Using reversed depth-buffer for increased precision, so Znear and Zfar are flipped
	return glm::perspective(fov, aspect_ratio, far_plane, near_plane);
}

float HPPPerspectiveCamera::get_aspect_ratio() const
{
	return aspect_ratio;
}

float HPPPerspectiveCamera::get_far_plane() const
{
	return far_plane;
}

float HPPPerspectiveCamera::get_near_plane() const
{
	return near_plane;
}

void HPPPerspectiveCamera::set_aspect_ratio(float new_aspect_ratio)
{
	aspect_ratio = new_aspect_ratio;
}

void HPPPerspectiveCamera::set_far_plane(float zfar)
{
	far_plane = zfar;
}

void HPPPerspectiveCamera::set_field_of_view(float new_fov)
{
	fov = new_fov;
}

void HPPPerspectiveCamera::set_near_plane(float znear)
{
	near_plane = znear;
}

float HPPPerspectiveCamera::get_field_of_view()
{
	return fov;
}
}        // namespace components
}        // namespace scene_graph
}        // namespace vkb
