#include <catch2/catch_test_macros.hpp>

#include <shaders/shader_handle.hpp>

TEST_CASE("ShaderHandleBuilder", "[shaders]")
{
	auto handle = vkb::ShaderHandleBuilder{}
	                  .with_path("shaders/base.vert.glsl")
	                  .with_define(vkb::HAS_BASE_COLOR_TEXTURE)
	                  .build();

	REQUIRE(handle.hash.size() > 0);
	REQUIRE(handle.path.size() > 0);
	REQUIRE(handle.define_hash.size() > 0);
	REQUIRE(handle.defines.size() > 0);

	REQUIRE(handle.hash == "4ab1222007bfad22f64e20964aff29c764ba0ac9e65ebd6fbf196d6380237d72");
	REQUIRE(handle.path == "shaders/base.vert.glsl");
	REQUIRE(handle.define_hash == "63f1fb56ceff809846cfec8edea494dd2c1fd928992bb0ed240bc1a114cc80bc");
	REQUIRE(handle.defines.size() == 1);
	REQUIRE(handle.defines[0] == vkb::HAS_BASE_COLOR_TEXTURE);
}