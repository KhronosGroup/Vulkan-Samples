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

#include "platform/input_events.h"
#include "scene_graph/component.h"

namespace vkb
{
namespace sg
{
/**
 * @brief Generic structure to receive platform events.
 *        Used for adding game logic to scene graph objects.
 */
class Script : public Component
{
  public:
	Script(const std::string &name = "");

	virtual ~Script() = default;

	virtual std::type_index get_type() override;

	/**
	 * @brief Main loop script events
	 */
	virtual void update(float delta_time) = 0;

	virtual void input_event(const InputEvent &input_event);

	virtual void resize(uint32_t width, uint32_t height);
};

class NodeScript : public Script
{
  public:
	NodeScript(Node &node, const std::string &name = "");

	virtual ~NodeScript() = default;

	Node &get_node();

  private:
	Node &node;
};
}        // namespace sg
}        // namespace vkb
