#pragma once

#include <CLI/Formatter.hpp>

namespace vkb
{
class HelpFormatter : public CLI::Formatter
{
  public:
	HelpFormatter()                      = default;
	HelpFormatter(const HelpFormatter &) = default;
	HelpFormatter(HelpFormatter &&)      = default;

	std::string         make_subcommands(const CLI::App *app, CLI::AppFormatMode mode, int depth) const;
	std::string         make_expanded(const CLI::App *sub, int depth) const;
	virtual std::string make_help(const CLI::App *app, std::string, CLI::AppFormatMode) const override;
};
}        // namespace vkb