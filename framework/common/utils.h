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

#include "common/error.h"

VKBP_DISABLE_WARNINGS()
#include <glm/glm.hpp>
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

}        // namespace vkb
