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

#pragma once

#include <map>
#include <string>

#include <docopt.h>

#include "platform/plugins/plugin.h"
#include "platform/plugins/flag.h"

namespace vkb
{
using DocoptMap = std::map<std::string, docopt::value>;

class Parser
{
  public:
	/**
	 * @brief Construct a new Parser object
	 *
	 * @param plugins Plugins used to generate CLI help menu
	 */
	Parser(const std::vector<Plugin *> &plugins = {});

	/**
	 * @brief Parse a list of arguments against the plugins
	 * 
	 * @param args A list of arguments to be considered
	 * @return true If parsing is a success
	 * @return false If parsing is a failure
	 */
	bool parse(const std::vector<std::string> &args = {});

	/**
	 * @brief Check if a flag is contained in the parser
	 * 
	 * @param flag The flag object used to query a flags existence
	 * @return true If the parser contains the flag
	 * @return false If the parser does not contain the flag
	 */
	bool contains(const Flag &flag) const;

	/**
	 * @brief Get the bool object of a given flag
	 * 
	 * @param flag Flag used to retrieve bool value
	 * @return A boolean value stored by the flag
	 */
	const bool get_bool(const Flag &flag) const;

	/**
	 * @brief Get the int object of a given flag
	 * 
	 * @param flag Flag used to retrieve int value
	 * @return A integer value stored by the flag
	 */
	const int32_t get_int(const Flag &flag) const;

	/**
	 * @brief Get the string object of a given flag
	 * 
	 * @param flag Flag used to retrieve string value
	 * @return A string value stored by the flag
	 */
	const std::string get_string(const Flag &flag) const;

	/**
	 * @brief Get the list object of a given flag
	 * 
	 * @param flag Flag used to retrieve a list
	 * @return A list value stored by the flag
	 */
	const std::vector<std::string> get_list(const Flag &flag) const;

	void print_help();

  private:
	// Help prompt that is printed to the user
	std::vector<std::string> help;
	std::vector<std::string> extra_help;
	// A map of string - variant pair that is derived from a list of argumnets
	DocoptMap parsed_args;

	/**
	 * @brief Get a variant of values from the docopt map
	 *
	 * @param key The element in the map to retrieve
	 * @return const docopt::value& 
	 */
	const docopt::value &get(const std::string &key) const;

	/**
	 * @brief Check if a flag is contained in the parser
	 *
	 * @param key Key used to retrieve bool value - use get_key(Flag)
	 * @return true If the parser contains the flag
	 * @return false If the parser does not contain the flag
	 */
	bool contains(const std::string &key) const;
};
}        // namespace vkb
