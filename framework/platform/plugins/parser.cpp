/* Copyright (c) 2019-2020, Arm Limited and Contributors
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

#include "parser.h"

#include <iostream>
#include <set>
#include <sstream>

#include <spdlog/fmt/fmt.h>

#include "common/logging.h"
#include "common/strings.h"
#include "platform/plugins/plugin.h"

namespace vkb
{
Parser::Parser(const std::vector<Plugin *> &plugins)
{
	std::vector<Plugin *> entrypoints     = plugins::with_tags<tags::Entrypoint>(plugins);
	std::vector<Plugin *> not_entrypoints = plugins::without_tags<tags::Entrypoint>(plugins);

	// Dont mix well togther
	std::vector<Plugin *> aggressive = plugins::with_tags<tags::FullControl, tags::Stopping>(not_entrypoints);

	// Work well with any plugin
	std::vector<Plugin *> passives = plugins::with_tags<tags::Passive>(not_entrypoints);

	// Entrypoint - {aggressive, passive}
	std::unordered_map<Plugin *, std::vector<Plugin *>> usage;
	usage.reserve(entrypoints.size());

	for (auto entrypoint : entrypoints)
	{
		std::vector<Plugin *> compatable;

		if (!entrypoint->has_tag<tags::FullControl>() || entrypoint->has_tag<tags::Stopping>())
		{
			// The entrypoint does not dictate the applications functionality so allow other plugins to take control
			compatable.reserve(compatable.size() + aggressive.size());
			for (auto ext : aggressive)
			{
				compatable.push_back(ext);
			}
		}

		compatable.reserve(compatable.size() + passives.size());
		for (auto ext : passives)
		{
			compatable.push_back(ext);
		}

		usage.insert({entrypoint, compatable});
	}

	const char *spacer   = "  ";
	const char *app_name = "VulkanSamples";

	// usage line + line per entrypoint
	help.reserve(help.capacity() + 1 + entrypoints.size());

	help.push_back("Usage:");

	for (auto it = usage.begin(); it != usage.end(); it++)
	{
		std::stringstream out;

		out << spacer << app_name << " ";

		{
			// Append all entrypoint flag groups
			auto   entry  = it->first;
			auto & groups = entry->get_flag_groups();
			size_t i      = 0;
			for (auto &group : groups)
			{
				out << group.get_command();

				if (i < groups.size() - 1)
				{
					out << " ";
				}

				i++;
			}
		}
		{
			size_t i = 0;
			for (auto ext : it->second)
			{
				auto & groups = ext->get_flag_groups();
				size_t i      = 0;
				for (auto &group : groups)
				{
					out << group.get_command();

					if (i < groups.size() - 1)
					{
						out << " ";
					}

					i++;
				}
			}
		}

		help.push_back(out.str());
	}

	help.push_back(fmt::format("{}{} {}", spacer, app_name, "(-h | --help)"));
	help.push_back("");
	help.push_back("Options:");

	extra_help.push_back("Extras:");

	std::set<Flag *> unique_flags;
	for (auto ext : plugins)
	{
		auto &groups = ext->get_flag_groups();
		for (auto &group : groups)
		{
			auto group_flags = group.get_flags();
			for (auto flag : group_flags)
			{
				unique_flags.insert(flag);
			}
		}
	}

	size_t column_width = 30;

	for (auto flag : unique_flags)
	{
		auto command = flag->get_command();
		auto line    = fmt::format("{}{}{}{}", spacer, command, std::string(column_width - command.size(), ' '), flag->get_help());

		bool option = flag->get_type() == Flag::Type::FlagOnly ||
		              flag->get_type() == Flag::Type::FlagWithOneArg ||
		              flag->get_type() == Flag::Type::FlagWithManyArg;

		if (option)
		{
			// These flags must be added at docopt parse time for the flag keys to work
			help.push_back(line);
		}
		else
		{
			// These flags can be appended in the printed help and are not added to docopt
			extra_help.push_back(line);
		}
	}
}

bool Parser::parse(const std::vector<std::string> &args)
{
	try
	{
		std::string help_str;

		for (auto line : help)
		{
			help_str += line + "\n";
		}

		parsed_args = docopt::docopt_parse(help_str, args, true, false, false);
	}
	catch (docopt::DocoptLanguageError e)
	{
		LOGE("LanguageError: {}", e.what());
		return false;
	}
	catch (docopt::DocoptArgumentError e)
	{
		LOGE("ArgumentError: {}", e.what());
		return false;
	}
	catch (...)
	{
		LOGE("Unknown Command");
		return false;
	}

	return true;
}

void Parser::print_help()
{
	for (auto &line : help)
	{
		LOGI("{}", line);
	}

	LOGI("");

	for (auto &line : extra_help)
	{
		LOGI("{}", line);
	}
}

bool Parser::contains(const Flag &flag) const
{
	return contains(flag.get_key());
}

bool Parser::contains(const std::string &key) const
{
	if (parsed_args.count(key) != 0)
	{
		const auto &result = parsed_args.at(key);

		if (result)
		{
			if (result.isBool())
			{
				return result.asBool();
			}

			return true;
		}
	}

	return false;
}

const docopt::value &Parser::get(const std::string &key) const
{
	return parsed_args.at(key);
}

const bool Parser::get_bool(const Flag &flag) const
{
	auto key = flag.get_key();

	if (contains(key))
	{
		auto result = get(key);
		if (result.isBool())
		{
			return result.asBool();
		}

		throw std::runtime_error("Argument option is not a bool type");
	}

	throw std::runtime_error("Couldn't find argument option");
}

const int32_t Parser::get_int(const Flag &flag) const
{
	auto key = flag.get_key();

	if (contains(key))
	{
		auto result = get(key);

		// Throws a runtime error if the result can not be parsed as long
		return static_cast<int32_t>(result.asLong());

		throw std::runtime_error("Argument option is not int type");
	}

	throw std::runtime_error("Couldn't find argument option");
}

const std::string Parser::get_string(const Flag &flag) const
{
	auto key = flag.get_key();

	if (contains(key))
	{
		auto result = get(key);
		if (result.isString())
		{
			return result.asString();
		}

		throw std::runtime_error("Argument option is not string type");
	}

	throw std::runtime_error("Couldn't find argument option");
}

const std::vector<std::string> Parser::get_list(const Flag &flag) const
{
	auto key = flag.get_key();

	if (contains(key))
	{
		auto result = get(key);
		if (result.isStringList())
		{
			return result.asStringList();
		}
		else if (result.isString())
		{
			// Only one item is in the list
			return {result.asString()};
		}

		throw std::runtime_error("Argument option is not vector of string type");
	}

	throw std::runtime_error("Couldn't find argument option");
}
}        // namespace vkb
