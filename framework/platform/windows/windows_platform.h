/* Copyright (c) 2019-2020, Arm Limited and Contributors
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

namespace vkb
{
class WindowsPlatform : public Platform
{
  public:
	WindowsPlatform(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	                PSTR lpCmdLine, INT nCmdShow);

	virtual ~WindowsPlatform() = default;

	virtual bool initialize(const std::vector<Extension *> &extensions) override;

	virtual void create_window() override;

	virtual void terminate(ExitCode code) override;

	virtual const char *get_surface_extension() override;

  private:
	virtual std::vector<spdlog::sink_ptr> get_platform_sinks() override;
};
}        // namespace vkb
