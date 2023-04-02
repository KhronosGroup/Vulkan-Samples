#include <components/common/logging.hpp>

#include <CLI/CLI.hpp>
#include <nlohmann/json.hpp>

#include <components/platform/platform.hpp>

#include "header_generator.hpp"
#include "shader_descriptor.hpp"

using json = nlohmann::json;
using namespace components;

CUSTOM_MAIN(platform_context)
{
	logging::init_default_logger();

	CLI::App app{"Shader Compiler. Compiles shaders to SPIR-V."};

	CLI::Option *schema_option = app.add_option("--json-schema", "A JSON schema string");
	CLI::Option *output_option = app.add_option("--output", "Where to write the compiled shader header to");

	try
	{
		app.parse(platform_context->arguments());
	}
	catch (const CLI::ParseError &e)
	{
		return app.exit(e);
	}

	json json_data;
	try
	{
		std::string json_schema = schema_option->as<std::string>();

		json_data = json::parse(json_schema);
	}
	catch (json::parse_error &e)
	{
		LOGE("Error processing shader: {}", e.what());
		return -1;
	}

	shader_compiler::ShaderDescriptor descriptor;
	try
	{
		descriptor = shader_compiler::shader_descriptor_from_json(json_data);
		shader_compiler::compile_and_reflect_shader(descriptor);

		std::string output_file = output_option->as<std::string>();
		shader_compiler::generate_shader_header(descriptor, output_file);
	}
	catch (std::exception &e)
	{
		shader_compiler::print_shader_descriptor(descriptor);
		LOGE("Error processing shader: {}", e.what());
		// return -1;
	}

	return 0;
}