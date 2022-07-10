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

#pragma once

#include <stdint.h>

#include <string_view>

#include <components/events/event_bus.hpp>

#include <volk.h>

namespace components
{
namespace windows
{
struct Extent
{
	uint32_t width;
	uint32_t height;
};

struct Position
{
	uint32_t x;
	uint32_t y;
};

struct ShouldCloseEvent
{};

struct ContentRectChangedEvent
{
	Extent extent{0, 0};
};

struct PositionChangedEvent
{
	Position position{0, 0};
};

struct FocusChangedEvent
{
	bool is_focused{false};
};

class Window : public events::EventObserver
{
  public:
	Window()          = default;
	virtual ~Window() = default;

	virtual void   set_extent(const Extent &extent) = 0;
	virtual Extent extent() const                   = 0;

	virtual void     set_position(const Position &position) = 0;
	virtual Position position() const                       = 0;

	virtual float dpi_factor() const = 0;

	virtual void             set_title(const std::string &title) = 0;
	virtual std::string_view title() const                       = 0;

	// supported vulkan surface create infos
	// compile errors indicate that a platform supports a surface but we do not have an implementation for it

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
	virtual VkResult populate_surface_create_info(VkAndroidSurfaceCreateInfoKHR * /* info */) const = 0;
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
	virtual VkResult populate_surface_create_info(VkWin32SurfaceCreateInfoKHR * /* info */) const = 0;
#elif defined(VK_USE_PLATFORM_METAL_EXT)
	virtual VkResult populate_surface_create_info(VkMetalSurfaceCreateInfoEXT * /* info */) const = 0;
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
	virtual VkResult populate_surface_create_info(VkXlibSurfaceCreateInfoKHR * /* info */) const = 0;
#elif defined(VK_USE_PLATFORM_XCB_KHR)
	virtual VkResult populate_surface_create_info(VkXcbSurfaceCreateInfoKHR * /* info */) const = 0;
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
	virtual VkResult populate_surface_create_info(VkWaylandSurfaceCreateInfoKHR * /* info */) const = 0;
#elif defined(VK_USE_PLATFORM_DISPLAY_KHR)
	virtual VkResult populate_surface_create_info(VkDisplaySurfaceCreateInfoKHR * /* info */) const = 0;
#endif
};
}        // namespace windows
}        // namespace components