/* Copyright (c) 2019, Arm Limited and Contributors
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

#include <docopt.h>

#include "common/logging.h"

namespace vkb
{
/**
 * @brief Class that handles and formats arguments passed into the application
 */
class Options
{
  public:
	Options() = default;

	/**
	 * @brief Parses the arguments, forcing an exit if it fails
	 * @param usage The usage string of the application
	 * @param args The arguments supplied to the program
	 */
	void parse(const std::string &usage, const std::vector<std::string> &args);

	/**
	 * @brief Helper function that determines if key exists within parsed args
	 * @param argument The argument to check for
	 * @returns True if the argument exists in the parsed_arguments
	 */
	bool contains(const std::string &argument) const;

	/**
	 * @brief Helper function to return the integer value from a flag
	 * @param argument The flag to check for
	 * @returns An integer representation of the value under the argument
	 */
	const int32_t get_int(const std::string &argument) const;

	/**
	 * @brief Helper function to return the string value from a flag
	 * @param argument The flag to check for
	 * @returns A string representation of the value under the argument
	 */
	const std::string get_string(const std::string &argument) const;

	/**
	 * @brief Helper function to return the a vector of string args
	 * @param argument The flag to check for
	 * @returns A vector representation of the value under the argument
	 */
	const std::vector<std::string> get_list(const std::string &argument) const;

	/**
	 * @brief Prints a formatted usage of the arguments
	 */
	void print_usage() const;

  private:
	std::string usage;

	std::map<std::string, docopt::value> parse_result;
};
}        // namespace vkb
