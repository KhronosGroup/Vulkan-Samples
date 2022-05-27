/* Copyright (c) 2022, Arm Limited and Contributors
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

#include <glm/glm.hpp>

#include "components/scene_graph/graph.hpp"

namespace components
{
namespace sg
{
namespace tags
{
// Camera currently in use
struct MainCamera
{};
}        // namespace tags

struct Camera
{
	std::weak_ptr<Node> node_ptr;
};

struct ViewMatrix
{
	glm::mat4 view_matrix;
};

struct ProjectionMatrix
{
	glm::mat4 projection_matrix;
};

struct OrthographicCamera
{
	float left{-1.0f};
	float right{1.0f};
	float bottom{-1.0f};
	float top{1.0f};
	float near_plane{0.0f};
	float far_plane{1.0f};
};

struct PerspectiveCamera
{
	float aspect_ratio{1.0f};
	float fov{glm::radians(60.0f)};
	float far_plane{100.0};
	float near_plane{0.1f};
};

namespace systems
{
/**
 * @brief Calculate view matrices for every entity containing a Camera tag
 * 
 * @param registry a scene graph registry
 */
void generate_camera_view_matrix(Registry &registry);

/**
 * @brief Calculate projection matrix for every entity containing a Camera tag
 * 
 * @param registry a scene graph registry
 */
void generate_projection_matrix(Registry &registry);
}        // namespace systems

}        // namespace sg
}        // namespace components