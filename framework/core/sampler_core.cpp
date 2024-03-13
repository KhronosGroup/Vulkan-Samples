/* Copyright (c) 2019-2024, Arm Limited and Contributors
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

#include "sampler.h"

#include "device.h"

namespace vkb
{
namespace core
{
Sampler::Sampler(Device const &d, const VkSamplerCreateInfo &info) :
    VulkanResource{VK_NULL_HANDLE, &d}
{
	VK_CHECK(vkCreateSampler(get_device().get_handle(), &info, nullptr, &handle));
}

Sampler::Sampler(Sampler &&other) :
    VulkanResource{std::move(other)}
{
}

Sampler::~Sampler()
{
	if (handle != VK_NULL_HANDLE)
	{
		vkDestroySampler(get_device().get_handle(), handle, nullptr);
	}
}

}        // namespace core
}        // namespace vkb
