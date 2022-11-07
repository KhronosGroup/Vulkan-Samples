/* Copyright (c) 2019-2022, Arm Limited and Contributors
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

#include <termios.h>
#include <unistd.h>

#include "common/vk_common.h"
#include "platform/platform.h"
#include "platform/window.h"

namespace vkb
{
/**
 * @brief Direct2Display window
 */
class DirectWindow : public Window
{
  public:
	DirectWindow(Platform *platform, const Window::Properties &properties);

	virtual ~DirectWindow();

	virtual VkSurfaceKHR create_surface(Instance &instance) override;

	virtual VkSurfaceKHR create_surface(VkInstance instance, VkPhysicalDevice physical_device) override;

	virtual bool should_close() override;

	virtual void process_events() override;

	virtual void close() override;

	virtual bool get_display_present_info(VkDisplayPresentInfoKHR *info,
	                                      uint32_t src_width, uint32_t src_height) const override;

	float get_dpi_factor() const override;

  private:
	void poll_terminal();

  private:
	mutable bool   keep_running = true;
	Platform *     platform     = nullptr;
	float          dpi;
	int            tty_fd;
	struct termios termio;
	struct termios termio_prev;
	KeyCode        key_down = KeyCode::Unknown;
	Extent         full_extent{};
};
}        // namespace vkb
