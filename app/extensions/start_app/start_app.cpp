/* Copyright (c) 2020, Arm Limited and Contributors
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

#include "start_app.h"

#include "apps.h"

namespace vkb
{
namespace extensions
{
Flag app_cmd = {"app", Flag::Type::Positional, "Run a specific application"};

Flag sample_cmd = {"sample", Flag::Type::CommandWithPositional, "Run a specific sample"};

StartApp::StartApp() :
    StartAppTags({}, {FlagGroup(FlagGroup::Type::UseOne, false, {&app_cmd, &sample_cmd})})
{
}

bool StartApp::is_active(const Parser &parser)
{
	return parser.contains(app_cmd) || parser.contains(sample_cmd);
}

void StartApp::init(Platform &platform, const Parser &parser)
{
	std::string          id;
	const apps::AppInfo *app = nullptr;

	if (parser.contains(app_cmd))
	{
		id = parser.get_string(app_cmd);
	}

	if (parser.contains(sample_cmd))
	{
		id = parser.get_string(sample_cmd);
	}

	app = apps::get_app(id);

	if (app != nullptr)
	{
		platform.request_application(app);
	}
}
}        // namespace extensions
}        // namespace vkb
