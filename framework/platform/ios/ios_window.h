/* Copyright (c) 2023, Holochip Inc.
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

#include "common/vk_common.h"
#include "platform/window.h"

namespace vkb
{
class IosPlatform;

/**
 * @brief Wrapper for a ANativeWindow, handles the window behaviour (including headless mode on Android)
 *        This class should not be responsible for destroying the underlying data it points to
 */
class IosWindow : public Window
{
  public:
	/**
	 * @brief Constructor
	 * @param platform The platform this window is created for
	 * @param properties Window configuration
	 */
	IosWindow(IosPlatform *platform, const Window::Properties &properties);

	~IosWindow() override = default;

	/**
	 * @brief Creates a Vulkan surface to the native window
	 *        If headless, this will return VK_NULL_HANDLE
	 */
	VkSurfaceKHR create_surface(Instance &instance) override;

	/**
	 * @brief Creates a Vulkan surface to the native window
	 *        If headless, this will return nullptr
	 */
	VkSurfaceKHR create_surface(VkInstance instance, VkPhysicalDevice physical_device) override;

	void process_events() override;

	bool should_close() override;

	void close() override;

	float get_dpi_factor() const override;

	std::vector<const char *> get_required_surface_extensions() const override;

  private:
	IosPlatform *platform;

	bool finish_called{false};
};
}        // namespace vkb
