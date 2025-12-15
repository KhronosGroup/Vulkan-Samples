/* Copyright (c) 2025 Holochip Corporation
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
#include "vulkan_device.h"
#include <algorithm>
#include <iostream>
#include <ranges>
#include <set>

// Constructor
VulkanDevice::VulkanDevice(vk::raii::Instance &instance, vk::raii::SurfaceKHR &surface,
                           const std::vector<const char *> &requiredExtensions,
                           const std::vector<const char *> &optionalExtensions) :
    instance(instance), surface(surface), requiredExtensions(requiredExtensions), optionalExtensions(optionalExtensions)
{
	// Initialize deviceExtensions with required extensions
	deviceExtensions = requiredExtensions;

	// Add optional extensions
	deviceExtensions.insert(deviceExtensions.end(), optionalExtensions.begin(), optionalExtensions.end());
}

// Destructor
VulkanDevice::~VulkanDevice()
{
	// RAII will handle destruction
}

// Pick physical device - improved implementation based on 37_multithreading.cpp
bool VulkanDevice::pickPhysicalDevice()
{
	try
	{
		// Get available physical devices
		std::vector<vk::raii::PhysicalDevice> devices = instance.enumeratePhysicalDevices();

		if (devices.empty())
		{
			std::cerr << "Failed to find GPUs with Vulkan support" << std::endl;
			return false;
		}

		// Find a suitable device using modern C++ ranges
		const auto devIter = std::ranges::find_if(
		    devices,
		    [&](auto &device) {
			    // Print device properties for debugging
			    vk::PhysicalDeviceProperties deviceProperties = device.getProperties();
			    std::cout << "Checking device: " << deviceProperties.deviceName << std::endl;

			    // Check if device supports Vulkan 1.3
			    bool supportsVulkan1_3 = deviceProperties.apiVersion >= vk::ApiVersion13;
			    if (!supportsVulkan1_3)
			    {
				    std::cout << "  - Does not support Vulkan 1.3" << std::endl;
			    }

			    // Check queue families
			    QueueFamilyIndices indices          = findQueueFamilies(device);
			    bool               supportsGraphics = indices.isComplete();
			    if (!supportsGraphics)
			    {
				    std::cout << "  - Missing required queue families" << std::endl;
			    }

			    // Check device extensions
			    bool supportsAllRequiredExtensions = checkDeviceExtensionSupport(device);
			    if (!supportsAllRequiredExtensions)
			    {
				    std::cout << "  - Missing required extensions" << std::endl;
			    }

			    // Check swap chain support
			    bool swapChainAdequate = false;
			    if (supportsAllRequiredExtensions)
			    {
				    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
				    swapChainAdequate                        = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
				    if (!swapChainAdequate)
				    {
					    std::cout << "  - Inadequate swap chain support" << std::endl;
				    }
			    }

			    // Check for required features
			    auto features                 = device.template getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT>();
			    bool supportsRequiredFeatures = features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering;
			    if (!supportsRequiredFeatures)
			    {
				    std::cout << "  - Does not support required features (dynamicRendering)" << std::endl;
			    }

			    supportsRequiredFeatures &= features.template get<vk::PhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT>().attachmentFeedbackLoopLayout;
			    if (!supportsRequiredFeatures)
			    {
				    std::cout << "  - Does not support required feature (attachmentFeedbackLoopLayout)" << std::endl;
			    }

			    return supportsVulkan1_3 && supportsGraphics && supportsAllRequiredExtensions && swapChainAdequate && supportsRequiredFeatures;
		    });

		if (devIter != devices.end())
		{
			physicalDevice                                = *devIter;
			vk::PhysicalDeviceProperties deviceProperties = physicalDevice.getProperties();
			std::cout << "Selected device: " << deviceProperties.deviceName << std::endl;

			// Store queue family indices for the selected device
			queueFamilyIndices = findQueueFamilies(physicalDevice);
			return true;
		}
		else
		{
			std::cerr << "Failed to find a suitable GPU. Make sure your GPU supports Vulkan and has the required extensions." << std::endl;
			return false;
		}
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to pick physical device: " << e.what() << std::endl;
		return false;
	}
}

// Create logical device
bool VulkanDevice::createLogicalDevice(bool enableValidationLayers, const std::vector<const char *> &validationLayers)
{
	try
	{
		// Create queue create infos for each unique queue family
		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t>                     uniqueQueueFamilies = {
            queueFamilyIndices.graphicsFamily.value(),
            queueFamilyIndices.presentFamily.value(),
            queueFamilyIndices.computeFamily.value()};

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies)
		{
			vk::DeviceQueueCreateInfo queueCreateInfo{
			    .queueFamilyIndex = queueFamily,
			    .queueCount       = 1,
			    .pQueuePriorities = &queuePriority};
			queueCreateInfos.push_back(queueCreateInfo);
		}

		// Enable required features
		auto features                       = physicalDevice.getFeatures2();
		features.features.samplerAnisotropy = vk::True;
		features.features.depthClamp        = vk::True;

		// Enable Vulkan 1.3 features
		vk::PhysicalDeviceVulkan13Features vulkan13Features;
		vulkan13Features.dynamicRendering = vk::True;
		vulkan13Features.synchronization2 = vk::True;
		features.pNext                    = &vulkan13Features;

		vk::PhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT feedbackLoopFeatures{};
		feedbackLoopFeatures.attachmentFeedbackLoopLayout = vk::True;

		vulkan13Features.pNext = &feedbackLoopFeatures;

		// Create device. Device layers are deprecated and ignored, so we
		// configure only extensions and features; validation is enabled via
		// instance layers.
		vk::DeviceCreateInfo createInfo{
		    .pNext                   = &features,
		    .queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size()),
		    .pQueueCreateInfos       = queueCreateInfos.data(),
		    .enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size()),
		    .ppEnabledExtensionNames = deviceExtensions.data(),
		    .pEnabledFeatures        = nullptr        // Using pNext for features
		};

		// Create the logical device
		device = vk::raii::Device(physicalDevice, createInfo);

		// Get queue handles
		graphicsQueue = vk::raii::Queue(device, queueFamilyIndices.graphicsFamily.value(), 0);
		presentQueue  = vk::raii::Queue(device, queueFamilyIndices.presentFamily.value(), 0);
		computeQueue  = vk::raii::Queue(device, queueFamilyIndices.computeFamily.value(), 0);

		return true;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to create logical device: " << e.what() << std::endl;
		return false;
	}
}

// Find queue families
QueueFamilyIndices VulkanDevice::findQueueFamilies(vk::raii::PhysicalDevice &device)
{
	QueueFamilyIndices indices;

	// Get queue family properties
	std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();

	// Find queue families that support graphics, compute, and present
	for (uint32_t i = 0; i < queueFamilies.size(); i++)
	{
		// Check for graphics support
		if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics)
		{
			indices.graphicsFamily = i;
		}

		// Check for compute support
		if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eCompute)
		{
			indices.computeFamily = i;
		}

		// Check for present support
		if (device.getSurfaceSupportKHR(i, surface))
		{
			indices.presentFamily = i;
		}

		// If all queue families are found, break
		if (indices.isComplete())
		{
			break;
		}
	}

	return indices;
}

// Query swap chain support
SwapChainSupportDetails VulkanDevice::querySwapChainSupport(vk::raii::PhysicalDevice &device)
{
	SwapChainSupportDetails details;

	// Get surface capabilities
	details.capabilities = device.getSurfaceCapabilitiesKHR(surface);

	// Get surface formats
	details.formats = device.getSurfaceFormatsKHR(surface);

	// Get present modes
	details.presentModes = device.getSurfacePresentModesKHR(surface);

	return details;
}

// Check device extension support
bool VulkanDevice::checkDeviceExtensionSupport(vk::raii::PhysicalDevice &device)
{
	// Get available extensions
	std::vector<vk::ExtensionProperties> availableExtensions = device.enumerateDeviceExtensionProperties();

	// Only check for required extensions, not optional ones
	std::set<std::string> requiredExtensionsSet(requiredExtensions.begin(), requiredExtensions.end());

	for (const auto &extension : availableExtensions)
	{
		requiredExtensionsSet.erase(extension.extensionName);
	}

	// Print missing required extensions
	if (!requiredExtensionsSet.empty())
	{
		std::cout << "Missing required extensions:" << std::endl;
		for (const auto &extension : requiredExtensionsSet)
		{
			std::cout << "  " << extension << std::endl;
		}
		return false;
	}

	return true;
}

// Check if a device is suitable
bool VulkanDevice::isDeviceSuitable(vk::raii::PhysicalDevice &device)
{
	// Check queue families
	QueueFamilyIndices indices = findQueueFamilies(device);

	// Check device extensions
	bool extensionsSupported = checkDeviceExtensionSupport(device);

	// Check swap chain support
	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
		swapChainAdequate                        = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	// Check for required features
	auto features                 = device.template getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features>();
	bool supportsRequiredFeatures = features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering;

	return indices.isComplete() && extensionsSupported && swapChainAdequate && supportsRequiredFeatures;
}

// Find memory type
uint32_t VulkanDevice::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const
{
	// Get memory properties
	vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

	// Find suitable memory type
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	throw std::runtime_error("Failed to find suitable memory type");
}
