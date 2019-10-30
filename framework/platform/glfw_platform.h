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

#include "common/vk_common.h"
#include "platform.h"

struct GLFWwindow;

namespace vkb
{
class GlfwPlatform : public Platform
{
  public:
	GlfwPlatform() = default;

	virtual ~GlfwPlatform() = default;

	virtual bool initialize(std::unique_ptr<Application> &&app) override;

	virtual VkSurfaceKHR create_surface(VkInstance instance) override;

	virtual void main_loop() override;

	virtual void terminate(ExitCode code) override;

	virtual void close() const override;

	float get_dpi_factor() const override;

  private:
	GLFWwindow *window = nullptr;

	virtual std::vector<spdlog::sink_ptr> get_platform_sinks() override;
};
}        // namespace vkb
