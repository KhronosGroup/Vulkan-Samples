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

#include <components/scene_graph/graph.hpp>

namespace components
{
namespace sg
{
namespace systems
{
void generate_camera_view_matrix(Registry &registry)
{
	auto view = registry->view<Camera>();

	for (auto entity : view)
	{
		const auto &camera = view.get<Camera>(entity);

		if (auto node = camera.node_ptr.lock())
		{
			node->emplace_component<ViewMatrix>(glm::inverse(node->world_matrix()));
		}
	}
}

void generate_projection_matrix(Registry &registry)
{
	{
		// Set cameras to the default PerspectiveCamera if OrthographicCamera or PerspectiveCamera are not set
		auto view = registry->view<Camera>(entt::exclude<OrthographicCamera, PerspectiveCamera>);
		registry->insert(view.begin(), view.end(), PerspectiveCamera{});
	}

	{
		// Generate all orthographic projection matrices
		auto view = registry->view<OrthographicCamera>();
		for (auto entity : view)
		{
			const auto &ortho = view.get<OrthographicCamera>(entity);

			// Note: Using reversed depth-buffer for increased precision, so z-near and z-far are flipped
			registry->emplace<ProjectionMatrix>(entity, glm::ortho(ortho.left, ortho.right, ortho.bottom, ortho.top, ortho.far_plane, ortho.near_plane));
		}
	}

	{
		// Generate all perspective projection matrices
		auto view = registry->view<PerspectiveCamera>();
		for (auto entity : view)
		{
			const auto &persp = view.get<PerspectiveCamera>(entity);

			// Note: Using reversed depth-buffer for increased precision, so z-near and z-far are flipped
			registry->emplace<ProjectionMatrix>(entity, glm::perspective(persp.fov, persp.aspect_ratio, persp.far_plane, persp.near_plane));
		}
	}
}
}        // namespace systems
}        // namespace sg
}        // namespace components