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

#include "common/logging.h"
#include "common/strings.h"

namespace vkb
{
CLI11CommandContext::CLI11CommandContext(CLI::App *cli, const CLI11CommandContextState &state) :
    CommandParserContext(), cli11(cli), _state(state)
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
    _cli11{std::make_unique<CLI::App>(description, name)}, _formatter{std::make_shared<HelpFormatter>()}
{
	_cli11->formatter(_formatter);

	_args.resize(args.size());
	std::transform(args.begin(), args.end(), _args.begin(), [](const std::string &string) -> char * { return const_cast<char *>(string.c_str()); });
}

std::vector<std::string> CLI11CommandParser::help() const
{
	return split(_cli11->help(), "\n");
}

// Helper to reduce duplication - throw should not occur as there should always be a valid context passed
#define CAST(type)                                                                                 \
	void CLI11CommandParser::parse(CommandParserContext *context, type *command)                   \
	{                                                                                              \
		parse(context == nullptr ? throw : dynamic_cast<CLI11CommandContext *>(context), command); \
	}
CAST(CommandGroup);
CAST(SubCommand);
CAST(PositionalCommand);
CAST(FlagCommand);
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
	auto *subcommand       = context->cli11->add_subcommand(command->get_name(), command->get_help_line());
	_sub_commands[command] = subcommand;
	subcommand->formatter(_formatter);
	CLI11CommandContext subcommand_context(subcommand, context->get_state());
	CommandParser::parse(&subcommand_context, command->get_commands());
}

void CLI11CommandParser::parse(CLI11CommandContext *context, PositionalCommand *command)
{
	auto *option = context->cli11->add_option(command->get_name(), command->get_help_line());

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
	{
		auto it = _options.find(command);

		if (it != _options.end())
		{
			return it->second->count() > 0;
		}
	}

	{
		auto it = _sub_commands.find(command);

		if (it != _sub_commands.end())
		{
			return it->second->count() > 0;
		}
	}

	return false;
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

// TODO: Explain this
bool CLI11CommandParser::parse(const std::vector<Plugin *> &plugins)
{
	// Generate all command groups
	for (auto plugin : plugins)
	{
		auto group = std::make_unique<CLI::App>();

		_formatter->register_meta(group.get(), {plugin->get_name(), plugin->get_description()});

		CLI11CommandContext context(group.get());
		CommandParser::parse(&context, plugin->get_cli_commands());

		_option_groups[plugin] = std::move(group);
	}

	// Associate correct command groups
	for (auto plugin : plugins)
	{
		auto plugin_cli       = _option_groups[plugin];
		auto included_plugins = plugin->get_inclusions();
		auto commands         = plugin->get_cli_commands();

		for (auto command : commands)
		{
			// Share flags and options with sub commands
			if (command->is<SubCommand>())
			{
				auto cli11_sub_command = _sub_commands[command];

				for (auto included_plugin : included_plugins)
				{
					cli11_sub_command->add_subcommand(_option_groups[included_plugin]);
				}
			}
		}

		_cli11->add_subcommand(plugin_cli);
	}

	return cli11_parse(_cli11.get());
}

bool CLI11CommandParser::parse(const std::vector<Command *> &commands)
{
	CLI11CommandContext context(_cli11.get());
	if (!CommandParser::parse(&context, commands))
	{
		return false;
	}

	return cli11_parse(_cli11.get());
}

bool CLI11CommandParser::cli11_parse(CLI::App *app)
{
	try
	{
		_args.insert(_args.begin(), "vulkan_samples");
		app->parse(static_cast<int>(_args.size()), _args.data());
	}
	catch (CLI::CallForHelp e)
	{
		return false;
	}
	catch (CLI::ParseError e)
	{
		bool success = e.get_exit_code() == static_cast<int>(CLI::ExitCodes::Success);

		if (!success)
		{
			LOGE("CLI11 Parse Error: [{}] {}", e.get_name(), e.what());
			return false;
		}
	}

	return true;
}
}        // namespace vkb