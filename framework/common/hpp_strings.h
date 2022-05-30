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

#include <common/strings.h>

#include <vulkan/vulkan.hpp>

namespace vkb
{
namespace common
{
/**
 * @brief facade helper functions around the functions in common/strings.h, providing a vulkan.hpp-based interface
 */
std::string to_string(vk::Extent2D const &extent)
{
	return vkb::to_string(static_cast<VkExtent2D const &>(extent));
}
}        // namespace common
}        // namespace vkb
