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
#include "renderer.h"
#include <array>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>

// This file contains utility methods from the Renderer class

// Find memory type
uint32_t Renderer::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const
{
	try
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
	catch (const std::exception &e)
	{
		std::cerr << "Failed to find memory type: " << e.what() << std::endl;
		throw;
	}
}

// Find supported format
vk::Format Renderer::findSupportedFormat(const std::vector<vk::Format> &candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
{
	try
	{
		for (vk::Format format : candidates)
		{
			vk::FormatProperties props = physicalDevice.getFormatProperties(format);

			if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features)
			{
				return format;
			}
			else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features)
			{
				return format;
			}
		}

		throw std::runtime_error("Failed to find supported format");
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to find supported format: " << e.what() << std::endl;
		throw;
	}
}

// Find depth format
vk::Format Renderer::findDepthFormat()
{
	try
	{
		vk::Format depthFormat = findSupportedFormat(
		    {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
		    vk::ImageTiling::eOptimal,
		    vk::FormatFeatureFlagBits::eDepthStencilAttachment);
		std::cout << "Found depth format: " << static_cast<int>(depthFormat) << std::endl;
		return depthFormat;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to find supported depth format, falling back to D32_SFLOAT: " << e.what() << std::endl;
		// Fallback to D32_SFLOAT which is widely supported
		return vk::Format::eD32Sfloat;
	}
}

// Check if format has stencil component
bool Renderer::hasStencilComponent(vk::Format format)
{
	return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
}

// Read file
std::vector<char> Renderer::readFile(const std::string &filename)
{
	try
	{
		// Open file at end to get size
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open())
		{
			throw std::runtime_error("Failed to open file: " + filename);
		}

		// Get file size
		size_t            fileSize = file.tellg();
		std::vector<char> buffer(fileSize);

		// Go back to beginning of file and read data
		file.seekg(0);
		file.read(buffer.data(), fileSize);

		// Close file
		file.close();

		return buffer;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to read file: " << e.what() << std::endl;
		throw;
	}
}

// Create shader module
vk::raii::ShaderModule Renderer::createShaderModule(const std::vector<char> &code)
{
	try
	{
		// Create shader module
		vk::ShaderModuleCreateInfo createInfo{
		    .codeSize = code.size(),
		    .pCode    = reinterpret_cast<const uint32_t *>(code.data())};

		return vk::raii::ShaderModule(device, createInfo);
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to create shader module: " << e.what() << std::endl;
		throw;
	}
}

// Find queue families
QueueFamilyIndices Renderer::findQueueFamilies(const vk::raii::PhysicalDevice &device)
{
	QueueFamilyIndices indices;

	// Get queue family properties
	std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();

	// Find queue families that support graphics, compute, present, and (optionally) a dedicated transfer queue
	for (uint32_t i = 0; i < queueFamilies.size(); i++)
	{
		const auto &qf = queueFamilies[i];
		// Check for graphics support
		if ((qf.queueFlags & vk::QueueFlagBits::eGraphics) && !indices.graphicsFamily.has_value())
		{
			indices.graphicsFamily = i;
		}
		// Check for compute support
		if ((qf.queueFlags & vk::QueueFlagBits::eCompute) && !indices.computeFamily.has_value())
		{
			indices.computeFamily = i;
		}
		// Check for present support
		if (!indices.presentFamily.has_value() && device.getSurfaceSupportKHR(i, surface))
		{
			indices.presentFamily = i;
		}
		// Prefer a dedicated transfer queue (transfer bit set, but NOT graphics) if available
		if ((qf.queueFlags & vk::QueueFlagBits::eTransfer) && !(qf.queueFlags & vk::QueueFlagBits::eGraphics))
		{
			if (!indices.transferFamily.has_value())
			{
				indices.transferFamily = i;
			}
		}
		// If all required queue families are found, we can still continue to try find a dedicated transfer queue
		if (indices.isComplete() && indices.transferFamily.has_value())
		{
			// Found everything including dedicated transfer
			break;
		}
	}

	// Fallback: if no dedicated transfer queue, reuse graphics queue for transfer
	if (!indices.transferFamily.has_value() && indices.graphicsFamily.has_value())
	{
		indices.transferFamily = indices.graphicsFamily;
	}

	return indices;
}

// Query swap chain support
SwapChainSupportDetails Renderer::querySwapChainSupport(const vk::raii::PhysicalDevice &device)
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
bool Renderer::checkDeviceExtensionSupport(vk::raii::PhysicalDevice &device)
{
	auto availableDeviceExtensions = device.enumerateDeviceExtensionProperties();

	// Check if all required extensions are supported
	std::set<std::string> requiredExtensionsSet(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end());

	for (const auto &extension : availableDeviceExtensions)
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

// Check if device is suitable
bool Renderer::isDeviceSuitable(vk::raii::PhysicalDevice &device)
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

// Choose swap surface format
vk::SurfaceFormatKHR Renderer::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats)
{
	// Look for SRGB format
	for (const auto &availableFormat : availableFormats)
	{
		if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
		{
			return availableFormat;
		}
	}

	// If not found, return first available format
	return availableFormats[0];
}

// Choose swap present mode
vk::PresentModeKHR Renderer::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes)
{
	// Look for mailbox mode (triple buffering)
	for (const auto &availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == vk::PresentModeKHR::eMailbox)
		{
			return availablePresentMode;
		}
	}

	// If not found, return FIFO mode (guaranteed to be available)
	return vk::PresentModeKHR::eFifo;
}

// Choose swap extent
vk::Extent2D Renderer::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}
	else
	{
		// Get framebuffer size
		int width, height;
		platform->GetWindowSize(&width, &height);

		// Create extent
		vk::Extent2D actualExtent = {
		    static_cast<uint32_t>(width),
		    static_cast<uint32_t>(height)};

		// Clamp to min/max extent
		actualExtent.width  = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}
}

// Wait for device to be idle
void Renderer::WaitIdle()
{
	// External synchronization: ensure no queue submits/presents overlap a full device idle.
	// This is required for VVL cleanliness when other threads may hold or use queues.
	std::lock_guard<std::mutex> lock(queueMutex);
	device.waitIdle();
}
