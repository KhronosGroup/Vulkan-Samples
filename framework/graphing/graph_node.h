/* Copyright (c) 2018-2020, Arm Limited and Contributors
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

#include "common/warnings.h"

VKBP_DISABLE_WARNINGS()
#include <json.hpp>
VKBP_ENABLE_WARNINGS()

namespace vkb
{
namespace graphing
{
class Node
{
  public:
	Node()= default;

	Node(size_t id, const char *title, const char *style = nullptr, const nlohmann::json &data = {});

	template <typename T>
	static std::uintptr_t handle_to_uintptr_t(T handle)
	{
		return reinterpret_cast<std::uintptr_t>(reinterpret_cast<void *>(handle));
	}

	nlohmann::json attributes;
};
}        // namespace graphing
}        // namespace vkb