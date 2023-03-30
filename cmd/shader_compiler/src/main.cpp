#include <components/common/logging.hpp>

#include <CLI/CLI.hpp>

#include <components/platform/platform.hpp>

#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace components;

CUSTOM_MAIN(platform_context)
{
	logging::init_default_logger();

	CLI::App app{"Shader Compiler. Compiles shaders to SPIR-V."};

	CLI::Option *option = app.add_option("--json-schema", "A JSON schema string");

	try
	{
		app.parse(platform_context->arguments());
	}
	catch (const CLI::ParseError &e)
	{
		return app.exit(e);
	}

	std::string json_schema = option->as<std::string>();

	LOGI("Starting Shader Compiler");

	try
	{
		json data = json::parse(json_schema);
		LOGI("Parsed JSON");

		if (data.contains("language"))
		{
			auto language = data.at("language").get<std::string>();
			LOGI("Language: {}", language.c_str());
		}

		if (data.contains("vertex"))
		{
			auto vertex = data.at("vertex").get<std::string>();
			LOGI("Vertex: {}", vertex.c_str());
		}

		if (data.contains("fragment"))
		{
			auto fragment = data.at("fragment").get<std::string>();
			LOGI("Fragment: {}", fragment.c_str());
		}

		if (data.contains("compute"))
		{
			auto compute = data.at("compute").get<std::string>();
			LOGI("Compute: {}", compute.c_str());
		}

		if (data.contains("includeFolders"))
		{
			auto includes = data.at("includeFolders").get<std::vector<std::string>>();
			for (auto &include : includes)
			{
				LOGI("Include Folder: {}", include.c_str());
			}
		}
	}
	catch (json::parse_error &e)
	{
		LOGE("Error parsing JSON: %s", e.what());
	}

	return 0;
}