#include "core/util/logging.hpp"

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
	// Load logger from environment variables
	char *env = getenv("VKB_LEVEL");
	if (env)
	{
		std::string levelStr = env;
#define SET_LEVEL(envLevel)    \
	if (levelStr == #envLevel) \
	spdlog::set_level(spdlog::level::envLevel)
		SET_LEVEL(trace);
		SET_LEVEL(debug);
		SET_LEVEL(info);
		SET_LEVEL(warn);
		SET_LEVEL(err);
		SET_LEVEL(critical);
		SET_LEVEL(off);
#undef SET_LEVEL
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