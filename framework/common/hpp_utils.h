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

#include <common/utils.h>

#include <rendering/hpp_render_context.h>

/**
 * @brief facade helper functions around the functions in common/utils.h, providing a vulkan.hpp-based interface
 */
namespace vkb
{
namespace common
{
void screenshot(vkb::rendering::HPPRenderContext &render_context, const std::string &filename)
{
	vkb::screenshot(reinterpret_cast<vkb::RenderContext &>(render_context), filename);
}

namespace graphs
{
bool generate_all(vkb::rendering::HPPRenderContext &context, sg::Scene &scene)
{
	return vkb::graphs::generate_all(reinterpret_cast<vkb::RenderContext &>(context), scene);
}
}        // namespace graphs

}        // namespace common
}        // namespace vkb
