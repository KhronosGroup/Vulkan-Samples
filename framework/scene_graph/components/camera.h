/* Copyright (c) 2019, Arm Limited and Contributors
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
#include <vector>

#include "common/error.h"

VKBP_DISABLE_WARNINGS()
#include "common/glm_common.h"
VKBP_ENABLE_WARNINGS()

#include "common/helpers.h"
#include "scene_graph/component.h"

namespace vkb
{
namespace sg
{
class Camera : public Component
{
  public:
	Camera(const std::string &name);

	virtual ~Camera() = default;

	virtual std::type_index get_type() override;

	virtual glm::mat4 get_projection() = 0;

	glm::mat4 get_view();

	void set_node(Node &node);

	Node *get_node();

	void set_pre_rotation(const glm::mat4 &pre_rotation);

  private:
	Node *node{nullptr};

	glm::mat4 pre_rotation{1.0f};
};
}        // namespace sg
}        // namespace vkb
