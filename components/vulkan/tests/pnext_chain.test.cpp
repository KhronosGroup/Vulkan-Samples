/* Copyright (c) 2022, Arm Limited and Contributors
 * Copyright (c) 2022, Tom Atkinson
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

#include <components/vulkan/common/pnext_chain.hpp>

using namespace components::vulkan;

TEST_CASE("pChainNext no usage", "[vulkan]")
{
	// if no structs are appended to the chain then the builder should return a nullptr

	pNextChain chain;

	REQUIRE(chain.build() == nullptr);
}

TEST_CASE("pChainNext single usage", "[vulkan]")
{
	// if a single struct is appended to the chain then the builder should return a pointer to that struct

	pNextChain chain;

	chain.append<VkDebugUtilsMessengerCreateInfoEXT>([](VkDebugUtilsMessengerCreateInfoEXT &info) {
		info.flags           = 0;
		info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
		info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		info.pfnUserCallback = nullptr;
	});

	void *head = chain.build();

	auto *utils = reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT *>(head);

	REQUIRE(utils->sType == VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT);
	REQUIRE(utils->pNext == nullptr);        // NO CHAIN

	bool messageSeverity = utils->messageSeverity == static_cast<VkDebugUtilsMessageSeverityFlagsEXT>(VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT);
	REQUIRE(messageSeverity);

	bool messageType = utils->messageType == static_cast<VkDebugUtilsMessageTypeFlagBitsEXT>(VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT);
	REQUIRE(messageType);
}

TEST_CASE("pChainNext multiple usage", "[vulkan]")
{
	// if multiple structs are appended to the chain then a builder should return a chain in the order that structs are appended

	pNextChain chain;

	chain.append<VkDebugUtilsMessengerCreateInfoEXT>([](VkDebugUtilsMessengerCreateInfoEXT &info) {
		info.flags           = 0;
		info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
		info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		info.pfnUserCallback = nullptr;
	});

	// doesn't matter if we use the same struct
	chain.append<VkDebugUtilsMessengerCreateInfoEXT>([](VkDebugUtilsMessengerCreateInfoEXT &info) {
		info.flags           = 0;
		info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
		info.pfnUserCallback = nullptr;
	});

	void *head = chain.build();

	{
		auto *utils = reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT *>(head);
		REQUIRE(utils->sType == VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT);
		bool messageSeverity = utils->messageSeverity == static_cast<VkDebugUtilsMessageSeverityFlagsEXT>(VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT);
		REQUIRE(messageSeverity);
		bool messageType = utils->messageType == static_cast<VkDebugUtilsMessageTypeFlagBitsEXT>(VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT);
		REQUIRE(messageType);

		REQUIRE(utils->pNext != nullptr);
		head = const_cast<void *>(utils->pNext);
	}

	{
		auto *utils = reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT *>(head);
		REQUIRE(utils->sType == VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT);
		bool messageSeverity = utils->messageSeverity == static_cast<VkDebugUtilsMessageSeverityFlagsEXT>(VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT);
		REQUIRE(messageSeverity);
		bool messageType = utils->messageType == static_cast<VkDebugUtilsMessageTypeFlagBitsEXT>(VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT);
		REQUIRE(messageType);

		REQUIRE(utils->pNext == nullptr);
	}
}