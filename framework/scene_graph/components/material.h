/* Copyright (c) 2018-2019, Arm Limited and Contributors
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

#include "common/glm.h"
#include "scene_graph/component.h"

namespace vkb
{
namespace sg
{
class Texture;

/**
 * @brief How the alpha value of the main factor and texture should be interpreted
 */
enum class AlphaMode
{
	/// Alpha value is ignored
	Opaque,
	/// Either full opaque or fully transparent
	Mask,
	/// Output is combined with the background
	Blend
};

class Material : public Component
{
  public:
	explicit Material(const std::string &name);

	Material(Material &&other) = default;

	~Material() override = default;

	std::type_index get_type() override;

	std::unordered_map<std::string, Texture *> textures;

	/// Emissive color of the material
	glm::vec3 emissive{0.0f, 0.0f, 0.0f};

	/// Whether the material is double sided
	bool double_sided{false};

	/// Cutoff threshold when in Mask mode
	float alpha_cutoff{0.5f};

	/// Alpha rendering mode
	AlphaMode alpha_mode{AlphaMode::Opaque};
};

}        // namespace sg
}        // namespace vkb
