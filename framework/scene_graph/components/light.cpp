/* Copyright (c) 2018-2019, Arm Limited and Contributors
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

#include "light.h"

namespace vkb
{
namespace sg
{
Light::Light(const std::string &name) :
    Component{name}
{}

std::type_index Light::get_type()
{
	return typeid(Light);
}

void Light::set_node(Node &n)
{
	node = &n;
}

Node *Light::get_node()
{
	return node;
}

void Light::set_light_type(const LightType &type)
{
	this->light_type = type;
}

const LightType &Light::get_light_type()
{
	return light_type;
}

void Light::set_properties(const LightProperties &properties)
{
	this->properties = properties;
}

const LightProperties &Light::get_properties()
{
	return properties;
}

}        // namespace sg
}        // namespace vkb
