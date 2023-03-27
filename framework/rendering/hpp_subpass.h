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

#include "rendering/subpass.h"
#include <hpp_buffer_pool.h>

namespace vkb
{
namespace rendering
{
struct alignas(16) HPPLight
{
	glm::vec4 position;         // position.w represents type of light
	glm::vec4 color;            // color.w represents light intensity
	glm::vec4 direction;        // direction.w represents range
	glm::vec2 info;             // (only used for spot lights) info.x represents light inner cone angle, info.y represents light outer cone angle
};

struct HPPLightingState
{
	std::vector<HPPLight>    directional_lights;
	std::vector<HPPLight>    point_lights;
	std::vector<HPPLight>    spot_lights;
	vkb::HPPBufferAllocation light_buffer;
};

/**
 * @brief facade class around vkb::Subpass, providing a vulkan.hpp-based interface
 *
 * See vkb::Subpass for documentation
 */
class HPPSubpass : private vkb::Subpass
{
  public:
	using vkb::Subpass::get_color_resolve_attachments;
	using vkb::Subpass::get_debug_name;
	using vkb::Subpass::get_depth_stencil_resolve_attachment;
	using vkb::Subpass::get_disable_depth_stencil_attachment;
	using vkb::Subpass::get_input_attachments;
	using vkb::Subpass::get_output_attachments;

  public:
	const vk::ResolveModeFlagBits get_depth_stencil_resolve_mode() const
	{
		return static_cast<vk::ResolveModeFlagBits>(vkb::Subpass::get_depth_stencil_resolve_mode());
	}
};

}        // namespace rendering
}        // namespace vkb
