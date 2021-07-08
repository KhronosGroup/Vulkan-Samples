/* Copyright (c) 2020-2021, Arm Limited and Contributors
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
#include "plugin.h"
#include "platform/platform.h"

namespace vkb
{
std::vector<Plugin *> associate_plugins(const std::vector<Plugin *> &plugins)
{
	for (auto *plugin : plugins)
	{
		for (auto *comparison_plugin : plugins)
		{
			bool full_control = comparison_plugin->has_tags<tags::FullControl>();
			bool stopping     = comparison_plugin->has_tags<tags::Stopping>();
			bool controlling  = full_control || stopping;

			bool entrypoint = comparison_plugin->has_tags<tags::Entrypoint>();

			if (plugin->has_tag<tags::FullControl>() && (controlling || entrypoint))
			{
				plugin->excludes(comparison_plugin);
				continue;
			}

			if (plugin->has_tag<tags::Stopping>() && stopping)
			{
				plugin->excludes(comparison_plugin);
				continue;
			}

			if (plugin->has_tag<tags::Entrypoint>() && entrypoint)
			{
				plugin->excludes(comparison_plugin);
				continue;
			}

			plugin->includes(comparison_plugin);
		}
	}

	return plugins;
}

bool Plugin::activate_plugin(Platform *p, const CommandParser &parser, bool force_activation)
{
	platform = p;

	bool active = is_active(parser);

	// Plugin activated
	if (force_activation || active)
	{
		init(parser);
	}

	return active;
}

const std::string &Plugin::get_name() const
{
	return name;
}

const std::string &Plugin::get_description() const
{
	return description;
}

void Plugin::excludes(Plugin *plugin)
{
	exclusions.push_back(plugin);
}

const std::vector<Plugin *> &Plugin::get_exclusions() const
{
	return exclusions;
}

void Plugin::includes(Plugin *plugin)
{
	inclusions.push_back(plugin);
}

const std::vector<Plugin *> &Plugin::get_inclusions() const
{
	return inclusions;
}
}        // namespace vkb