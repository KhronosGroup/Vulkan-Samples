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

#include "components/hpp_mesh.h"
#include "scene.h"
#include "scene_graph/script.h"
#include "scene_graph/scripts/animation.h"

namespace vkb
{
namespace scene_graph
{
/**
 * @brief facade class around vkb::sg::Scene, providing a vulkan.hpp-based interface
 *
 * See vkb::sb::Scene for documentation
 */
class HPPScene : private vkb::sg::Scene
{
  public:
	template <class T>
	std::vector<T *> get_components() const
	{
		if constexpr (std::is_same<T, vkb::sg::Script>::value)
		{
			return vkb::sg::Scene::get_components<T>();
		}
		else if constexpr (std::is_same<T, vkb::scene_graph::components::HPPMesh>::value)
		{
			std::vector<vkb::sg::Mesh *> meshes = vkb::sg::Scene::get_components<vkb::sg::Mesh>();
			return *reinterpret_cast<std::vector<T *> *>(&meshes);
		}
		else
		{
			assert(false);        // path never passed -> Please add a type-check here!
			return {};
		}
	}

	template <class T>
	bool has_component() const
	{
		if constexpr (std::is_same<T, vkb::sg::Animation>::value || std::is_same<T, vkb::sg::Script>::value)
		{
			return vkb::sg::Scene::has_component(typeid(T));
		}
		else
		{
			assert(false);        // path never passed -> Please add a type-check here!
			return false;
		}
	}
};
}        // namespace scene_graph
}        // namespace vkb
