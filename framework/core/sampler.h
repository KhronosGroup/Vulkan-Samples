/* Copyright (c) 2019, Arm Limited and Contributors
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

#include "common/helpers.h"
#include "common/vk_common.h"

namespace vkb
{
class Device;

namespace core
{
/**
 * @brief Represents a Vulkan Sampler
 */
class Sampler : public NonCopyable
{
  public:
	/**
	 * @brief Creates a Vulkan Sampler
	 * @param d The device to use
	 * @param info Creation details
	 */
	Sampler(Device &d, const VkSamplerCreateInfo &info);

	/**
	 * @brief Move constructs
	 */
	Sampler(Sampler &&sampler);

	~Sampler();

	/**
	 * @return The vulkan sampler handle
	 */
	VkSampler get_handle() const;

  private:
	Device &device;

	VkSampler handle{VK_NULL_HANDLE};
};
}        // namespace core
}        // namespace vkb
