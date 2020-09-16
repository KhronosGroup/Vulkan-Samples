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

#include "start_test.h"

#include "apps.h"

namespace vkb
{
namespace extensions
{
Flag test_cmd = {"test", Flag::Type::CommandWithPositional, "Run a specific test"};

StartTest::StartTest() :
    StartTestTags({}, {FlagGroup(FlagGroup::Type::UseOne, false, {&test_cmd})})
{
}

bool StartTest::is_active(const Parser &parser)
{
	return parser.contains(test_cmd);
}

void StartTest::init(Platform &platform, const Parser &parser)
{
	if (parser.contains(test_cmd))
	{
		auto id   = parser.get_string(test_cmd);
		auto test = apps::get_app(id);
		if (test != nullptr)
		{
			platform.request_application(test);
		}
	}
}
}        // namespace extensions
}        // namespace vkb
