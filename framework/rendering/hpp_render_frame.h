/* Copyright (c) 2021, NVIDIA CORPORATION. All rights reserved.
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

#include <rendering/render_frame.h>

#include <rendering/hpp_render_target.h>

namespace vkb
{
namespace rendering
{
/**
 * @brief facade class around vkb::RenderFrame, providing a vulkan.hpp-based interface
 *
 * See vkb::RenderFrame for documentation
 */
class HPPRenderFrame : protected vkb::RenderFrame
{
  public:
	vkb::rendering::HPPRenderTarget &get_render_target()
	{
		return reinterpret_cast<vkb::rendering::HPPRenderTarget &>(vkb::RenderFrame::get_render_target());
	}
};
}        // namespace rendering
}        // namespace vkb
