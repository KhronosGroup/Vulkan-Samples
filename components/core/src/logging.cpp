#include "core/util/logging.hpp"

#include "spdlog/cfg/env.h"

#ifdef PLATFORM__ANDROID
#	include "spdlog/sinks/android_sink.h"
#else
#	include "spdlog/sinks/stdout_color_sinks.h"
#endif

namespace vkb
{
namespace logging
{
void init()
{
	// Taken from "spdlog/cfg/env.h" and renamed SPDLOG_LEVEL to VKB_LEVEL
	auto env_val = spdlog::details::os::getenv("VKB_LEVEL");
	if (!env_val.empty())
	{
		spdlog::cfg::helpers::load_levels(env_val);
	}

#ifdef PLATFORM__ANDROID
	auto logger = spdlog::android_logger_mt("vkb", "VulkanSamples");
#else
	auto logger = spdlog::stdout_color_mt("vkb");
#endif

	logger->set_pattern(LOGGER_FORMAT);
	logger->set_level(spdlog::level::trace);
	spdlog::set_default_logger(logger);
}
}        // namespace logging
}        // namespace vkb