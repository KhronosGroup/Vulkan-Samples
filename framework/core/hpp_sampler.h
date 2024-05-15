/* Copyright (c) 2023-2024, NVIDIA CORPORATION. All rights reserved.
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

#include "core/vulkan_resource.h"

namespace vkb
{
namespace core
{
/**
 * @brief Represents a Vulkan Sampler, using Vulkan-Hpp
 */
class HPPSampler : public vkb::core::VulkanResource<vkb::BindingType::Cpp, vk::Sampler>
{
  public:
	/**
	 * @brief Creates a Vulkan HPPSampler
	 * @param device The device to use
	 * @param info Creation details
	 */
	HPPSampler(vkb::core::HPPDevice &device, const vk::SamplerCreateInfo &info);

	HPPSampler(const HPPSampler &) = delete;

	HPPSampler(HPPSampler &&sampler);

	~HPPSampler();

	HPPSampler &operator=(const HPPSampler &) = delete;

	HPPSampler &operator=(HPPSampler &&) = delete;
};
}        // namespace core
}        // namespace vkb
