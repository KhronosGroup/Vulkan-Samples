/* Copyright (c) 2018-2020, Arm Limited and Contributors
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
Window::Window(Platform &platform, uint32_t width, uint32_t height) :
    platform{platform},
    width{width},
    height{height}
{
}

void Window::process_events()
{
}

Platform &Window::get_platform()
{
	return platform;
}

void Window::resize(uint32_t _width, uint32_t _height)
{
	this->width  = _width;
	this->height = _height;
}

uint32_t Window::get_width() const
{
	return width;
}

uint32_t Window::get_height() const
{
	return height;
}

float Window::get_content_scale_factor() const
{
	return 1.0f;
}

}        // namespace vkb
