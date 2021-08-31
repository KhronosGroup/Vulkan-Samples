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

#pragma once

#include <unordered_map>

#include "common/warnings.h"

VKBP_DISABLE_WARNINGS()
#include <CLI/CLI.hpp>
VKBP_ENABLE_WARNINGS()

#include "platform/parser.h"

namespace vkb
{
struct CLI11CommandContextState

{
	std::string group_name;
};

class CLI11CommandContext : public CommandParserContext
{
  public:
	explicit CLI11CommandContext(CLI::App *cli, CLI11CommandContextState state = {});

	~CLI11CommandContext() override = default;

	bool                     has_group_name() const;
	const std::string &      get_group_name() const;
	CLI11CommandContextState get_state() const;

	CLI::App *cli11;

  private:
	CLI11CommandContextState _state;
};

class CLI11CommandParser : public CommandParser
{
  public:
	CLI11CommandParser(const std::string &name, const std::string &description, const std::vector<std::string> &args);
	~CLI11CommandParser() override = default;

	std::vector<std::string> help() const override;

	bool parse(CommandParserContext *context, const std::vector<Command *> &commands) override;

#define CAST(type) virtual void parse(CommandParserContext *context, type *command) override;
	CAST(CommandGroup);
	CAST(SubCommand);
	CAST(PositionalCommand);
	CAST(FlagCommand);
#undef CAST

	void parse(CLI11CommandContext *context, CommandGroup *command);
	void parse(CLI11CommandContext *context, SubCommand *command);
	void parse(CLI11CommandContext *context, PositionalCommand *command);
	void parse(CLI11CommandContext *context, FlagCommand *command);

	bool contains(Command *command) const override;

  private:
	std::vector<char *>       _args;
	std::unique_ptr<CLI::App> _cli;
	CLI11CommandContext       _base_context;

	std::unordered_map<Command *, CLI::Option *> _options;

	std::vector<std::string> get_command_value(Command *command) const override;
};
}        // namespace vkb