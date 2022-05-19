/* Copyright (c) 2018-2022, Arm Limited and Contributors
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

#include <android_native_app_glue.h>

#include "common/vk_common.h"
#include "platform/window.h"

namespace vkb
{
class AndroidPlatform;

/**
 * @brief Wrapper for a ANativeWindow, handles the window behaviour (including headless mode on Android)
 *        This class should not be responsible for destroying the underlying data it points to
 */
class AndroidWindow : public Window
{
  public:
	/**
	 * @brief Constructor
	 * @param platform The platform this window is created for
	 * @param window A reference to the location of the Android native window
	 * @param properties Window configuration
	 */
	AndroidWindow(AndroidPlatform *platform, ANativeWindow *&window, const Window::Properties &properties);

	virtual ~AndroidWindow() = default;

	/**
	 * @brief Creates a Vulkan surface to the native window
	 *        If headless, this will return VK_NULL_HANDLE
	 */
	virtual VkSurfaceKHR create_surface(Instance &instance) override;

	/**
	 * @brief Creates a Vulkan surface to the native window
	 *        If headless, this will return nullptr
	 */
	virtual VkSurfaceKHR create_surface(VkInstance instance, VkPhysicalDevice physical_device) override;

	virtual void process_events() override;

	virtual bool should_close() override;

	virtual void close() override;

	virtual float get_dpi_factor() const override;

  private:
	AndroidPlatform *platform;

	// Handle to the android window
	ANativeWindow *&handle;

	bool finish_called{false};
};
}        // namespace vkb
