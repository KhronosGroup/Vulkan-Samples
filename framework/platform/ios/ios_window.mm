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

#include "ios_window.h"
#include <UIKit/UIScreen.h>
#include <UIKit/UIWindow.h>

#include "platform/ios/ios_platform.h"

namespace vkb
{
IosWindow::IosWindow(IosPlatform *platform, const Window::Properties &properties) :
    Window(properties),
    platform{platform}
{
}

VkSurfaceKHR IosWindow::create_surface(Instance &instance)
{
	return create_surface(instance.get_handle(), VK_NULL_HANDLE);
}

VkSurfaceKHR IosWindow::create_surface(VkInstance instance, VkPhysicalDevice)
{
	if (instance == VK_NULL_HANDLE || properties.mode == Mode::Headless)
	{
		return VK_NULL_HANDLE;
	}

	// Create a window that is the same size as the screen
	UIWindow *window = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];
	[window makeKeyAndVisible];

	VkSurfaceKHR surface{};

	VkMetalSurfaceCreateInfoEXT info{VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT};

	info.pLayer = (CAMetalLayer *) [window layer];

	VK_CHECK(vkCreateMetalSurfaceEXT(instance, &info, nullptr, &surface));

	return surface;
}

void IosWindow::process_events()
{
	//	process_android_events(platform->get_android_app());
}

bool IosWindow::should_close()
{
	return finish_called;
}

void IosWindow::close()
{
	//	GameActivity_finish(platform->get_activity());
	finish_called = true;
}

float IosWindow::get_dpi_factor() const
{
	//	return AConfiguration_getDensity(platform->get_android_app()->config) / static_cast<float>(ACONFIGURATION_DENSITY_MEDIUM);
}
}        // namespace vkb
