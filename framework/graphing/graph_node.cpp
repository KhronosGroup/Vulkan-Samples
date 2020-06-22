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
#include "graphing/graph_node.h"

namespace vkb
{
namespace graphing
{
Node::Node(size_t id, const char *title, const char *style, const nlohmann::json &data)
{
	attributes["id"]    = id;
	attributes["label"] = title;
	attributes["data"]  = data;
	attributes["style"] = style;
}
}        // namespace graphing
}        // namespace vkb
