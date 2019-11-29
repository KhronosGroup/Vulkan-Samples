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

#include "headless_window.h"

namespace vkb
{
HeadlessWindow::HeadlessWindow(Platform &platform, uint32_t width, uint32_t height) :
    Window(platform, width, height)
{
}

VkSurfaceKHR HeadlessWindow::create_surface(Instance &instance)
{
	return VK_NULL_HANDLE;
}

bool HeadlessWindow::should_close()
{
	return closed;
}

void HeadlessWindow::close()
{
	closed = true;
}

float HeadlessWindow::get_dpi_factor() const
{
	// This factor is used for scaling UI elements, so return 1.0f (1 x n = n)
	return 1.0f;
}
}        // namespace vkb
