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

#include "components/vulkan/instance.hpp"

namespace components
{
namespace vulkan
{
InstanceBuilder &InstanceBuilder::set_vulkan_api_version(uint32_t encoded_version)
{
	return *this;
}

InstanceBuilder &InstanceBuilder::enable_layer(const char *layer_name)
{
	return *this;
}

InstanceBuilder &InstanceBuilder::enable_extension(const char *layer_name)
{
	return *this;
}

StackError InstanceBuilder::build(Instance *o_instance)
{
	m_instance_create_info.pApplicationInfo = &m_application_info;

	Instance instance;

	auto result = vkCreateInstance(&m_instance_create_info, nullptr, &instance.instance_handle);
	if (result != VK_SUCCESS)
	{
		return StackError::create("failed to create instance", "vulkan/instance.cpp", __LINE__);
	}

	return nullptr;
}
}        // namespace vulkan
}        // namespace components