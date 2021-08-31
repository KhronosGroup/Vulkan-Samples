/* Copyright (c) 2019, Arm Limited and Contributors
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

#include "debug_info.h"

namespace vkb
{
std::vector<std::unique_ptr<field::Base>> &DebugInfo::get_fields()
{
	return fields;
}

float DebugInfo::get_longest_label() const
{
	float column_width = 0.0f;
	for (auto &field : fields)
	{
		const std::string &label = field->label;

		if (static_cast<float>(label.size()) > column_width)
		{
			column_width = static_cast<float>(label.size());
		}
	}
	return column_width;
}
}        // namespace vkb
