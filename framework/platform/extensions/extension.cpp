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
#include "extension.h"
#include "platform/extensions/parser.h"
#include "platform/platform.h"

namespace vkb
{
bool Extension::activate_extension(Platform &p, const Parser &parser)
{
	platform = &p;

	bool active = is_active(parser);

	// Extension activated
	if (active)
	{
		init(p, parser);
	}

	return active;
};
}        // namespace vkb