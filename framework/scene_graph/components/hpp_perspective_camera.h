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

#pragma once

#include "hpp_camera.h"

namespace vkb
{
namespace scene_graph
{
namespace components
{
class HPPPerspectiveCamera : public vkb::scene_graph::components::HPPCamera
{
  public:
	HPPPerspectiveCamera(const std::string &name);

	virtual ~HPPPerspectiveCamera() = default;

  public:
	// from vkb::sg::Camera
	virtual glm::mat4 get_projection() override;

	float get_aspect_ratio() const;
	float get_far_plane() const;
	float get_near_plane() const;
	void  set_aspect_ratio(float new_aspect_ratio);
	void  set_far_plane(float zfar);
	void  set_field_of_view(float new_fov);
	void  set_near_plane(float znear);
	float get_field_of_view();

  private:
	float aspect_ratio = 1.0f;                       // Screen size aspect ratio
	float fov          = glm::radians(60.0f);        // Horizontal field of view in radians
	float far_plane    = 100.0;
	float near_plane   = 0.1f;
};
}        // namespace components
}        // namespace scene_graph
}        // namespace vkb
