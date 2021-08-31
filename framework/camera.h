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

#pragma once

#include "common/error.h"

#include "common/glm.h"

namespace vkb
{
enum CameraType
{
	LookAt,
	FirstPerson
};

class Camera
{
  public:
	void update(float deltaTime);

	// Update camera passing separate axis data (gamepad)
	// Returns true if view or position has been changed
	bool update_gamepad(glm::vec2 axis_left, glm::vec2 axis_right, float delta_time);

	CameraType type = CameraType::LookAt;

	glm::vec3 rotation = glm::vec3();
	glm::vec3 position = glm::vec3();

	float rotation_speed    = 1.0f;
	float translation_speed = 1.0f;

	bool updated = false;

	struct
	{
		glm::mat4 perspective;
		glm::mat4 view;
	} matrices;

	struct
	{
		bool left  = false;
		bool right = false;
		bool up    = false;
		bool down  = false;
	} keys;

	bool moving() const;

	float get_near_clip() const;

	float get_far_clip() const;

	void set_perspective(float fov, float aspect, float znear, float zfar);

	void update_aspect_ratio(float aspect);

	void set_position(const glm::vec3 &position);

	void set_rotation(const glm::vec3 &rotation);

	void rotate(const glm::vec3 &delta);

	void set_translation(const glm::vec3 &translation);

	void translate(const glm::vec3 &delta);

  private:
	float fov;
	float znear, zfar;

	void update_view_matrix();
};
}        // namespace vkb
