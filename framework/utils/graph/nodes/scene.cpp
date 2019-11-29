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

#include "utils/graph/nodes/scene.h"

#include <cstdint>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <type_traits>

#include <glm/gtx/string_cast.hpp>
#include <json.hpp>
#include <spdlog/fmt/fmt.h>

#include "utils/strings.h"

namespace vkb
{
namespace utils
{
template <typename T>
std::string SceneNode::get_id(SceneNodeType type, T value)
{
	auto it = SceneNodeTypeStrings.find(type);
	if (it != SceneNodeTypeStrings.end())
	{
		return fmt::format("{}-{}", it->second, value);
	}
	throw std::runtime_error{"Type not in SceneNodeTypeStrings map"};
}

template <typename T>
std::string label(SceneNodeType type, const T &node)
{
	return node.get_name().empty() ? SceneNode::get_type_str(type) : SceneNode::get_type_str(type) + ": " + node.get_name();
}

std::unordered_map<SceneNodeType, std::string> SceneNode::SceneNodeTypeStrings{
    {SceneNodeType::Text, "Text"},
    {SceneNodeType::Scene, "Scene"},
    {SceneNodeType::Node, "Node"},
    {SceneNodeType::Transform, "Transform"},
    {SceneNodeType::Mesh, "Mesh"},
    {SceneNodeType::SubMesh, "SubMesh"},
    {SceneNodeType::Texture, "Texture"},
    {SceneNodeType::Material, "Material"}};

std::string SceneNode::get_type_str(SceneNodeType type)
{
	auto it = SceneNodeTypeStrings.find(type);
	if (it != SceneNodeTypeStrings.end())
	{
		return it->second;
	}
	throw std::runtime_error{"Type not in SceneNodeTypeStrings map"};
}

SceneNode::SceneNode(size_t id, std::string text)
{
	attributes["id"]    = id;
	attributes["label"] = text;
}

SceneNode::SceneNode(size_t id, const sg::Scene &scene)
{
	attributes["id"]    = id;
	attributes["type"]  = SceneNode::get_type_str(SceneNodeType::Scene);
	attributes["label"] = label(SceneNodeType::Scene, scene);
	attributes["data"]  = nlohmann::json{};
	attributes["group"] = "Scene";
}

SceneNode::SceneNode(size_t id, const sg::Node &node)
{
	attributes["id"]    = id;
	attributes["type"]  = SceneNode::get_type_str(SceneNodeType::Node);
	attributes["label"] = label(SceneNodeType::Node, node);
	attributes["group"] = "Node";
}

SceneNode::SceneNode(size_t id, const sg::Component &component)
{
	attributes["group"] = "Component";
	attributes["label"] = "Component";
}

SceneNode::SceneNode(size_t id, const sg::Transform &transform)
{
	attributes["id"]    = id;
	attributes["type"]  = SceneNode::get_type_str(SceneNodeType::Transform);
	attributes["label"] = label(SceneNodeType::Transform, transform);

	attributes["data"] = nlohmann::json{
	    {"translation", {{"x", transform.get_translation().x}, {"y", transform.get_translation().y}, {"z", transform.get_translation().z}}},
	    {"rotation", {{"x", transform.get_rotation().x}, {"y", transform.get_rotation().y}, {"z", transform.get_rotation().z}, {"w", transform.get_rotation().w}}},
	    {"scale", {{"x", transform.get_scale().x}, {"y", transform.get_scale().y}, {"z", transform.get_scale().z}}},
	    {"matrix", glm::to_string(transform.get_matrix())}};

	attributes["group"] = "Component";
}

SceneNode::SceneNode(size_t id, const sg::Mesh &mesh)
{
	attributes["id"]    = id;
	attributes["type"]  = SceneNode::get_type_str(SceneNodeType::Mesh);
	attributes["label"] = label(SceneNodeType::Mesh, mesh);
	attributes["group"] = "Component";
}

SceneNode::SceneNode(size_t id, const sg::SubMesh &submesh)
{
	attributes["id"]    = id;
	attributes["type"]  = SceneNode::get_type_str(SceneNodeType::SubMesh);
	attributes["label"] = label(SceneNodeType::SubMesh, submesh);
	attributes["group"] = "Component";
}

SceneNode::SceneNode(size_t id, const sg::Texture &texture, std::string name)
{
	attributes["id"]    = id;
	attributes["type"]  = SceneNode::get_type_str(SceneNodeType::Texture);
	attributes["label"] = label(SceneNodeType::Texture, texture);
	attributes["group"] = "Component";
}

SceneNode::SceneNode(size_t id, const sg::Material &mat)
{
	attributes["id"]    = id;
	attributes["type"]  = SceneNode::get_type_str(SceneNodeType::Material);
	attributes["label"] = label(SceneNodeType::Material, mat);
	attributes["data"]  = nlohmann::json{
        {"AlphaMode", utils::to_string(mat.alpha_mode)},
        {"emissive", glm::to_string(mat.emissive)},
        {"double_sided", utils::to_string(mat.double_sided)},
        {"alpha_cutoff", mat.alpha_cutoff}};

	attributes["group"] = "Component";
}
}        // namespace utils
}        // namespace vkb