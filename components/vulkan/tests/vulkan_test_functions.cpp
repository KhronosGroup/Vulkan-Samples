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

#include "vulkan_test_functions.hpp"

#include <vector>

#include <catch2/catch_test_macros.hpp>

namespace components
{
namespace vulkan
{
namespace test
{
PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties = wrapper<PFN_vkEnumerateInstanceLayerProperties>([](uint32_t *pPropertyCount, VkLayerProperties *pProperties) -> VkResult {
	static std::vector<VkLayerProperties> default_layers = {
	    VkLayerProperties{
	        "VK_some_vulkan_layer",
	        VK_MAKE_API_VERSION(0, 1, 0, 0),
	        VK_MAKE_API_VERSION(0, 1, 0, 0),
	        "Just your average vulkan layer",
	    },
	    VkLayerProperties{
	        "VK_another_layer",
	        VK_MAKE_API_VERSION(0, 1, 0, 0),
	        VK_MAKE_API_VERSION(0, 1, 0, 0),
	        "Just your average vulkan layer",
	    },
	};

	if (pPropertyCount)
	{
		*pPropertyCount = static_cast<uint32_t>(default_layers.size());
	}

	if (pProperties)
	{
		memcpy(pProperties, default_layers.data(), default_layers.size() * sizeof(default_layers[0]));
	}

	return VK_SUCCESS;
});

PFN_vkCreateInstance vkCreateInstance = wrapper<PFN_vkCreateInstance>([](const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkInstance *pInstance) -> VkResult {
	REQUIRE(pCreateInfo != nullptr);
	REQUIRE(pAllocator == nullptr);
	REQUIRE(pInstance != nullptr);
	REQUIRE(pCreateInfo->pApplicationInfo != nullptr);

	// spoof instance creation
	*pInstance = reinterpret_cast<VkInstance>(0x1);

	return VK_SUCCESS;
});

PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT = wrapper<PFN_vkDestroyDebugReportCallbackEXT>([](VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks *pAllocator) {
	REQUIRE(instance != VK_NULL_HANDLE);
	REQUIRE(callback != VK_NULL_HANDLE);
	REQUIRE(pAllocator == nullptr);
});

PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = wrapper<PFN_vkDestroyDebugUtilsMessengerEXT>([](VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks *pAllocator) {
	REQUIRE(instance != VK_NULL_HANDLE);
	REQUIRE(messenger != VK_NULL_HANDLE);
	REQUIRE(pAllocator == nullptr);
});

PFN_vkDestroyInstance vkDestroyInstance = wrapper<PFN_vkDestroyInstance>([](VkInstance instance, const VkAllocationCallbacks *pAllocator) {
	REQUIRE(instance != VK_NULL_HANDLE);
	REQUIRE(pAllocator == nullptr);
});

}        // namespace test
}        // namespace vulkan
}        // namespace components