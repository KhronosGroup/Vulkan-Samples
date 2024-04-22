/* Copyright (c) 2019-2024, Arm Limited and Contributors
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
#include <unordered_map>
#include <vector>

#include "common/error.h"

#include "common/glm_common.h"

#include "scene_graph/components/camera.h"

namespace vkb
{
namespace sg
{
class PerspectiveCamera : public Camera
{
  public:
	PerspectiveCamera(const std::string &name);

	virtual ~PerspectiveCamera() = default;

	void set_aspect_ratio(float aspect_ratio);

	void set_field_of_view(float fov);

	float get_far_plane() const;

	void set_far_plane(float zfar);

	float get_near_plane() const;

	void set_near_plane(float znear);

	float get_aspect_ratio();

	float get_field_of_view();

	virtual glm::mat4 get_projection() override;

  private:
	/**
	 * @brief Screen size aspect ratio
	 */
	float aspect_ratio{1.0f};

	/**
	 * @brief Horizontal field of view in radians
	 */
	float fov{glm::radians(60.0f)};

	float far_plane{100.0};

	float near_plane{0.1f};
};
}        // namespace sg
}        // namespace vkb
