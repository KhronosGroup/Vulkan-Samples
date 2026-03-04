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

#include "start_test.h"

#include "apps.h"

namespace plugins
{
StartTest::StartTest() : StartTestTags("Tests", "A collection of flags to run tests.", {}, {{"test", "Run a specific test"}})
{}

bool StartTest::handle_command(std::deque<std::string> &arguments) const
{
	assert(!arguments.empty());
	if (arguments[0] == "test")
	{
		if (arguments.size() < 2)
		{
			LOGE("Command \"test\" is missing the actual test_id to launch!");
			return false;
		}
		auto *test = apps::get_app(arguments[1]);
		if (!test)
		{
			LOGE("Command \"test\" is called with an unknown test_id \"{}\"!", arguments[1]);
			return false;
		}
		platform->request_application(test);

		arguments.pop_front();
		arguments.pop_front();
		return true;
	}
	return false;
}
}        // namespace plugins