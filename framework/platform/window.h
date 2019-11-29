/* Copyright (c) 2018-2019, Arm Limited and Contributors
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
#include "core/instance.h"

namespace vkb
{
class Platform;

/**
 * @brief An interface class, declaring the behaviour of a Window
 */
class Window
{
  public:
	/**
	 * @brief Constructs a Window
	 * @param platform The platform this window is created for
	 * @param width The width of the window
	 * @param height The height of the window
	 */
	Window(Platform &platform, uint32_t width, uint32_t height);

	virtual ~Window() = default;

	/**
	 * @brief Gets a handle from the platform's Vulkan surface 
	 * @param instance A Vulkan instance
	 * @returns A VkSurfaceKHR handle, for use by the application
	 */
	virtual VkSurfaceKHR create_surface(Instance &instance) = 0;

	/**
	 * @brief Checks if the window should be closed
	 */
	virtual bool should_close() = 0;

	/**
	 * @brief Handles the processing of all underlying window events
	 */
	virtual void process_events();

	/**
	 * @brief Requests to close the window
	 */
	virtual void close() = 0;

	/**
	 * @return The dot-per-inch scale factor
	 */
	virtual float get_dpi_factor() const = 0;

	Platform &get_platform();

	void resize(uint32_t width, uint32_t height);

	uint32_t get_width();

	uint32_t get_height();

  protected:
	Platform &platform;

  private:
	uint32_t width;

	uint32_t height;
};
}        // namespace vkb
