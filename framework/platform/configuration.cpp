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

#include "configuration.h"

namespace vkb
{
BoolSetting::BoolSetting(bool &handle, bool value) :
    handle{handle},
    value{value}
{
}

void BoolSetting::set()
{
	handle = value;
}

std::type_index BoolSetting::get_type()
{
	return typeid(BoolSetting);
}

IntSetting::IntSetting(int &handle, int value) :
    handle{handle},
    value{value}
{
}

void IntSetting::set()
{
	handle = value;
}

std::type_index IntSetting::get_type()
{
	return typeid(IntSetting);
}

EmptySetting::EmptySetting()
{
}

void EmptySetting::set()
{
}

std::type_index EmptySetting::get_type()
{
	return typeid(EmptySetting);
}

void Configuration::set()
{
	for (auto pair : current_configuration->second)
	{
		for (auto setting : pair.second)
		{
			setting->set();
		}
	}
}

bool Configuration::next()
{
	if (configs.size() == 0)
	{
		return false;
	}

	current_configuration++;

	if (current_configuration == configs.end())
	{
		return false;
	}

	return true;
}

void Configuration::reset()
{
	current_configuration = configs.begin();
}

void Configuration::insert_setting(uint32_t config_index, std::unique_ptr<Setting> setting)
{
	settings.push_back(std::move(setting));
	configs[config_index][settings.back()->get_type()].push_back(settings.back().get());
}

}        // namespace vkb
