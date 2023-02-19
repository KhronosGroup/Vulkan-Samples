/* Copyright (c) 2023, NVIDIA CORPORATION. All rights reserved.
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

#include "core/sampler.h"

namespace vkb
{
namespace core
{
/**
 * @brief facade class around vkb::core::Sampler, providing a vulkan.hpp-based interface
 *
 * See vkb::core::Sampler for documentation
 */
class HPPSampler : private vkb::core::Sampler
{
  public:
	using vkb::core::Sampler::set_debug_name;

  public:
	HPPSampler(vkb::core::HPPDevice const &device, const vk::SamplerCreateInfo &info) :
	    vkb::core::Sampler(reinterpret_cast<vkb::Device const &>(device), reinterpret_cast<VkSamplerCreateInfo const &>(info))
	{}

  public:
	vk::Sampler const &get_handle() const
	{
		return reinterpret_cast<vk::Sampler const &>(vkb::core::Sampler::get_handle());
	}
};
}        // namespace core
}        // namespace vkb
