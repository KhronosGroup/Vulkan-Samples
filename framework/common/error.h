/* Copyright (c) 2018-2020, Arm Limited and Contributors
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

#include <cassert>
#include <stdexcept>
#include <string>

#include "common/strings.h"
#include "logging.h"
#include "vk_common.h"

namespace vkb
{
/**
 * @brief Vulkan exception structure
 */
class VulkanException : public std::runtime_error
{
  public:
	/**
	 * @brief Vulkan exception constructor
	 */
	explicit VulkanException(VkResult result, const std::string &msg = "Vulkan error");

	/**
	 * @brief Returns the Vulkan error code as string
	 * @return String message of exception
	 */
	const char *what() const noexcept override;

	VkResult result;

  private:
	std::string error_message;
};
}        // namespace vkb

/// @brief Helper macro to test the result of Vulkan calls which can return an error.
#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		VkResult err = x;                                           \
		if (err)                                                    \
		{                                                           \
			LOGE("Detected Vulkan error: {}", vkb::to_string(err)); \
			abort();                                                \
		}                                                           \
	} while (0)

#define ASSERT_VK_HANDLE(handle)        \
	do                                  \
	{                                   \
		if ((handle) == VK_NULL_HANDLE) \
		{                               \
			LOGE("Handle is NULL");     \
			abort();                    \
		}                               \
	} while (0)

#if !defined(NDEBUG) || defined(DEBUG) || defined(_DEBUG)
#	define VKB_DEBUG
#endif
