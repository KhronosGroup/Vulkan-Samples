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

#include "hpp_sampler.h"

#include "hpp_device.h"

namespace vkb
{
namespace core
{
HPPSampler::HPPSampler(vkb::core::HPPDevice &device, const vk::SamplerCreateInfo &info) :
    vkb::core::VulkanResource<vkb::BindingType::Cpp, vk::Sampler>{device.get_handle().createSampler(info), &device}
{}

HPPSampler::HPPSampler(HPPSampler &&other) :
    VulkanResource(std::move(other))
{}

HPPSampler::~HPPSampler()
{
	if (get_handle())
	{
		get_device().get_handle().destroySampler(get_handle());
	}
}

}        // namespace core
}        // namespace vkb
