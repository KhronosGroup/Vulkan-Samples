#include <core/platform/entrypoint.hpp>
#include <core/util/strings.hpp>
#include <fs/filesystem.hpp>
#include <shaders/reflectors/spirv_reflector.hpp>

#include <CLI/CLI.hpp>
#include <nlohmann/json.hpp>
#include <vulkan/vulkan.hpp>
using namespace nlohmann;

CUSTOM_MAIN(context)
{
	CLI::App app{"Shader Reflector"};

	std::string input_file;
	app.add_option("input_file", input_file, "SPIRV Input File")->required();

	std::string output_file;
	app.add_option("output_file", output_file, "Reflection Output File")->required();

	std::string stage;
	app.add_option("stage", stage, "Shader Stage")->required();

	std::vector<std::string> defines;
	app.add_option("-D,--define", defines, "Preprocessor defines");

	try
	{
		app.parse();
	}
	catch (const CLI::ParseError &e)
	{
		return app.exit(e);
	}

	auto fs = vkb::fs::get_filesystem();

	auto spriv = fs->read_binary_file<uint8_t>(input_file);

	vkb::SpirvReflector reflector;

	auto resource_set = reflector.reflect(spriv);

	json reflection;
	reflection["file"]    = "./" + vkb::fs::filename(input_file);
	reflection["defines"] = defines;
	reflection["entry"]   = "main";
	reflection["stage"]   = stage;

	json resources;

	{
		auto inputs = resource_set.get_resources(vkb::ShaderResourceType::Input);

		std::vector<json> shader_resources;

		for (auto &input : inputs)
		{
			json resource;
			resource["name"]                   = input.name;
			resource["type"]                   = "input";
			resource["location"]               = input.location;
			resource["input_attachment_index"] = input.input_attachment_index;
			resource["vec_size"]               = input.vec_size;
			resource["columns"]                = input.columns;
			resource["array_size"]             = input.array_size;
			resource["offset"]                 = input.offset;
			resource["size"]                   = input.size;
			resource["constant_id"]            = input.constant_id;
			resource["qualifiers"]             = input.qualifiers;

			shader_resources.push_back(resource);
		}

		resources["input"] = shader_resources;
	}

	reflection["resources"] = resources;

	fs->write_file(output_file, reflection.dump(4));

	return 0;
}