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

#include <memory>

#include <volk.h>

namespace components
{
namespace vulkan
{
struct Instance
{
	VkInstance instance_handle;
};

class InstanceBuilder
{
  public:
	InstanceBuilder()  = default;
	~InstanceBuilder() = default;

	inline InstanceBuilder &set_vulkan_api_version(uint32_t major, uint32_t minor, uint32_t patch = 0, uint32_t variant = 0)
	{
		return set_vulkan_api_version(VK_MAKE_API_VERSION(variant, major, minor, patch));
	}

	InstanceBuilder &set_vulkan_api_version(uint32_t encoded_version);
	InstanceBuilder &enable_layer(const char *layer_name);
	InstanceBuilder &enable_extension(const char *layer_name);

	template <typename Func>
	InstanceBuilder &apply(Func &&func)
	{
		func(*this);
	}

	StackError build(Instance *o_instance);

  private:
	VkApplicationInfo    m_application_info{VK_STRUCTURE_TYPE_APPLICATION_INFO};
	VkInstanceCreateInfo m_instance_create_info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
};
}        // namespace vulkan
}        // namespace components