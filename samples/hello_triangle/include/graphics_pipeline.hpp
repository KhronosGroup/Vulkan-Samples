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

#include "context.hpp"

#include <string>

/**
 * @brief Initializes the Vulkan render pass.
 * @param context A Vulkan context with a device already set up.
 */
void init_render_pass(Context &context);

/**
 * @brief Initializes the Vulkan pipeline.
 * @param context A Vulkan context with a device and a render pass already set up.
 */
void init_pipeline(Context &context, const std::vector<uint8_t> &vertex_shader, const std::vector<uint8_t> &fragment_shader);

/**
 * @brief Helper function to load a shader module.
 * @param context A Vulkan context with a device.
 * @param path The path for the shader (relative to the assets directory).
 * @returns A VkShaderModule handle. Aborts execution if shader creation fails.
 */
VkShaderModule load_shader_module(Context &context, VkShaderStageFlagBits stage, const std::vector<uint8_t> &glsl_source);