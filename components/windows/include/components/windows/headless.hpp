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

#include <components/windows/window.hpp>

namespace components
{
namespace windows
{
class HeadlessWindow : public Window
{
  public:
	HeadlessWindow(const std::string &title = "New Window", const Extent &initial_extent = {600, 600});
	virtual ~HeadlessWindow() = default;
	virtual void             set_extent(const Extent &extent) override;
	virtual Extent           extent() const override;
	virtual void             set_position(const Position &position) override;
	virtual Position         position() const override;
	virtual float            dpi_factor() const override;
	virtual void             set_title(const std::string &title) override;
	virtual std::string_view title() const override;
	virtual void             update() override;
	virtual void             attach(events::EventBus &bus) override;

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
	virtual VkResult populate_surface_create_info(VkAndroidSurfaceCreateInfoKHR * /* info */) const override
	{
		return VK_INCOMPLETE;
	}
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
	virtual VkResult populate_surface_create_info(VkWin32SurfaceCreateInfoKHR * /* info */) const override
	{
		return VK_INCOMPLETE;
	}
#elif defined(VK_USE_PLATFORM_METAL_EXT)
	virtual VkResult populate_surface_create_info(VkMetalSurfaceCreateInfoEXT * /* info */) const override
	{
		return VK_INCOMPLETE;
	}
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
	virtual VkResult populate_surface_create_info(VkXlibSurfaceCreateInfoKHR * /* info */) const override
	{
		return VK_INCOMPLETE;
	}
#elif defined(VK_USE_PLATFORM_XCB_KHR)
	virtual VkResult populate_surface_create_info(VkXcbSurfaceCreateInfoKHR * /* info */) const override
	{
		return VK_INCOMPLETE;
	}
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
	virtual VkResult populate_surface_create_info(VkWaylandSurfaceCreateInfoKHR * /* info */) const override
	{
		return VK_INCOMPLETE;
	}
#elif defined(VK_USE_PLATFORM_DISPLAY_KHR)
	virtual VkResult populate_surface_create_info(VkDisplaySurfaceCreateInfoKHR * /* info */) const override
	{
		return VK_INCOMPLETE;
	}
#endif

  private:
	std::string m_title;

	bool   m_extent_changed{false};
	Extent m_extent{};

	bool     m_position_changed{false};
	Position m_position{0, 0};

	float m_dpi_factor{1.0f};

	events::ChannelSenderPtr<PositionChangedEvent>    m_position_sender{nullptr};
	events::ChannelSenderPtr<ContentRectChangedEvent> m_content_rect_sender{nullptr};
};
}        // namespace windows
}        // namespace components