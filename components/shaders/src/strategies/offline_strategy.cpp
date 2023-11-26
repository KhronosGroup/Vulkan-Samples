#include "shaders/strategies/offline_strategy.hpp"

#include <nlohmann/json.hpp>

#include <core/util/logging.hpp>
#include <filesystem/filesystem.hpp>

#include <shaders/reflectors/spirv_reflector.hpp>

namespace vkb
{

OfflineShaderstrategy::OfflineShaderstrategy()
{
	load_atlas("generated/shader_atlas.json");
}

void OfflineShaderstrategy::load_atlas(const std::string &atlas_path)
{
	auto fs = fs::get_filesystem();
	if (!fs->exists(atlas_path))
	{
		LOGE("Atlas file {} does not exist", atlas_path);
		return;
	}

	auto contents = fs->read_file(atlas_path);

	LOGI("Loading shader atlas {}", atlas_path);

	nlohmann::json atlas_json = nlohmann::json::parse(contents);

	SpirvReflector reflector;

	for (auto &[shader_path, shader_json] : atlas_json.items())
	{
		auto &variants_json = shader_json["variants"];
		for (auto &[define_hash, variant_json] : variants_json.items())
		{
			auto variant_defines = variant_json["defines"].get<std::vector<std::string>>();
			auto variant_file    = variant_json["file"].get<std::string>();
			auto variant_hash    = variant_json["hash"].get<std::string>();

			auto variant_code = fs->read_binary_file<uint8_t>(variant_file);
			if (variant_code.empty())
			{
				LOGE("Failed to load shader {} with defines {}", shader_path, define_hash);
				continue;
			}

			auto resources = reflector.reflect(variant_code);

			ShaderPtr shader = std::make_shared<Shader>(std::vector<uint32_t>{variant_code.begin(), variant_code.end()}, std::move(resources));

			auto it = atlas.shaders.find(shader_path);
			if (it != atlas.shaders.end())
			{
				it->second.variants[define_hash] = std::move(shader);
				continue;
			}

			AtlasShader atlas_shader;
			atlas_shader.variants[define_hash] = std::move(shader);
			atlas.shaders[shader_path]         = atlas_shader;
		}
	}

	LOGI("Loaded {} shaders", atlas.shaders.size());
}

ShaderPtr OfflineShaderstrategy::load_shader_from_atlas(const ShaderHandle &handle)
{
	try
	{
		auto &atlas_shader = atlas.shaders.at(handle.path);
		auto &shader       = atlas_shader.variants.at(handle.define_hash);
		return shader;
	}
	catch (const std::exception &e)
	{
		throw std::runtime_error{fmt::format("Failed to load shader {} with defines {}", handle.path, handle.define_hash)};
	}
}

ShaderPtr OfflineShaderstrategy::load_shader(const ShaderHandle &handle)
{
	return load_shader_from_atlas(handle);
}

std::vector<uint32_t> OfflineShaderstrategy::load_spirv(const ShaderHandle &handle)
{
	return load_shader_from_atlas(handle)->code;
}

ShaderResourceSet OfflineShaderstrategy::reflect(const ShaderHandle &handle)
{
	return load_shader_from_atlas(handle)->resource_set;
}

}        // namespace vkb