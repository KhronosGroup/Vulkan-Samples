/* Copyright (c) 2026 Batorip
 *
 * SPDX-FileCopyrightText: 2026 Batorip
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstdio>
#include <string_view>
#include <vector>
#include <vulkan/vulkan_core.h>

void probeDeviceGeneratedCommands(VkPhysicalDevice physicalDevice)
{
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensions.data());

	bool dgcSupported = false;
	bool isUnifiedExt = false;

	for (const auto &ext : extensions)
	{
		std::string_view name(ext.extensionName);
		if (name == "VK_EXT_device_generated_commands")
		{
			dgcSupported = true;
			isUnifiedExt = true;
		}
		else if (name == "VK_NV_device_generated_commands")
		{
			dgcSupported = true;
			isUnifiedExt = false;
		}
	}

	std::printf("[DGC Extension Status]: FOUND (%s)\n", isUnifiedExt ? "VK_EXT" : "VK_NV");

	if (dgcSupported)
	{
		if (isUnifiedExt)
		{
			VkPhysicalDeviceDeviceGeneratedCommandsPropertiesEXT dgcProps{
			    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_PROPERTIES_EXT};
			VkPhysicalDeviceProperties2 deviceProps2{
			    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
			    .pNext = &dgcProps};
			vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProps2);
			std::printf("--- Unified EXT DGC Limits ---\n");
			std::printf("Max Indirect Pipeline Count: %u\n", dgcProps.maxIndirectPipelineCount);
		}
		else
		{
			VkPhysicalDeviceDeviceGeneratedCommandsPropertiesNV nvDgcProps{
			    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_PROPERTIES_NV};
			VkPhysicalDeviceProperties2 deviceProps2{
			    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
			    .pNext = &nvDgcProps};
			vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProps2);
			std::printf("--- Legacy NVIDIA DGC Limits (Ampere/RTX 30-series) ---\n");
			std::printf("Max Graphics Shader Group Count:      %u\n", nvDgcProps.maxGraphicsShaderGroupCount);
			std::printf("Min Indirect Buffer Offset Alignment: %u\n", nvDgcProps.minIndirectCommandsBufferOffsetAlignment);
		}
	}
	else
	{
		std::printf("[DGC Extension Status]: NOT SUPPORTED on this physical device.\n");
	}
}
