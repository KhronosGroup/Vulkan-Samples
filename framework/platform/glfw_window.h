/* Copyright (c) 2019-2023, Arm Limited and Contributors
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
#include "platform/window.h"

struct GLFWwindow;

namespace vkb
{
class Platform;

/**
 * @brief An implementation of GLFW, inheriting the behaviour of the Window interface
 */
class GlfwWindow : public Window
{
  public:
	GlfwWindow(Platform *platform, const Window::Properties &properties);

	virtual ~GlfwWindow();

	VkSurfaceKHR create_surface(Instance &instance) override;

	VkSurfaceKHR create_surface(VkInstance instance, VkPhysicalDevice physical_device) override;

	bool should_close() override;

	void process_events() override;

	void close() override;

	float get_dpi_factor() const override;

	float get_content_scale_factor() const override;

	std::vector<const char *> get_required_surface_extensions() const override;

  private:
	GLFWwindow *handle = nullptr;
};
}        // namespace vkb
