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
#include "plugin.h"
#include "platform/platform.h"

namespace vkb
{
const std::string &Plugin::get_name() const
{
	return name;
}

const std::string &Plugin::get_description() const
{
	return description;
}

void Plugin::log_help(size_t width) const
{
	LOGI("");
	LOGI("\t{}", name);
	LOGI("\t\t{}", description);
	LOGI("");

	if (!commands.empty())
	{
		LOGI("\t\tSubcommands:");
		for (auto const &command : commands)
		{
			std::ostringstream oss;
			oss.setf(std::ios::left);
			oss.width(width + 2);
			oss << command.first;
			LOGI("\t\t\t{}{}", oss.str().c_str(), command.second);
		}
		LOGI("");
	}

	if (!options.empty())
	{
		LOGI("\t\tOptions:");
		for (auto const &option : options)
		{
			std::ostringstream oss;
			oss.setf(std::ios::left);
			oss.width(width);
			oss << option.first;
			LOGI("\t\t\t--{}{}", oss.str().c_str(), option.second);
		}
		LOGI("");
	}
}

}        // namespace vkb