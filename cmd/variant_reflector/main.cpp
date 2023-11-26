#include <filesystem>
#include <regex>

#include <CLI/CLI.hpp>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include <tiny_gltf.h>

#include <core/platform/entrypoint.hpp>
#include <core/util/logging.hpp>
#include <core/util/strings.hpp>
#include <filesystem/filesystem.hpp>

struct Define
{
	std::string name;
	std::string value;

	bool operator<(const Define &other) const
	{
		if (name < other.name)
		{
			return true;
		}

		if (name > other.name)
		{
			return false;
		}
		// Only compares value if names are equal
		return value < other.value;
	}
};

struct ShaderVariant
{
	std::set<Define> defines;

	bool operator<(const ShaderVariant &other) const
	{
		return defines < other.defines;
	}
};

struct ShaderVariantFile
{
	std::string             file_path;
	std::set<ShaderVariant> variants;
};

ShaderVariantFile process_model_file(const std::string &file_path)
{
	LOGI("Processing file: {}", file_path);

	tinygltf::Model    model;
	tinygltf::TinyGLTF loader;
	std::string        err;
	std::string        warn;
	bool               ret = loader.LoadASCIIFromFile(&model, &err, &warn, file_path);

	if (!warn.empty())
	{
		LOGW("{}", warn);
	}

	if (!err.empty())
	{
		LOGE("{}", err);
	}

	if (!ret)
	{
		LOGE("Failed to parse glTF");
		return {};
	}

	std::set<ShaderVariant> variants;

	for (auto &gltf_material : model.materials)
	{
		ShaderVariant variant;

		for (auto &gltf_value : gltf_material.values)
		{
			if (gltf_value.first.find("Texture") != std::string::npos)
			{
				std::string tex_name = vkb::to_snake_case(gltf_value.first);
				tex_name             = "HAS_" + vkb::to_upper_case(tex_name);
				variant.defines.insert({tex_name, "1"});
			}
		}

		for (auto &gltf_value : gltf_material.additionalValues)
		{
			if (gltf_value.first.find("Texture") != std::string::npos)
			{
				std::string tex_name = vkb::to_snake_case(gltf_value.first);
				tex_name             = "HAS_" + vkb::to_upper_case(tex_name);
				variant.defines.insert({tex_name, "1"});
			}
		}

		variants.insert(variant);
	}

	return {file_path, variants};
}

std::vector<std::string> collect_model_paths(const std::string &asset_dir)
{
	std::set<std::string> paths;

	std::filesystem::path asset_dir_path(asset_dir);

	try
	{
		if (std::filesystem::exists(asset_dir_path) && std::filesystem::is_directory(asset_dir_path))
		{
			for (const auto &entry : std::filesystem::recursive_directory_iterator(asset_dir_path))
			{
				if (entry.path().extension() == ".gltf")
				{
					paths.insert(entry.path().string());
				}
			}
		}
	}
	catch (const std::filesystem::filesystem_error &e)
	{
		std::cerr << e.what() << std::endl;
	}
	return {paths.begin(), paths.end()};
}

std::vector<ShaderVariantFile> collect_model_variants(const std::vector<std::string> &model_paths)
{
	std::vector<ShaderVariantFile> variants;

	for (auto &model_path : model_paths)
	{
		variants.push_back(process_model_file(model_path));
	}

	return variants;
}

ShaderVariantFile process_shader_file(const std::string &file_path)
{
	LOGI("Processing file: {}", file_path);

	auto fs = vkb::fs::get_filesystem();

	if (!fs->exists(file_path))
	{
		LOGE("File does not exist: {}", file_path);
		return {};
	}

	auto shader_source = fs->read_file(file_path);

	std::set<Define> defines;

	std::regex                  define_regex("#ifdef +([A-Za-z0-9_]+)");
	std::smatch                 match;
	std::string::const_iterator start(shader_source.cbegin());
	while (std::regex_search(start, shader_source.cend(), match, define_regex))
	{
		defines.insert({match[1], "1"});
		start += match.position() + match.length();
	}

	return ShaderVariantFile{file_path, {ShaderVariant{defines}}};
}

