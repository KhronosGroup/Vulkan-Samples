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

#include "texture.h"

namespace vkb
{
namespace scene_graph
{
namespace components
{
/**
 * @brief facade class around vkb::sg::Texture, providing a vulkan.hpp-based interface
 *
 * See vkb::sg::Texture for documentation
 */
class HPPTexture : private vkb::sg::Texture
{
  public:
	using ComponentType = vkb::sg::Texture;

	HPPTexture(const std::string &name) :
	    vkb::sg::Texture(name)
	{}

	vkb::scene_graph::components::HPPImage *get_image()
	{
		return reinterpret_cast<vkb::scene_graph::components::HPPImage *>(vkb::sg::Texture::get_image());
	}

	void set_image(vkb::scene_graph::components::HPPImage &image)
	{
		vkb::sg::Texture::set_image(reinterpret_cast<vkb::sg::Image &>(image));
	}

	void set_sampler(vkb::scene_graph::components::HPPSampler &sampler)
	{
		vkb::sg::Texture::set_sampler(reinterpret_cast<vkb::sg::Sampler &>(sampler));
	}
};
}        // namespace components
}        // namespace scene_graph
}        // namespace vkb
