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
#include "vulkan_test_functions.hpp"

namespace components
{
namespace vulkan
{
struct TestCase
{
	PFN_vkEnumerateInstanceLayerProperties EnumerateInstanceLayerProperties{test::vkEnumerateInstanceLayerProperties};
	PFN_vkCreateInstance                   CreateInstance{test::vkCreateInstance};
	PFN_vkDestroyDebugReportCallbackEXT    DestroyDebugReportCallbackEXT{test::vkDestroyDebugReportCallbackEXT};
	PFN_vkDestroyDebugUtilsMessengerEXT    DestroyDebugUtilsMessengerEXT{test::vkDestroyDebugUtilsMessengerEXT};
	PFN_vkDestroyInstance                  DestroyInstance{test::vkDestroyInstance};

	inline void setup() const
	{
		// Overrides global vkFunction ptrs - classes vulkan usage gets passed to these functions instead
		vkEnumerateInstanceLayerProperties = EnumerateInstanceLayerProperties;
		vkCreateInstance                   = CreateInstance;
		vkDestroyDebugReportCallbackEXT    = DestroyDebugReportCallbackEXT;
		vkDestroyDebugUtilsMessengerEXT    = DestroyDebugUtilsMessengerEXT;
		vkDestroyInstance                  = DestroyInstance;
	}

	bool expect_build_error{false};
};

void execute_test(const TestCase &test, InstanceBuilder &builder)
{
	test.setup();

	Instance instance;
	instance.instance_handle = VK_NULL_HANDLE;

	if (test.expect_build_error)
	{
		REQUIRE(builder.build(&instance) != nullptr);
	}
	else
	{
		REQUIRE(builder.build(&instance) == nullptr);
		REQUIRE(instance.instance_handle != VK_NULL_HANDLE);
	}
}

TEST_CASE("instance apply vulkan api version", "[vulkan]")
{
	TestCase test;

	test.CreateInstance = test::wrapper<PFN_vkCreateInstance>([](const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkInstance *pInstance) -> VkResult {
		test::vkCreateInstance(pCreateInfo, pAllocator, pInstance);

		// correct API version in use
		REQUIRE(pCreateInfo->pApplicationInfo->apiVersion == VK_MAKE_API_VERSION(0, 1, 2, 0));
		return VK_SUCCESS;
	});

	InstanceBuilder builder;

	builder
	    .set_vulkan_api_version(1, 2);

	execute_test(test, builder);
}

TEST_CASE("instance add optional layer", "[vulkan]")
{
	TestCase test;
	test.CreateInstance = test::wrapper<PFN_vkCreateInstance>([](const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkInstance *pInstance) -> VkResult {
		test::vkCreateInstance(pCreateInfo, pAllocator, pInstance);

		// only one requested layer exists in the test harness
		REQUIRE(pCreateInfo->enabledLayerCount == 1);
		REQUIRE(std::string{pCreateInfo->ppEnabledLayerNames[0]} == "VK_some_vulkan_layer");
		REQUIRE(pCreateInfo->pApplicationInfo->apiVersion == VK_MAKE_API_VERSION(0, 1, 2, 0));

		return VK_SUCCESS;
	});

	InstanceBuilder builder;

	builder
	    .set_vulkan_api_version(1, 2, 0, 0)
	    .enable_optional_layer("VK_some_vulkan_layer")
	    .enable_optional_layer("non_existent_layer");

	execute_test(test, builder);
}

TEST_CASE("instance add required layer", "[vulkan]")
{
	TestCase test;
	test.CreateInstance = test::wrapper<PFN_vkCreateInstance>([](const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkInstance *pInstance) -> VkResult {
		test::vkCreateInstance(pCreateInfo, pAllocator, pInstance);

		REQUIRE(pCreateInfo->enabledLayerCount == 1);
		REQUIRE(std::string{pCreateInfo->ppEnabledLayerNames[0]} == "VK_some_vulkan_layer");
		REQUIRE(pCreateInfo->pApplicationInfo->apiVersion == VK_MAKE_API_VERSION(0, 1, 2, 0));

		return VK_SUCCESS;
	});

	InstanceBuilder builder;

	builder
	    .set_vulkan_api_version(1, 2, 0, 0)
	    .enable_required_layer("VK_some_vulkan_layer");

	execute_test(test, builder);
}

TEST_CASE("instance add required layer that doesn't exist", "[vulkan]")
{
	TestCase test;
	test.expect_build_error = true;

	InstanceBuilder builder;

	builder
	    .set_vulkan_api_version(1, 2, 0, 0)
	    .enable_required_layer("non_existant_layer");

	execute_test(test, builder);
}
}        // namespace vulkan
}        // namespace components
