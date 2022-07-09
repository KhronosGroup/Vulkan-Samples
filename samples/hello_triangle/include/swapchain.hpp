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

#include <components/windows/window.hpp>

/**
 * @brief Create a platform specific Vulkan Surface
 * 
 * @param context A Vulkan context with the render pass already set up.
 * @param window the window
 */
void init_surface(Context &context, components::windows::Window &window);

/**
 * @brief Initializes per frame data.
 * @param context A newly created Vulkan context.
 * @param per_frame The data of a frame.
 */
void init_per_frame(Context &context, PerFrame &per_frame);

/**
 * @brief Initializes the Vulkan framebuffers.
 * @param context A Vulkan context with the render pass already set up.
 */
void init_framebuffers(Context &context);

/**
 * @brief Initializes the Vulkan swapchain.
 * @param context A Vulkan context with a physical device already set up.
 */
void init_swapchain(Context &context);

/**
 * @brief Acquires an image from the swapchain.
 * @param context A Vulkan context with a swapchain already set up.
 * @param[out] image The swapchain index for the acquired image.
 * @returns Vulkan result code
 */
VkResult acquire_next_image(Context &context, uint32_t *image);

/**
 * @brief Presents an image to the swapchain.
 * @param context The Vulkan context, with a swapchain and per-frame resources already set up.
 * @param index The swapchain index previously obtained from @ref acquire_next_image.
 * @returns Vulkan result code
 */
VkResult present_image(Context &context, uint32_t index);

/**
 * @brief Tears down the frame data.
 * @param context The Vulkan context.
 * @param per_frame The data of a frame.
 */
void teardown_per_frame(Context &context, PerFrame &per_frame);

/**
 * @brief Tears down the framebuffers. If our swapchain changes, we will call this, and create a new swapchain.
 * @param context The Vulkan context.
 */
void teardown_framebuffers(Context &context);