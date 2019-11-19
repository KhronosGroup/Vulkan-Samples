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

#include <iostream>
#include <map>
#include <string>
#include <typeindex>
#include <unordered_map>

#include <json.hpp>
#include <spdlog/fmt/fmt.h>

#include "scene_graph/component.h"
#include "scene_graph/components/material.h"
#include "scene_graph/components/mesh.h"
#include "scene_graph/components/sub_mesh.h"
#include "scene_graph/components/texture.h"
#include "scene_graph/components/transform.h"
#include "scene_graph/node.h"
#include "scene_graph/scene.h"
#include "utils/graph/node.h"

using namespace nlohmann;

namespace vkb
{
namespace utils
{
enum class SceneNodeType
{
	Text,
	Scene,
	Node,
	Transform,
	Mesh,
	SubMesh,
	Texture,
	Material
};

/**
 * @brief SceneNode is a node type used by utils::Graph to create different node variants for different types of scene components.
 * This structure allows for minimum code cluttering when using the graph api.
 * Note: if you want to add a new scene node definition to the graph it must also be defined here
 */
class SceneNode : public Node
{
  public:
	enum class Group
	{
		Node,
		Scene,
		Component
	};

	SceneNode()
	{}

	SceneNode(size_t id, std::string text);

	SceneNode(size_t id, const sg::Scene &scene);
	SceneNode(size_t id, const sg::Node &node);
	SceneNode(size_t id, const sg::Component &component);
	SceneNode(size_t id, const sg::Transform &transform);
	SceneNode(size_t id, const sg::Mesh &mesh);
	SceneNode(size_t id, const sg::SubMesh &submesh);
	SceneNode(size_t id, const sg::Texture &texture, std::string name);
	SceneNode(size_t id, const sg::Material &mat);

	template <typename T>
	static std::string get_id(SceneNodeType type, T value);

	static std::string get_type_str(SceneNodeType type);

  private:
	static std::unordered_map<SceneNodeType, std::string> SceneNodeTypeStrings;
};
}        // namespace utils
}        // namespace vkb