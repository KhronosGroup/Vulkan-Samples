/* Copyright (c) 2022, Arm Limited and Contributors
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
using DataPathTags = vkb::PluginBase<vkb::tags::Passive>;

/**
 * @brief Data path override
 *
 * Controls the root path used to find data files
 *
 * Usage: vulkan_sample sample afbc --data-path <folder>
 *
 */
class DataPath : public DataPathTags
{
  public:
	DataPath();

	virtual ~DataPath() = default;

	virtual bool is_active(const vkb::CommandParser &parser) override;

	virtual void init(const vkb::CommandParser &parser) override;

	vkb::FlagCommand data_path_flag = {vkb::FlagType::OneValue, "data-path", "", "Folder containing data files"};
};
}        // namespace plugins