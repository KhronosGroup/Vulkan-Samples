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

#include <catch2/catch_test_macros.hpp>

#include "components/vulkan/instance.hpp"

namespace components
{
namespace vulkan
{
void default_config(InstanceBuilder &builder)
{
	builder.set_vulkan_api_version(1, 2);
}

VkResult instance_minimum_config_override(const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkInstance *pInstance)
{
	REQUIRE(pCreateInfo != nullptr);
	REQUIRE(pAllocator == nullptr);
	REQUIRE(pInstance != nullptr);

	return VK_SUCCESS;
}

TEST_CASE("instance minimum config", "[vulkan]")
{
	// Override vkCreateInstance call
	vkCreateInstance = (PFN_vkCreateInstance) instance_minimum_config_override;

	InstanceBuilder builder;

	builder
	    .apply(default_config);        // apply default minimum config

	REQUIRE(builder.build() != nullptr);
}
}        // namespace vulkan
}        // namespace components
