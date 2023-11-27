#include <catch2/catch_test_macros.hpp>

#include <shaders/shader_cache.hpp>
#include <shaders/strategies/offline_strategy.hpp>

TEST_CASE("ShaderCache throws", "[shaders]")
{
	vkb::ShaderCache *shader_cache = vkb::ShaderCache::get();
	shader_cache->set_strategy(nullptr);

	REQUIRE_THROWS(shader_cache->load_shader({}));
	REQUIRE_THROWS(shader_cache->load_spirv({}));
	REQUIRE_THROWS(shader_cache->reflect({}));
}

TEST_CASE("ShaderCache loads", "[shaders]")
{
	vkb::ShaderCache *shader_cache = vkb::ShaderCache::get();
	shader_cache->set_strategy(std::make_unique<vkb::OfflineShaderStrategy>());

	auto handle = vkb::ShaderHandleBuilder{}
	                  .with_path("shaders/base.vert.glsl")
	                  .with_define(vkb::HAS_BASE_COLOR_TEXTURE)
	                  .build();

	auto shader = shader_cache->load_shader(handle);

	REQUIRE(shader->code.size() > 0);
	REQUIRE(shader->resource_set.resources().size() > 0);
}