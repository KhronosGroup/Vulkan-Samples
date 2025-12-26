/* Copyright (c) 2020-2025, Arm Limited and Contributors
 * Copyright (c) 2025, NVIDIA CORPORATION. All rights reserved.
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

#include "platform/plugins/plugin_base.h"

namespace plugins
{
using StartTestTags = vkb::PluginBase<vkb::tags::Entrypoint>;

/**
 * @brief Start Test
 *
 * Start a given test. Used by system_test.py
 *
 * Usage: vulkan_sample test bonza
 *
 */
class StartTest : public StartTestTags
{
  public:
	StartTest();

	virtual ~StartTest() = default;

	bool handle_command(std::deque<std::string> &arguments) const override;
};
}        // namespace plugins