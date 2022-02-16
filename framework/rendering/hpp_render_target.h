/* Copyright (c) 2021-2022, NVIDIA CORPORATION. All rights reserved.
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

#include <rendering/render_target.h>

namespace vkb
{
namespace rendering
{
/**
 * @brief facade class around vkb::RenderTarget, providing a vulkan.hpp-based interface
 *
 * See vkb::RenderTarget for documentation
 */
class HPPRenderTarget : private vkb::RenderTarget
{
  public:
	const vk::Extent2D &get_extent() const
	{
		return reinterpret_cast<vk::Extent2D const &>(vkb::RenderTarget::get_extent());
	}

	std::vector<vkb::core::HPPImageView> const &get_views() const
	{
		return reinterpret_cast<std::vector<vkb::core::HPPImageView> const &>(vkb::RenderTarget::get_views());
	}
};
}        // namespace rendering
}        // namespace vkb
