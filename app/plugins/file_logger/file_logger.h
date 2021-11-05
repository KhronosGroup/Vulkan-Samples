/* Copyright (c) 2020-2021, Arm Limited and Contributors
 * Copyright (c) 2021, Sascha Willems
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
using FileLoggerTags = vkb::PluginBase<vkb::tags::Passive>;

/**
 * @brief File Logger
 * 
 * Enables writing log messages to a file
 * 
 * Usage: vulkan_sample --log-file filename.txt
 * 
 */
class FileLogger : public FileLoggerTags
{
  public:
	FileLogger();

	virtual ~FileLogger() = default;

	virtual bool is_active(const vkb::CommandParser &parser) override;

	virtual void init(const vkb::CommandParser &parser) override;

	vkb::FlagCommand log_file_flag = {vkb::FlagType::OneValue, "log-file", "", "Write log messages to the given file name"};
};
}        // namespace plugins
