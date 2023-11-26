#include <core/platform/entrypoint.hpp>
#include <core/util/strings.hpp>
#include <filesystem/filesystem.hpp>
#include <shaders/reflectors/spirv_reflector.hpp>

#include <CLI/CLI.hpp>
#include <nlohmann/json.hpp>
#include <vulkan/vulkan.hpp>
using namespace nlohmann;

const std::vector<vkb::ShaderResourceType> REFLECTED_TYPES = {
    vkb::ShaderResourceType::Input,
    vkb::ShaderResourceType::InputAttachment,
    vkb::ShaderResourceType::Output,
    vkb::ShaderResourceType::Image,
    vkb::ShaderResourceType::ImageSampler,
    vkb::ShaderResourceType::ImageStorage,
    vkb::ShaderResourceType::Sampler,
    vkb::ShaderResourceType::BufferUniform,
    vkb::ShaderResourceType::BufferStorage,
    vkb::ShaderResourceType::PushConstant,
    vkb::ShaderResourceType::SpecializationConstant,
};

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

	for (auto type : REFLECTED_TYPES)
	{
		auto              res = resource_set.get_resources(type);
		std::vector<json> shader_resources;
		std::transform(res.begin(), res.end(), std::back_inserter(shader_resources),
		               [](const vkb::ShaderResource &input) { return input.to_json(); });
		resources[vkb::to_string(type)] = shader_resources;
	}

	reflection["resources"] = resources;

	fs->write_file(output_file, reflection.dump(4));

	return 0;
}