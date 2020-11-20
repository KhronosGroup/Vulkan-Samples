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

#pragma once

#include <string>
#include <vector>

namespace vkb
{
/**
 * @brief The flag object used to interface with the argument parser
 */
class Flag
{
  public:
	enum class Type
	{
		Command,                      // A command word: command (Bool)
		CommandWithPositional,        // A command word with a positional argument: command <arg> (Bool)
		Positional,                   // A positional argument: <arg> (Any)
		FlagOnly,                     // A single flag: --flag (Bool)
		FlagWithOneArg,               // A flag with one positional arg: --flag <arg> (Any)
		FlagWithManyArg               // A flag with multiple args: --flag <arg>... (Any[])
	};

	Flag(const std::string &name, Type type, const std::string &help = "");

	const std::string &get_name() const;

	const std::string &get_help() const;

	Type get_type() const;

	std::string get_command() const;

	std::string get_key() const;

  private:
	std::string name;
	std::string help;
	Type        type;
};

/**
 * @brief Group flags and define how they should be accessed from the command line
 */
class FlagGroup
{
  public:
	enum class Type
	{
		Individual,        // Each flag is treated as independant from others in the flag group
		UseOne,            // Only one flag in the group can be used
		UseAll             // All flags must be used
	};

	FlagGroup(const Type &type, bool optional, const std::vector<Flag *> &flags = {});

	FlagGroup(const std::vector<FlagGroup> &groups = {});

	bool is_optional() const;

	Type get_type() const;

	std::string get_command() const;

	std::vector<Flag *> get_flags() const;

  private:
	Type                   type;
	bool                   optional;
	std::vector<Flag *>    flags;
	std::vector<FlagGroup> groups;
};
}        // namespace vkb