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

#include "parser.h"

namespace vkb
{
Command::Command(const std::string &name, const std::string &help_line) :
    _name(name),
    _help_line(help_line){

    };

const std::string &Command::get_name() const
{
	assert(!_name.empty() && "Command name unset");
	return _name;
}

void Command::set_name(const std::string &name)
{
	_name = name;
}

const std::string &Command::get_help_line() const
{
	assert(!_help_line.empty() && "Command help line unset");
	return _help_line;
}

void Command::set_help_line(const std::string &help_line)
{
	_help_line = help_line;
}

MultipleCommands::MultipleCommands(const std::vector<Command *> &commands) :
    _commands(commands)
{}

const std::vector<Command *> &MultipleCommands::get_commands() const
{
	return _commands;
}

CommandGroup::CommandGroup(const std::string &name, const std::vector<Command *> &commands) :
    TypedCommand<CommandGroup>(name, ""), MultipleCommands(commands)
{
}

SubCommand::SubCommand(const std::string &name, const std::string &help_line, const std::vector<Command *> &commands) :
    TypedCommand<SubCommand>(name, help_line), MultipleCommands(commands)
{}

PositionalCommand::PositionalCommand(const std::string &name, const std::string &help_line) :
    TypedCommand<PositionalCommand>(name, help_line)
{}

FlagCommand::FlagCommand(FlagType type, const std::string &long_name, const std::string &short_name, const std::string &help_line) :
    TypedCommand<FlagCommand>("", help_line), _type{type}
{
	std::string name;

	if (!short_name.empty())
	{
		name += "-" + short_name;
	}

	if (!long_name.empty())
	{
		if (!name.empty())
		{
			name += ",";
		}

		name += "--" + long_name;
	}

	Command::set_name(name);
}

FlagType FlagCommand::get_flag_type() const
{
	return _type;
}

bool CommandParser::parse(const std::vector<Command *> &commands)
{
	return parse(nullptr, commands);
}

bool CommandParser::parse(CommandParserContext *context, const std::vector<Command *> &commands)
{
#define PARSE(type)                           \
	if (command->is<type>())                  \
	{                                         \
		parse(context, command->get<type>()); \
	}

	for (auto *command : commands)
	{
		PARSE(SubCommand);
		PARSE(PositionalCommand);
		PARSE(FlagCommand);
		PARSE(CommandGroup);
	}

#undef PARSE

	return true;
}

}        // namespace vkb