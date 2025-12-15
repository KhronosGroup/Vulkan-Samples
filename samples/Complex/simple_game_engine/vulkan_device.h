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
#pragma once

#include <cstdint>
#include <optional>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

/**
 * @brief Structure for Vulkan queue family indices.
 */
struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;
	std::optional<uint32_t> computeFamily;

	bool isComplete() const
	{
		return graphicsFamily.has_value() && presentFamily.has_value() && computeFamily.has_value();
	}
};

/**
 * @brief Structure for swap chain support details.
 */
struct SwapChainSupportDetails
{
	vk::SurfaceCapabilitiesKHR        capabilities;
	std::vector<vk::SurfaceFormatKHR> formats;
	std::vector<vk::PresentModeKHR>   presentModes;
};

/**
 * @brief Class for managing Vulkan device selection and creation.
 */
class VulkanDevice
{
  public:
	/**
	 * @brief Constructor.
	 * @param instance The Vulkan instance.
	 * @param surface The Vulkan surface.
	 * @param requiredExtensions The required device extensions.
	 * @param optionalExtensions The optional device extensions.
	 */
	VulkanDevice(vk::raii::Instance &instance, vk::raii::SurfaceKHR &surface,
	             const std::vector<const char *> &requiredExtensions,
	             const std::vector<const char *> &optionalExtensions = {});

	/**
	 * @brief Destructor.
	 */
	~VulkanDevice();

	/**
	 * @brief Pick a suitable physical device.
	 * @return True if a suitable device was found, false otherwise.
	 */
	bool pickPhysicalDevice();

	/**
	 * @brief Create a logical device.
	 * @param enableValidationLayers Whether to enable validation layers.
	 * @param validationLayers The validation layers to enable.
	 * @return True if the logical device was created successfully, false otherwise.
	 */
	bool createLogicalDevice(bool enableValidationLayers, const std::vector<const char *> &validationLayers);

	/**
	 * @brief Get the physical device.
	 * @return The physical device.
	 */
	vk::raii::PhysicalDevice &getPhysicalDevice()
	{
		return physicalDevice;
	}

	/**
	 * @brief Get the logical device.
	 * @return The logical device.
	 */
	vk::raii::Device &getDevice()
	{
		return device;
	}

	/**
	 * @brief Get the graphics queue.
	 * @return The graphics queue.
	 */
	vk::raii::Queue &getGraphicsQueue()
	{
		return graphicsQueue;
	}

	/**
	 * @brief Get the present queue.
	 * @return The present queue.
	 */
	vk::raii::Queue &getPresentQueue()
	{
		return presentQueue;
	}

	/**
	 * @brief Get the compute queue.
	 * @return The compute queue.
	 */
	vk::raii::Queue &getComputeQueue()
	{
		return computeQueue;
	}

	/**
	 * @brief Get the queue family indices.
	 * @return The queue family indices.
	 */
	QueueFamilyIndices getQueueFamilyIndices() const
	{
		return queueFamilyIndices;
	}

	/**
	 * @brief Find queue families for a physical device.
	 * @param device The physical device.
	 * @return The queue family indices.
	 */
	QueueFamilyIndices findQueueFamilies(vk::raii::PhysicalDevice &device);

	/**
	 * @brief Query swap chain support for a physical device.
	 * @param device The physical device.
	 * @return The swap chain support details.
	 */
	SwapChainSupportDetails querySwapChainSupport(vk::raii::PhysicalDevice &device);

	/**
	 * @brief Find a memory type with the specified properties.
	 * @param typeFilter The type filter.
	 * @param properties The memory properties.
	 * @return The memory type index.
	 */
	uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;

  private:
	// Vulkan instance and surface
	vk::raii::Instance   &instance;
	vk::raii::SurfaceKHR &surface;

	// Vulkan device
	vk::raii::PhysicalDevice physicalDevice = nullptr;
	vk::raii::Device         device         = nullptr;

	// Vulkan queues
	vk::raii::Queue graphicsQueue = nullptr;
	vk::raii::Queue presentQueue  = nullptr;
	vk::raii::Queue computeQueue  = nullptr;

	// Queue family indices
	QueueFamilyIndices queueFamilyIndices;

	// Device extensions
	std::vector<const char *> requiredExtensions;
	std::vector<const char *> optionalExtensions;
	std::vector<const char *> deviceExtensions;

	// Private methods
	bool isDeviceSuitable(vk::raii::PhysicalDevice &device);
	bool checkDeviceExtensionSupport(vk::raii::PhysicalDevice &device);
};
