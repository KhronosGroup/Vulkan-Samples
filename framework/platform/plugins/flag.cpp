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

#include "flag.h"

#include <set>
#include <sstream>
#include <stdexcept>

namespace vkb
{
Flag::Flag(const std::string &name, Type type, const std::string &help) :
    name{name}, type{type}, help{help} {};

const std::string &Flag::get_name() const
{
	return name;
}

const std::string &Flag::get_help() const
{
	return help;
}

Flag::Type Flag::get_type() const
{
	return type;
}

std::string Flag::get_command() const
{
	switch (type)
	{
		case Flag::Type::Command:
			return name;
		case Flag::Type::CommandWithPositional:
			return name + " <" + name + "_arg>";
		case Flag::Type::Positional:
			return "<" + name + ">";
		case Flag::Type::FlagOnly:
			return "--" + name;
		case Flag::Type::FlagWithOneArg:
			return "--" + name + " <arg>";
		case Flag::Type::FlagWithManyArg:
			return "--" + name + "=<arg>";
		default:
			throw std::runtime_error{"Could not derive a key from a flag, is the type supported?"};
	}
}

std::string Flag::get_key() const
{
	switch (type)
	{
		case Flag::Type::Command:
			return name;
		case Flag::Type::CommandWithPositional:
			return "<" + name + "_arg>";
		case Flag::Type::Positional:
			return "<" + name + ">";
		case Flag::Type::FlagOnly:
		case Flag::Type::FlagWithOneArg:
		case Flag::Type::FlagWithManyArg:
			return "--" + name;
		default:
			throw std::runtime_error{"Could not derive a key from a flag, is the type supported?"};
	}
}

FlagGroup::FlagGroup(const Type &type, bool optional, const std::vector<Flag *> &flags) :
    type{type}, flags{flags}, optional{optional} {};

FlagGroup::FlagGroup(const std::vector<FlagGroup> &g)
{
	groups = g;
};

bool FlagGroup::is_optional() const
{
	return optional;
}

FlagGroup::Type FlagGroup::get_type() const
{
	return type;
}

std::string FlagGroup::get_command() const
{
	std::stringstream out;

	if (!flags.empty())
	{
		// Print the flags
		if (optional && (type == FlagGroup::Type::UseAll || type == FlagGroup::Type::UseOne))
		{
			out << "[";
		}
		else if (!optional && (type == FlagGroup::Type::UseAll || type == FlagGroup::Type::UseOne))
		{
			out << "(";
		}

		for (size_t i = 0; i < flags.size(); i++)
		{
			if (optional && type == FlagGroup::Type::Individual)
			{
				out << "[";
			}

			out << flags[i]->get_command();

			if (optional && type == FlagGroup::Type::Individual)
			{
				out << "]";
			}

			if (i < flags.size() - 1)
			{
				if (type == FlagGroup::Type::UseAll)
				{
					// Add spacer
					out << " ";
				}

				if (type == FlagGroup::Type::UseOne)
				{
					// Add OR
					out << " | ";
				}
			}
		}

		if (optional && (type == FlagGroup::Type::UseAll || type == FlagGroup::Type::UseOne))
		{
			out << "]";
		}
		else if (!optional && (type == FlagGroup::Type::UseAll || type == FlagGroup::Type::UseOne))
		{
			out << ")";
		}
	}
	else if (!groups.empty())
	{
		// Print the groups
		for (size_t i = 0; i < groups.size(); i++)
		{
			out << groups[i].get_command();

			if (i < groups.size() - 1)
			{
				out << " ";
			}
		}
	}

	return out.str();
}

std::vector<Flag *> FlagGroup::get_flags() const
{
	if (!flags.empty())
	{
		return flags;
	}
	else if (!groups.empty())
	{
		// Flags may be reused in different groups - get only unique flags
		std::set<Flag *> all_flags;

		for (auto &group : groups)
		{
			auto group_flags = group.get_flags();

			for (auto flag : group_flags)
			{
				all_flags.insert(flag);
			}
		}

		return {all_flags.begin(), all_flags.end()};
	}

	return {};
}
}        // namespace vkb