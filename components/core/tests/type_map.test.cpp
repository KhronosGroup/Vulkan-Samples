#include <catch2/catch_test_macros.hpp>

#include "core/containers/type_map.hpp"

using namespace vkb;

TEST_CASE("TypeMap", "[containers]")
{
	SECTION("find_or_insert")
	{
		vkb::TypeMap<int> map;

		int value = 0;

		auto it = map.find_or_insert<int>([&]() { return value++; });

		REQUIRE(it->second == 0);

		it = map.find_or_insert<int>([&]() { return value++; });

		REQUIRE(it->second == 0);

		it = map.find_or_insert<float>([&]() { return value++; });

		REQUIRE(it->second == 1);
	}

	SECTION("replace_emplace")
	{
		vkb::TypeMap<int> map;

		auto it = map.replace_emplace<int>(0);
		REQUIRE(it->second == 0);

		it = map.replace_emplace<int>(1);
		REQUIRE(it->second == 1);

		REQUIRE(map.find<int>()->second == 1);
	}

	SECTION("find")
	{
		vkb::TypeMap<int> map;

		int value = 0;

		map.replace_emplace<int>(value++);

		auto it = map.find<int>();

		REQUIRE(it->second == 0);

		it = map.find<float>();

		REQUIRE(it == map.end());
	}

	SECTION("find const")
	{
		vkb::TypeMap<int> map;

		int value = 0;

		map.replace_emplace<int>(value++);

		auto it = map.find<int>();

		REQUIRE(it->second == 0);

		it = map.find<float>();

		REQUIRE(it == map.end());
	}

	SECTION("contains", "[containers]")
	{
		vkb::TypeMap<int> map;

		int value = 0;

		map.replace_emplace<int>(value++);

		REQUIRE(map.contains<int>() == true);
		REQUIRE(map.contains<float>() == false);
	}
}