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

#include "common/error.h"

VKBP_DISABLE_WARNINGS()
#include "common/glm_common.h"
#include "glm/gtx/quaternion.hpp"
VKBP_ENABLE_WARNINGS()

#include "platform/filesystem.h"
#include "rendering/pipeline_state.h"
#include "rendering/render_context.h"
#include "scene_graph/components/sub_mesh.h"
#include "scene_graph/scene.h"

namespace vkb
{
/**
 * @brief Extracts the extension from an uri
 * @param uri An uniform Resource Identifier
 * @return The extension
 */
std::string get_extension(const std::string &uri);

/**
 * @param name String to convert to snake case
 * @return a snake case version of the string
 */
std::string to_snake_case(const std::string &name);

/**
 * @brief Takes a screenshot of the app by writing the swapchain image to file (slow function)
 * @param filename The name of the file to save the output to
 */
void screenshot(RenderContext &render_context, const std::string &filename);

/**
 * @brief Adds a light to the scene with the specified parameters
 * @param scene The scene to add the light to
 * @param type The light type
 * @param position The position of the light
 * @param rotation The rotation of the light
 * @param props The light properties, such as color and intensity
 * @param parent_node The parent node for the line, defaults to root
 * @return The newly created light component
 */
sg::Light &add_light(sg::Scene &scene, sg::LightType type, const glm::vec3 &position, const glm::quat &rotation = {}, const sg::LightProperties &props = {}, sg::Node *parent_node = nullptr);

/**
 * @brief Adds a point light to the scene with the specified parameters
 * @param scene The scene to add the light to
 * @param position The position of the light
 * @param props The light properties, such as color and intensity
 * @param parent_node The parent node for the line, defaults to root
 * @return The newly created light component
 */
sg::Light &add_point_light(sg::Scene &scene, const glm::vec3 &position, const sg::LightProperties &props = {}, sg::Node *parent_node = nullptr);

/**
 * @brief Adds a directional light to the scene with the specified parameters
 * @param scene The scene to add the light to
 * @param rotation The rotation of the light
 * @param props The light properties, such as color and intensity
 * @param parent_node The parent node for the line, defaults to root
 * @return The newly created light component
 */
sg::Light &add_directional_light(sg::Scene &scene, const glm::quat &rotation, const sg::LightProperties &props = {}, sg::Node *parent_node = nullptr);

/**
 * @brief Adds a spot light to the scene with the specified parameters
 * @param scene The scene to add the light to
 * @param position The position of the light
 * @param rotation The rotation of the light
 * @param props The light properties, such as color and intensity
 * @param parent_node The parent node for the line, defaults to root
 * @return The newly created light component
 */
sg::Light &add_spot_light(sg::Scene &scene, const glm::vec3 &position, const glm::quat &rotation, const sg::LightProperties &props = {}, sg::Node *parent_node = nullptr);

/**
 * @brief Add free camera script to a node with a camera object.
 *        Fallback to the default_camera if node not found.
 * @param scene The scene to add the camera to
 * @param node_name The scene node name
 * @param extent The initial resolution of the camera
 * @return Node where the script was attached as component
 */
sg::Node &add_free_camera(sg::Scene &scene, const std::string &node_name, VkExtent2D extent);

namespace graphs
{
bool generate_all(RenderContext &context, sg::Scene &scene);
}

}        // namespace vkb
