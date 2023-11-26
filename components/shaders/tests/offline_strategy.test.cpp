#include <catch2/catch_test_macros.hpp>

#include <shaders/shader_cache.hpp>
#include <shaders/strategies/offline_strategy.hpp>

TEST_CASE("OfflineShaderStrategy", "[shaders]")
{
	vkb::ShaderCache cache{std::make_unique<vkb::OfflineShaderCacheStrategy>()};

	auto handle = vkb::ShaderHandleBuilder{}
	                  .with_path("shaders/base.vert.glsl")
	                  .with_define(vkb::HAS_BASE_COLOR_TEXTURE)
	                  .build();

	auto shader = cache.load_shader(handle);

	REQUIRE(shader.code.size() > 0);
}