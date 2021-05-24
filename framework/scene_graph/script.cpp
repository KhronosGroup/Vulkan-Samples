/* Copyright (c) 2019-2021, Arm Limited and Contributors
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

#include "script.h"

namespace vkb
{
namespace sg
{
Script::Script(const std::string &name) :
    Component{name}
{}

std::type_index Script::get_type()
{
	return typeid(Script);
}

void Script::input_event(const InputEvent & /*input_event*/)
{
}

void Script::resize(uint32_t /*width*/, uint32_t /*height*/)
{
}

NodeScript::NodeScript(Node &node, const std::string &name) :
    Script{name},
    node{node}
{
}

Node &NodeScript::get_node()
{
	return node;
}

}        // namespace sg
}        // namespace vkb
