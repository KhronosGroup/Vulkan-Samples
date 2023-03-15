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

#include "data_path.h"

namespace plugins
{
DataPath::DataPath() :
    DataPathTags("Data Path Override",
                 "Specify the folder containing the sample data folders.",
                 {vkb::Hook::OnAppStart}, {&data_path_flag})
{
}

bool DataPath::is_active(const vkb::CommandParser &parser)
{
	return parser.contains(&data_path_flag);
}

void DataPath::init(const vkb::CommandParser &parser)
{
	if (parser.contains(&data_path_flag))
	{
		vkb::Platform::set_external_storage_directory(parser.as<std::string>(&data_path_flag) + "/");
	}
}

}        // namespace plugins