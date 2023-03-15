/* Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.
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

#include <common/error.h>

#include <vulkan/vulkan.hpp>

namespace vkb
{
namespace common
{
/**
 * @brief facade class around vkb::VulkanException, providing a vulkan.hpp-based interface
 *
 * See vkb::VulkanException for documentation
 */
class HPPVulkanException : private vkb::VulkanException
{
  public:
	HPPVulkanException(vk::Result result, std::string const &msg = "Vulkan error") :
	    vkb::VulkanException(static_cast<VkResult>(result), msg)
	{}
};
}        // namespace common
}        // namespace vkb
