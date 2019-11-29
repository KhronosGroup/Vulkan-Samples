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

#include "android_window.h"

#include "platform/android/android_platform.h"

namespace vkb
{
AndroidWindow::AndroidWindow(Platform &platform, ANativeWindow *&window, bool headless) :
    Window(platform, 0, 0),
    handle{window},
    headless{headless}
{
}

VkSurfaceKHR AndroidWindow::create_surface(Instance &instance)
{
	if (instance.get_handle() == VK_NULL_HANDLE || !handle || headless)
	{
		return VK_NULL_HANDLE;
	}

	VkSurfaceKHR surface{};

	VkAndroidSurfaceCreateInfoKHR info{VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR};

	info.window = handle;

	VK_CHECK(vkCreateAndroidSurfaceKHR(instance.get_handle(), &info, nullptr, &surface));

	return surface;
}

bool AndroidWindow::should_close()
{
	return handle == nullptr;
}

void AndroidWindow::close()
{
	auto &android_platform = dynamic_cast<AndroidPlatform &>(platform);
	ANativeActivity_finish(android_platform.get_android_app()->activity);
}

float AndroidWindow::get_dpi_factor() const
{
	auto &android_platform = dynamic_cast<AndroidPlatform &>(platform);
	return AConfiguration_getDensity(android_platform.get_android_app()->config) / static_cast<float>(ACONFIGURATION_DENSITY_MEDIUM);
}
}        // namespace vkb
