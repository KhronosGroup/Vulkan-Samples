/* Copyright (c) 2019-2021, Arm Limited and Contributors
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

VKBP_DISABLE_WARNINGS()
#include "common/glm_common.h"
VKBP_ENABLE_WARNINGS()

#include "scene_graph/script.h"

namespace vkb
{
namespace sg
{
class FreeCamera : public NodeScript
{
  public:
	static const float TOUCH_DOWN_MOVE_FORWARD_WAIT_TIME;

	static const float ROTATION_MOVE_WEIGHT;

	static const float KEY_ROTATION_MOVE_WEIGHT;

	static const float TRANSLATION_MOVE_WEIGHT;

	static const float TRANSLATION_MOVE_STEP;

	static const uint32_t TRANSLATION_MOVE_SPEED;

	explicit FreeCamera(Node &node);

	~FreeCamera() override = default;

	void update(float delta_time) override;

	void input_event(const InputEvent &input_event) override;

	void resize(uint32_t width, uint32_t height) override;

  private:
	float speed_multiplier{3.0f};

	glm::vec2 mouse_move_delta{0.0f};

	glm::vec2 mouse_last_pos{0.0f};

	glm::vec2 touch_move_delta{0.0f};

	glm::vec2 touch_last_pos{0.0f};

	float touch_pointer_time{0.0f};

	std::unordered_map<KeyCode, bool> key_pressed;

	std::unordered_map<MouseButton, bool> mouse_button_pressed;

	std::unordered_map<int32_t, bool> touch_pointer_pressed;
};
}        // namespace sg
}        // namespace vkb
