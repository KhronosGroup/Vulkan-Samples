/* Copyright (c) 2022-2025, Arm Limited and Contributors
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

#include "data_path.h"

#include "filesystem/filesystem.hpp"

namespace plugins
{
DataPath::DataPath() :
    DataPathTags("Data Path Override", "Specify the folder containing the sample data folders.", {vkb::Hook::OnAppStart}, {},
                 {{"data-path", "Folder containing data files"}})
{}

bool DataPath::handle_option(std::deque<std::string> &arguments)
{
	assert(!arguments.empty() && (arguments[0].substr(0, 2) == "--"));
	std::string option = arguments[0].substr(2);
	if (option == "data-path")
	{
		if (arguments.size() < 2)
		{
			LOGE("Option \"data-path\" is missing the actual data path!");
			return false;
		}
		std::string data_path = arguments[1];

		auto fs = vkb::filesystem::get();
		fs->set_external_storage_directory(data_path + "/");

		arguments.pop_front();
		arguments.pop_front();
		return true;
	}
	return false;
}
}        // namespace plugins