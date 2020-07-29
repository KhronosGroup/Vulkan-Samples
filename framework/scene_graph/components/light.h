/* Copyright (c) 2018-2020, Arm Limited and Contributors
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

#include "core/shader_module.h"
#include "scene_graph/component.h"

namespace vkb
{
namespace sg
{
enum LightType
{
	Directional = 0,
	Point       = 1,
	Spot        = 2,
	// Insert new lightype here
	Max
};

struct LightProperties
{
	glm::vec3 direction{0.0f, 0.0f, -1.0f};

	glm::vec3 color{1.0f, 1.0f, 1.0f};

	float intensity{1.0f};

	float range{0.0f};

	float inner_cone_angle{0.0f};

	float outer_cone_angle{0.0f};
};

class Light : public Component
{
  public:
	Light(const std::string &name);

	Light(Light &&other) = default;

	virtual ~Light() = default;

	virtual std::type_index get_type() override;

	void set_node(Node &node);

	Node *get_node();

	void set_light_type(const LightType &type);

	const LightType &get_light_type();

	void set_properties(const LightProperties &properties);

	const LightProperties &get_properties();

  private:
	Node *node{nullptr};

	LightType light_type;

	LightProperties properties;
};

}        // namespace sg
}        // namespace vkb
