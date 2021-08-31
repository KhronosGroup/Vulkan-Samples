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

#pragma once

#include "common/error.h"

VKBP_DISABLE_WARNINGS()
#include "common/glm_common.h"
VKBP_ENABLE_WARNINGS()

#include "scene_graph/components/camera.h"

namespace vkb
{
namespace sg
{
class OrthographicCamera : public Camera
{
  public:
	explicit OrthographicCamera(const std::string &name);

	OrthographicCamera(const std::string &name, float left, float right, float bottom, float top, float near_plane, float far_plane);

	~OrthographicCamera() override = default;

	void set_left(float left);

	float get_left() const;

	void set_right(float right);

	float get_right() const;

	void set_bottom(float bottom);

	float get_bottom() const;

	void set_top(float top);

	float get_top() const;

	void set_near_plane(float near_plane);

	float get_near_plane() const;

	void set_far_plane(float far_plane);

	float get_far_plane() const;

	glm::mat4 get_projection() override;

  private:
	float left{-1.0f};

	float right{1.0f};

	float bottom{-1.0f};

	float top{1.0f};

	float near_plane{0.0f};

	float far_plane{1.0f};
};
}        // namespace sg
}        // namespace vkb