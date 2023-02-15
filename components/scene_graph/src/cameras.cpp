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

#include <components/scene_graph/components/cameras.hpp>

#include <components/scene_graph/components/transform.hpp>
#include <components/scene_graph/graph.hpp>

namespace components
{
namespace sg
{
namespace systems
{
void generate_view_and_projection_matrix(Registry &registry)
{
	{
		// Set cameras to the default PerspectiveCamera if OrthographicCamera or PerspectiveCamera are not set
		auto view = registry->view<Camera>(entt::exclude<OrthographicCamera, PerspectiveCamera>);
		registry->insert(view.begin(), view.end(), PerspectiveCamera{});
	}

	{
		// Generate all orthographic projection matrices
		auto view = registry->view<Camera, OrthographicCamera>();
		for (auto entity : view)
		{
			auto       &camera = view.get<Camera>(entity);
			const auto &ortho  = view.get<OrthographicCamera>(entity);

			// Note: Using reversed depth-buffer for increased precision, so z-near and z-far are flipped
			camera.projection_matrix = glm::ortho(ortho.left, ortho.right, ortho.bottom, ortho.top, ortho.far_plane, ortho.near_plane);
		}
	}

	{
		// Generate all perspective projection matrices
		auto view = registry->view<Camera, PerspectiveCamera>();
		for (auto entity : view)
		{
			auto       &camera = view.get<Camera>(entity);
			const auto &persp  = view.get<PerspectiveCamera>(entity);

			// Note: Using reversed depth-buffer for increased precision, so z-near and z-far are flipped
			camera.projection_matrix = glm::perspective(persp.fov, persp.aspect_ratio, persp.far_plane, persp.near_plane);
		}
	}
	{
		// generate view matrix
		auto view = registry->view<SceneNode, Camera>();
		for (auto entity : view)
		{
			auto       &camera = view.get<Camera>(entity);
			const auto &node   = view.get<SceneNode>(entity);

			if (auto locked_node = node.ptr.lock())
			{
				camera.view_matrix = glm::inverse(locked_node->world_matrix());
			}
		}
	}
}
}        // namespace systems
}        // namespace sg
}        // namespace components