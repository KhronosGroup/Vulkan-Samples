/* Copyright (c) 2023, NVIDIA CORPORATION. All rights reserved.
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

#include "pbr_material.h"

namespace vkb
{
namespace scene_graph
{
namespace components
{
class HPPTexture;

/**
 * @brief facade class around vkb::sg::PBRMaterial, providing a vulkan.hpp-based interface
 *
 * See vkb::sg::PBRMaterial for documentation
 */
class HPPPBRMaterial : private vkb::sg::PBRMaterial
{
  public:
	using ComponentType = vkb::sg::PBRMaterial;

	using vkb::sg::PBRMaterial::alpha_cutoff;
	using vkb::sg::PBRMaterial::alpha_mode;
	using vkb::sg::PBRMaterial::base_color_factor;
	using vkb::sg::PBRMaterial::double_sided;
	using vkb::sg::PBRMaterial::emissive;
	using vkb::sg::PBRMaterial::get_name;
	using vkb::sg::PBRMaterial::metallic_factor;
	using vkb::sg::PBRMaterial::roughness_factor;

	HPPPBRMaterial(const std::string &name) :
	    vkb::sg::PBRMaterial(name)
	{}

	void set_texture(std::string const &name, vkb::scene_graph::components::HPPTexture *texture)
	{
		assert(vkb::sg::Material::textures.find(name) == vkb::sg::Material::textures.end());
		vkb::sg::PBRMaterial::textures[name] = reinterpret_cast<vkb::sg::Texture *>(texture);
	}
};

}        // namespace components
}        // namespace scene_graph
}        // namespace vkb
