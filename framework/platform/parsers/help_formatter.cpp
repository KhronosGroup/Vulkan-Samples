/* Copyright (c) 2021, Arm Limited and Contributors
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

#include "help_formatter.h"

#include <cassert>

#include "CLI11.h"
#include "common/strings.h"

namespace vkb
{
const std::string SPACER = "  ";

std::string prepend(const std::string &target, const std::string prepended)
{
	auto lines = split(target, "\n");

	if (lines.size() == 1)
	{
		if (lines[0].size() > 0)
		{
			return prepended + lines[0];
		}
	}
	else if (lines.size() > 1)
	{
		return prepended + join(lines, "\n" + prepended);
	}

	return target;
}

std::string HelpFormatter::make_help(const CLI::App *app, std::string name, CLI::AppFormatMode mode) const
{
	// This immediately forwards to the make_expanded method. This is done this way so that subcommands can
	// have overridden formatters
	if (mode == CLI::AppFormatMode::Sub)
	{
		return make_expanded(app);
	}

	std::stringstream out;
	if ((app->get_name().empty()) && (app->get_parent() != nullptr))
	{
		if (app->get_group() != "Subcommands")
		{
			out << app->get_group() << ':';
		}
	}

	out << make_description(app);
	out << make_usage(app, name);
	out << prepend(make_positionals(app), SPACER);
	out << prepend(make_groups(app, mode), SPACER);
	out << "\n"
	    << prepend(make_subcommands(app, mode), SPACER);

	return out.str();
}

std::string HelpFormatter::make_expanded(const CLI::App *sub) const
{
#define SPACE()                                 \
	{                                           \
		size_t current_size = out.str().size(); \
		if (current_size > last_size)           \
		{                                       \
			last_size = current_size;           \
			out << "\n";                        \
		}                                       \
	}

	const auto *meta = get_meta(sub);

	std::stringstream out;

	if (meta != nullptr)
	{
		out << meta->name << "\n";
	}
	else
	{
		out << sub->get_display_name(true) << "\n";
	}

	if (meta != nullptr)
	{
		out << SPACER << meta->description << "\n";
	}
	else
	{
		out << prepend(make_description(sub), SPACER);
	}

	if (sub->get_name().empty() && !sub->get_aliases().empty())
	{
		CLI::detail::format_aliases(out, sub->get_aliases(), column_width_ + 2);
	}

	size_t last_size = out.str().size();

	out << prepend(make_positionals(sub), SPACER);

	SPACE();

	out << prepend(make_groups(sub, CLI::AppFormatMode::Sub), SPACER);

	SPACE();

	out << prepend(make_subcommands(sub, CLI::AppFormatMode::Sub), SPACER);

	SPACE();

	return out.str();

#undef SPACE
}

void HelpFormatter::register_meta(const CLI::App *command, const HelpFormatter::Meta &meta)
{
	_meta.insert({command, meta});
}

const HelpFormatter::Meta *HelpFormatter::get_meta(const CLI::App *command) const
{
	auto it = _meta.find(command);
	return it == _meta.end() ? nullptr : &it->second;
}
}        // namespace vkb