std::vector<std::string> shader_extensions = {
    "vert",
    "tesc",
    "tese",
    "geom",
    "frag",
    "comp",
    "rchit",
    "rahit",
    "rmiss",
    "rint",
    "rcall",
    "rgen",
    "task",
    "mesh",
};

std::vector<std::string> shader_types = {
    "glsl",
    "hlsl",
};

std::vector<std::string> generate_shader_extensions()
{
	std::vector<std::string> extensions;
	for (auto &type : shader_types)
	{
		for (auto &extension : shader_extensions)
		{
			extensions.push_back(fmt::format(".{}.{}", extension, type));
		}
	}

	// Add .h for header files
	extensions.push_back(".h");

	return extensions;
}

std::vector<std::string> collect_shader_paths(const std::string &shader_dir)
{
	std::vector<std::string> paths;
	std::filesystem::path    shader_dir_path(shader_dir);

	auto shader_extensions = generate_shader_extensions();

	try
	{
		if (std::filesystem::exists(shader_dir_path) && std::filesystem::is_directory(shader_dir_path))
		{
			for (const auto &entry : std::filesystem::recursive_directory_iterator(shader_dir_path))
			{
				for (auto &extension : shader_extensions)
				{
					if (vkb::ends_with(entry.path().string(), extension))
					{
						paths.push_back(entry.path().string());
						// break;
					}
				}
			}
		}
	}
	catch (const std::filesystem::filesystem_error &e)
	{
		LOGE("{}", e.what());
	}
	return {paths.begin(), paths.end()};
}

std::vector<ShaderVariantFile> collect_shader_variants(const std::vector<std::string> &shader_paths)
{
	std::vector<ShaderVariantFile> variants;

	for (auto &shader_path : shader_paths)
	{
		variants.push_back(process_shader_file(shader_path));
	}

	return variants;
}

std::set<ShaderVariant> to_unique_variants(const std::vector<ShaderVariantFile> &variant_files)
{
	std::set<ShaderVariant> unique_variants;

	for (auto &variant_file : variant_files)
	{
		for (auto &variant : variant_file.variants)
		{
			unique_variants.insert(variant);
		}
	}

	return unique_variants;
}

nlohmann::json create_variant_json(const std::set<ShaderVariant> &variants)
{
	nlohmann::json json = nlohmann::json::object();
	json["variants"]    = nlohmann::json::array();

	for (auto &variant : variants)
	{
		nlohmann::json json_variant = nlohmann::json::object();
		json_variant["defines"]     = nlohmann::json::array();

		for (auto &define : variant.defines)
		{
			json_variant["defines"].push_back(fmt::format("{}={}", define.name, define.value));
		}

		json["variants"].push_back(json_variant);
	}

	return json;
}

CUSTOM_MAIN(context)
{
	CLI::App app{"Variant Reflector"};

	std::string asset_dir;
	app.add_option("--asset-dir", asset_dir, "Asset Directory")->required();

	std::string output_file;
	app.add_option("-o,--output", output_file, "Output file")->required();

	std::string shader_dir;
	app.add_option("--shader-dir", shader_dir, "Shader directory")->required();

	try
	{
		app.parse();
	}
	catch (const CLI::ParseError &e)
	{
		return app.exit(e);
	}

	auto models          = collect_model_paths(asset_dir);
	auto model_variants  = collect_model_variants(models);
	auto shaders         = collect_shader_paths(shader_dir);
	auto shader_variants = collect_shader_variants(shaders);

	auto unique_model_variants = to_unique_variants(model_variants);
	LOGI("Unique model variants: {}", unique_model_variants.size());

	auto unique_shader_variants = to_unique_variants(shader_variants);
	LOGI("Unique shader variants: {}", unique_shader_variants.size());

	auto unique_variants = unique_model_variants;
	unique_variants.insert(unique_shader_variants.begin(), unique_shader_variants.end());
	LOGI("Unique variants: {}", unique_variants.size());

	auto json = create_variant_json(unique_variants);

	auto fs = vkb::fs::get_filesystem();
	fs->write_file(output_file, json.dump(4));

	return 0;
}