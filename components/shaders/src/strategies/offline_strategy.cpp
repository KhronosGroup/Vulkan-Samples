#include "shaders/strategies/offline_strategy.hpp"

#include <nlohmann/json.hpp>

#include <core/util/logging.hpp>
#include <fs/filesystem.hpp>

#include <shaders/reflectors/spirv_reflector.hpp>

namespace vkb
{

OfflineShaderCacheStrategy::OfflineShaderCacheStrategy()
{
	load_atlas("generated/shader_atlas.json");
}

void OfflineShaderCacheStrategy::load_atlas(const std::string &atlas_path)
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

			auto variant_code = fs->read_binary_file<uint32_t>(variant_file);
			if (variant_code.empty())
			{
				LOGE("Failed to load shader {} with defines {}", shader_path, define_hash);
				continue;
			}

			auto   resources = reflector.reflect(variant_code);
			Shader shader{variant_code, std::move(resources)};

			auto it = atlas.shaders.find(shader_path);
			if (it != atlas.shaders.end())
			{
				it->second.variants[define_hash] = shader;
				continue;
			}

			AtlasShader atlas_shader;
			atlas_shader.variants[define_hash] = shader;
			atlas.shaders[shader_path]         = atlas_shader;
		}
	}

	LOGI("Loaded {} shaders", atlas.shaders.size());
}

std::vector<uint32_t> OfflineShaderCacheStrategy::load_shader(const ShaderHandle &handle)
{
	try
	{
		auto &atlas_shader = atlas.shaders.at(handle.path);

		LOGI("variants: {}", atlas_shader.variants.size());

		auto &shader = atlas_shader.variants.at(handle.define_hash);

		return shader.code;
	}
	catch (const std::exception &e)
	{
		LOGE("Failed to load shader {} with defines {}", handle.path, handle.define_hash);
		return {};
	}
}

ShaderResourceSet OfflineShaderCacheStrategy::reflect(const std::vector<uint32_t> &code)
{
	return {};
}

}        // namespace vkb