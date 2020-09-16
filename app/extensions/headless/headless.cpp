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

#include "headless.h"

namespace vkb
{
namespace extensions
{
Flag headless_flag = {"headless", Flag::Type::FlagOnly, "Run in headless mode"};

Headless::Headless() :
    HeadlessTags({}, {FlagGroup(FlagGroup::Type::Individual, true, {&headless_flag})})
{
}

bool Headless::is_active(const Parser &parser)
{
	return parser.contains(headless_flag);
}

void Headless::init(Platform &platform, const Parser &parser)
{
	// Use has_extension<extensions::Headless>() to query if headless mode is enabled
}
}        // namespace extensions
}        // namespace vkb
