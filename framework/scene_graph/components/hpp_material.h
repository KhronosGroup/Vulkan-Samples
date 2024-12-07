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

#include "hpp_texture.h"
#include "material.h"

namespace vkb
{
namespace scene_graph
{
namespace components
{
/**
 * @brief facade class around vkb::sg::Material, providing a vulkan.hpp-based interface
 *
 * See vkb::sb::Material for documentation
 */
class HPPMaterial : private vkb::sg::Material
{
  public:
	std::unordered_map<std::string, vkb::scene_graph::components::HPPTexture *> const &get_textures() const
	{
		return reinterpret_cast<std::unordered_map<std::string, vkb::scene_graph::components::HPPTexture *> const &>(vkb::sg::Material::textures);
	}
};
}        // namespace components
}        // namespace scene_graph
}        // namespace vkb
