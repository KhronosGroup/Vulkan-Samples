/* Copyright (c) 2018-2023, Arm Limited and Contributors
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
enum UnixType
{
	Mac,
	Linux
};

class UnixPlatform : public Platform
{
  public:
	UnixPlatform(const PlatformContext &context, const UnixType &type);

	virtual ~UnixPlatform() = default;

  protected:
	virtual void create_window(const Window::Properties &properties) override;

  private:
	UnixType type;
};
}        // namespace vkb
