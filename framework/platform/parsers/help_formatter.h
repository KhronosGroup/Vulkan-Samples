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

#include <functional>
#include <unordered_map>

#include <CLI/FormatterFwd.hpp>
namespace vkb
{
class HelpFormatter : public CLI::Formatter
{
  public:
	struct Meta
	{
		std::string name;
		std::string description;
	};

	HelpFormatter()                      = default;
	HelpFormatter(const HelpFormatter &) = default;
	HelpFormatter(HelpFormatter &&)      = default;

	std::string         make_help(const CLI::App *, std::string, CLI::AppFormatMode) const override;
	virtual std::string make_expanded(const CLI::App *sub) const override;

	void register_meta(const CLI::App *command, const Meta &meta);

  private:
	std::unordered_map<const CLI::App *, Meta> _meta;

	const Meta *get_meta(const CLI::App *command) const;
};
}        // namespace vkb