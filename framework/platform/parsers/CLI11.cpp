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

#include "CLI11.h"

#include <utility>

#include "common/logging.h"
#include "common/strings.h"

namespace vkb
{
CLI11CommandContext::CLI11CommandContext(CLI::App *cli, CLI11CommandContextState state) :
    CommandParserContext(), cli11(cli), _state(std::move(state))
{}

bool CLI11CommandContext::has_group_name() const
{
	return !_state.group_name.empty();
}

const std::string &CLI11CommandContext::get_group_name() const
{
	return _state.group_name;
}

CLI11CommandContextState CLI11CommandContext::get_state() const
{
	return _state;
}

CLI11CommandParser::CLI11CommandParser(const std::string &name, const std::string &description, const std::vector<std::string> &args) :
    _cli(std::make_unique<CLI::App>(description, name)),
    _base_context(_cli.get())
{
	_cli->allow_extras();

	_args.resize(args.size());
	std::transform(args.begin(), args.end(), _args.begin(), [](const std::string &string) -> char * { return const_cast<char *>(string.c_str()); });
}

std::vector<std::string> CLI11CommandParser::help() const
{
	return split(_cli->help(), "\n");
}

bool CLI11CommandParser::parse(CommandParserContext *context, const std::vector<Command *> &commands)
{
	if (!CommandParser::parse(&_base_context, commands))
	{
		std::cerr << "CLI11 Parser Error: Commands are not in the expected format\n";
		return false;
	}

	try
	{
		_cli->parse(static_cast<int>(_args.size()), _args.data());
	}
	catch (CLI::ParseError &e)
	{
		if (e.get_name() == "RuntimeError")
		{
			LOGE("CLI11 Parse Error")
			return false;
		}

		if (e.get_name() == "CallForHelp")
		{
			return false;
		}

		return e.get_exit_code() == static_cast<int>(CLI::ExitCodes::Success);
	}

	return true;
}

// Helper to reduce duplication
#define CAST(type)                                                                                          \
	void CLI11CommandParser::parse(CommandParserContext *context, type *command)                            \
	{                                                                                                       \
		parse(context == nullptr ? &_base_context : dynamic_cast<CLI11CommandContext *>(context), command); \
	}
CAST(CommandGroup)
CAST(SubCommand)
CAST(PositionalCommand)
CAST(FlagCommand)
#undef CAST

void CLI11CommandParser::parse(CLI11CommandContext *context, CommandGroup *command)
{
	auto state       = context->get_state();
	state.group_name = command->get_name();
	CLI11CommandContext group_context(context->cli11, state);
	CommandParser::parse(&group_context, command->get_commands());
}

void CLI11CommandParser::parse(CLI11CommandContext *context, SubCommand *command)
{
	auto *              subcommand = context->cli11->add_subcommand(command->get_name(), command->get_help_line());
	CLI11CommandContext subcommand_context(subcommand, context->get_state());
	CommandParser::parse(&subcommand_context, command->get_commands());
}

void CLI11CommandParser::parse(CLI11CommandContext *context, PositionalCommand *command)
{
	auto option = context->cli11->add_option(command->get_name(), command->get_help_line());

	_options.emplace(command, option);

	if (context->has_group_name())
	{
		option->group(context->get_group_name());
	}
}

void CLI11CommandParser::parse(CLI11CommandContext *context, FlagCommand *command)
{
	CLI::Option *flag;

	switch (command->get_flag_type())
	{
		case FlagType::FlagOnly:
			flag = context->cli11->add_flag(command->get_name(), command->get_help_line());
			break;
		case FlagType::OneValue:
		case FlagType::ManyValues:
			flag = context->cli11->add_option(command->get_name(), command->get_help_line());
			break;
	}

	_options.emplace(command, flag);

	if (context->has_group_name())
	{
		flag->group(context->get_group_name());
	}
}

bool CLI11CommandParser::contains(Command *command) const
{
	auto it = _options.find(command);

	if (it == _options.end())
	{
		return false;
	}

	return it->second->count() > 0;
}

std::vector<std::string> CLI11CommandParser::get_command_value(Command *command) const
{
	auto it = _options.find(command);

	if (it == _options.end())
	{
		return {};
	}

	return it->second->results();
}
}        // namespace vkb