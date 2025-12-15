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

#include <memory>
#include <vector>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_raii.hpp>

#include "platform.h"
#include "vulkan_device.h"

/**
 * @brief Class for managing the Vulkan swap chain.
 */
class SwapChain
{
  public:
	/**
	 * @brief Constructor.
	 * @param device The Vulkan device.
	 * @param platform The platform.
	 */
	SwapChain(VulkanDevice &device, Platform *platform);

	/**
	 * @brief Destructor.
	 */
	~SwapChain();

	/**
	 * @brief Create the swap chain.
	 * @return True if the swap chain was created successfully, false otherwise.
	 */
	bool create();

	/**
	 * @brief Create image views for the swap chain images.
	 * @return True if the image views were created successfully, false otherwise.
	 */
	bool createImageViews();

	/**
	 * @brief Clean up the swap chain.
	 */
	void cleanup();

	/**
	 * @brief Recreate the swap chain.
	 * @return True, if the swap chain was recreated successfully, false otherwise.
	 */
	bool recreate();

	/**
	 * @brief Get the swap chain.
	 * @return The swap chain.
	 */
	vk::raii::SwapchainKHR &getSwapChain()
	{
		return swapChain;
	}

	/**
	 * @brief Get the swap chain images.
	 * @return The swap chain images.
	 */
	const std::vector<vk::Image> &getSwapChainImages() const
	{
		return swapChainImages;
	}

	/**
	 * @brief Get the swap chain image format.
	 * @return The swap chain image format.
	 */
	vk::Format getSwapChainImageFormat() const
	{
		return swapChainImageFormat;
	}

	/**
	 * @brief Get the swap chain extent.
	 * @return The swap chain extent.
	 */
	vk::Extent2D getSwapChainExtent() const
	{
		return swapChainExtent;
	}

	/**
	 * @brief Get the swap chain image views.
	 * @return The swap chain image views.
	 */
	const std::vector<vk::raii::ImageView> &getSwapChainImageViews() const
	{
		return swapChainImageViews;
	}

  private:
	// Vulkan device
	VulkanDevice &device;

	// Platform
	Platform *platform;

	// Swap chain
	vk::raii::SwapchainKHR           swapChain = nullptr;
	std::vector<vk::Image>           swapChainImages;
	vk::Format                       swapChainImageFormat = vk::Format::eUndefined;
	vk::Extent2D                     swapChainExtent      = {0, 0};
	std::vector<vk::raii::ImageView> swapChainImageViews;

	// Helper functions
	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats);
	vk::PresentModeKHR   chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes);
	vk::Extent2D         chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities);
};
