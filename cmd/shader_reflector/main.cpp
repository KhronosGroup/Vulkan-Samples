#include <core/platform/entrypoint.hpp>
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

	std::string stage = "unknown";
	app.add_option("-s,--stage", stage, "Shader Stage")->required();

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

	auto spriv = fs->read_binary_file<uint32_t>(input_file);

	vkb::SpirvReflector reflector;

	auto resource_set     = reflector.reflect(spriv);
	auto shader_resources = resource_set.get_resources();

	std::vector<json> resources;

	for (auto &shader_resource : shader_resources)
	{
		json resource;
		resource["name"]                   = shader_resource.name;
		resource["type"]                   = "unknown";
		resource["set"]                    = shader_resource.set;
		resource["binding"]                = shader_resource.binding;
		resource["location"]               = shader_resource.location;
		resource["input_attachment_index"] = shader_resource.input_attachment_index;
		resource["vec_size"]               = shader_resource.vec_size;
		resource["columns"]                = shader_resource.columns;
		resource["array_size"]             = shader_resource.array_size;
		resource["offset"]                 = shader_resource.offset;
		resource["size"]                   = shader_resource.size;
		resource["constant_id"]            = shader_resource.constant_id;
		resource["qualifiers"]             = shader_resource.qualifiers;

		resources.push_back(resource);
	}

	json reflection;
	reflection["file"]      = input_file;
	reflection["defines"]   = defines;
	reflection["entry"]     = "main";
	reflection["stage"]     = stage;
	reflection["resources"] = resources;

	fs->write_file(output_file, reflection.dump(4));

	return 0;
}