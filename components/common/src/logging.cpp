#include "components/common/logging.hpp"

#include <components/common/error.hpp>

VKBP_DISABLE_WARNINGS()
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
VKBP_ENABLE_WARNINGS()

namespace components
{
namespace logging
{
void init_default_logger()
{
	spdlog::set_level(spdlog::level::trace);
	spdlog::set_pattern("[%^%l%$] %v");

	// TODO: Add android support
	auto console = spdlog::stdout_color_mt("console");

	spdlog::set_default_logger(console);
}
}        // namespace logging
}        // namespace components