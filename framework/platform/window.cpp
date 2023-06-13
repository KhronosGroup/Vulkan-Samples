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

#include "window.h"

#include "platform/platform.h"

namespace vkb
{
Window::Window(const Properties &properties) :
    properties{properties}
{
}

void Window::process_events()
{
}

Window::Extent Window::resize(const Extent &new_extent)
{
	if (properties.resizable)
	{
		properties.extent.width  = new_extent.width;
		properties.extent.height = new_extent.height;
	}

	return properties.extent;
}

const Window::Extent &Window::get_extent() const
{
	return properties.extent;
}

float Window::get_content_scale_factor() const
{
	return 1.0f;
}

Window::Mode Window::get_window_mode() const
{
	return properties.mode;
}

bool Window::get_display_present_info(VkDisplayPresentInfoKHR *info,
                                      uint32_t src_width, uint32_t src_height) const
{
	// Default is to not use the extra present info
	return false;
}
}        // namespace vkb
