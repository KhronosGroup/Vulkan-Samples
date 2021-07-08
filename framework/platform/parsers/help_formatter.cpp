#include "help_formatter.h"

namespace vkb
{
std::string HelpFormatter::make_help(const CLI::App *app, std::string name, CLI::AppFormatMode mode) const
{
	// This immediately forwards to the make_expanded method. This is done this way so that subcommands can
	// have overridden HelpFormatters
	if (mode == CLI::AppFormatMode::Sub)
		return make_expanded(app, INT_MAX);

	std::stringstream out;

	if ((app->get_name().empty()) && (app->get_parent() != nullptr))
	{
		if (app->get_group() != "Subcommands")
		{
			out << app->get_group() << ':';
		}
	}

	out << make_description(app);
	out << make_usage(app, name);
	out << make_positionals(app);
	out << make_groups(app, mode);
	out << make_subcommands(app, mode, 2);

	out << '\n'
	    << make_footer(app);

	return out.str();
}

std::string HelpFormatter::make_subcommands(const CLI::App *app, CLI::AppFormatMode mode, int depth) const
{
	std::stringstream out;

	std::vector<const CLI::App *> subcommands = app->get_subcommands({});

	// Make a list in definition order of the groups seen
	std::vector<std::string> subcmd_groups_seen;
	for (const CLI::App *com : subcommands)
	{
		if (!com->get_group().empty() && !com->get_name().empty())
		{
			out << make_expanded(com, depth - 1);
			continue;
		}

		std::string group_key = com->get_group();
		if (!group_key.empty() &&
		    std::find_if(subcmd_groups_seen.begin(), subcmd_groups_seen.end(), [&group_key](std::string a) {
			    return CLI::detail::to_lower(a) == CLI::detail::to_lower(group_key);
		    }) == subcmd_groups_seen.end())
			subcmd_groups_seen.push_back(group_key);
	}

	// For each group, filter out and print subcommands
	for (const std::string &group : subcmd_groups_seen)
	{
		out << "\n"
		    << group << ":\n";
		std::vector<const CLI::App *> subcommands_group = app->get_subcommands(
		    [&group](const CLI::App *sub_app) { return CLI::detail::to_lower(sub_app->get_group()) == CLI::detail::to_lower(group); });
		for (const CLI::App *new_com : subcommands_group)
		{
			if (new_com->get_name().empty())
				continue;
			if (mode != CLI::AppFormatMode::All)
			{
				out << make_subcommand(new_com);
			}
			else
			{
				out << new_com->help(new_com->get_name(), CLI::AppFormatMode::Sub);
				out << "\n";
			}
		}
	}

	return out.str();
}

std::string HelpFormatter::make_expanded(const CLI::App *sub, int depth) const
{
	std::stringstream out;
	out << "\n"
	    << sub->get_display_name(true) << "\n";

	if (depth > 1)
	{
		out << "\n";
	}

	out << make_description(sub) << "\n";

	if (sub->get_name().empty() && !sub->get_aliases().empty())
	{
		CLI::detail::format_aliases(out, sub->get_aliases(), column_width_ + 2);
	}
	out << make_positionals(sub);
	out << make_groups(sub, CLI::AppFormatMode::Sub);

	if (depth > 0)
	{
		out << make_subcommands(sub, CLI::AppFormatMode::Sub, depth);
	}

	// Drop blank spaces
	std::string tmp = CLI::detail::find_and_replace(out.str(), "\n\n", "\n");
	tmp             = tmp.substr(0, tmp.size() - 1);        // Remove the final '\n'

	// Indent all but the first line (the name)
	return CLI::detail::find_and_replace(tmp, "\n", "\n  ") + "\n";
}
}        // namespace vkb