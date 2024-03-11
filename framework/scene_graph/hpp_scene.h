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

#include "components/hpp_camera.h"
#include "components/hpp_image.h"
#include "components/hpp_light.h"
#include "components/hpp_mesh.h"
#include "components/hpp_pbr_material.h"
#include "components/hpp_sampler.h"
#include "components/hpp_sub_mesh.h"
#include "components/hpp_texture.h"
#include "components/image.h"
#include "scene.h"
#include "scripts/hpp_animation.h"

namespace vkb
{
namespace scene_graph
{
class HPPNode;

/**
 * @brief facade class around vkb::sg::Scene, providing a vulkan.hpp-based interface
 *
 * See vkb::sg::Scene for documentation
 */
class HPPScene : private vkb::sg::Scene
{
  public:
	using vkb::sg::Scene::set_name;

	template <typename T>
	void add_component(std::unique_ptr<T> &&component)
	{
		vkb::sg::Scene::add_component(reinterpret_cast<std::unique_ptr<typename T::ComponentType> &&>(std::forward<std::unique_ptr<T>>(component)));
	}

	void add_node(std::unique_ptr<vkb::scene_graph::HPPNode> &&node)
	{
		vkb::sg::Scene::add_node(reinterpret_cast<std::unique_ptr<vkb::sg::Node> &&>(node));
	}

	template <typename T>
	std::vector<T *> get_components() const
	{
		auto components = vkb::sg::Scene::get_components<typename T::ComponentType>();
		return *reinterpret_cast<std::vector<T *> *>(&components);
	}

	std::vector<std::unique_ptr<vkb::scene_graph::HPPNode>> const &get_nodes() const
	{
		return reinterpret_cast<std::vector<std::unique_ptr<vkb::scene_graph::HPPNode>> const &>(vkb::sg::Scene::get_nodes());
	}

	vkb::scene_graph::HPPNode &get_root_node()
	{
		return reinterpret_cast<vkb::scene_graph::HPPNode &>(vkb::sg::Scene::get_root_node());
	}

	template <class T>
	bool has_component() const
	{
		return vkb::sg::Scene::has_component<typename T::ComponentType>();
	}

	template <typename T>
	void set_components(std::vector<std::unique_ptr<T>> &&components)
	{
		vkb::sg::Scene::set_components(
		    reinterpret_cast<std::vector<std::unique_ptr<typename T::ComponentType>> &&>(std::forward<std::vector<std::unique_ptr<T>>>(components)));
	}

	void set_nodes(std::vector<std::unique_ptr<vkb::scene_graph::HPPNode>> &&nodes)
	{
		vkb::sg::Scene::set_nodes(reinterpret_cast<std::vector<std::unique_ptr<vkb::sg::Node>> &&>(nodes));
	}

	void set_root_node(vkb::scene_graph::HPPNode &node)
	{
		vkb::sg::Scene::set_root_node(reinterpret_cast<vkb::sg::Node &>(node));
	}
};
}        // namespace scene_graph
}        // namespace vkb