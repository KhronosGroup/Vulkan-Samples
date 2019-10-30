/* Copyright (c) 2019, Arm Limited and Contributors
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

#include "platform/platform.h"
#include <termios.h>
#include <unistd.h>

namespace vkb
{
// Direct-To-Display platform
class LinuxD2DPlatform : public Platform
{
  public:
	LinuxD2DPlatform(int argc, char **argv);

	virtual ~LinuxD2DPlatform() = default;

	virtual bool initialize(std::unique_ptr<Application> &&app) override;

	virtual VkSurfaceKHR create_surface(VkInstance instance) override;

	virtual void main_loop() override;

	virtual void terminate(ExitCode code) override;

	virtual void close() const override;

	float get_dpi_factor() const override;

  private:
	virtual std::vector<spdlog::sink_ptr> get_platform_sinks() override;

	void poll_terminal();

	VkPhysicalDevice get_physical_device(VkInstance instance);

	uint32_t find_compatible_plane(VkPhysicalDevice phys_dev, VkDisplayKHR display,
	                               const std::vector<VkDisplayPlanePropertiesKHR> &plane_properties);

  private:
	mutable bool   keep_running = true;
	float          dpi;
	int            tty_fd;
	struct termios termio;
	struct termios termio_prev;
	KeyCode        key_down = KeyCode::Unknown;
};
}        // namespace vkb
