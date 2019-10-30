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

#include "options.h"

#include <sstream>

namespace vkb
{
void Options::parse(const std::string &usage, const std::vector<std::string> &args)

{
	if (usage.size() != 0)
	{
		this->usage = usage;

		if (args.size() > 0)
		{
			this->parse_result = docopt::docopt(usage, args, false);
		}
	}
}

bool Options::contains(const std::string &argument) const
{
	if (parse_result.count(argument) != 0)
	{
		if (const auto &result = parse_result.at(argument))
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

const int32_t Options::get_int(const std::string &argument) const
{
	if (contains(argument))
	{
		auto result = parse_result.at(argument);
		if (result.isString())
		{
			return std::stoi(result.asString());
		}
		else if (result.isLong())
		{
			return static_cast<int32_t>(result.asLong());
		}

		throw std::runtime_error("Argument option is not int type");
	}

	throw std::runtime_error("Couldn't find argument option");
}

const std::string Options::get_string(const std::string &argument) const
{
	if (contains(argument))
	{
		auto result = parse_result.at(argument);
		if (result.isString())
		{
			return result.asString();
		}

		throw std::runtime_error("Argument option is not string type");
	}

	throw std::runtime_error("Couldn't find argument option");
}

const std::vector<std::string> Options::get_list(const std::string &argument) const
{
	if (contains(argument))
	{
		auto result = parse_result.at(argument);
		if (result.isStringList())
		{
			return result.asStringList();
		}

		throw std::runtime_error("Argument option is not vector of string type");
	}

	throw std::runtime_error("Couldn't find argument option");
}

void Options::print_usage() const
{
	if (!usage.empty())
	{
		std::stringstream sstream(usage);
		std::string       token;
		while (std::getline(sstream, token, '\n'))
		{
			LOGI(token);
		}
	}
}
}        // namespace vkb
