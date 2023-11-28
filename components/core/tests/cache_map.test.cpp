#include <catch2/catch_test_macros.hpp>

#include <map>

#include "core/containers/cache_map.hpp"

TEST_CASE("CacheMap", "[containers]")
{
	SECTION("find_or_emplace")
	{
		vkb::CacheMap<int, int> map;

		int value = 0;

		auto it = map.find_or_emplace(0, [&]() { return value++; });

		REQUIRE(it->second == 0);

		it = map.find_or_emplace(0, [&]() { return value++; });

		REQUIRE(it->second == 0);

		it = map.find_or_emplace(1, [&]() { return value++; });

		REQUIRE(it->second == 1);
	}

	SECTION("find_or_emplace use first", "[containers]")
	{
		vkb::CacheMap<int, int, std::map<int, int>> map;

		int value = 0;

		auto it = map.find_or_emplace(0, [&]() { return ++value; });

		REQUIRE(it->second == 1);

		it = map.find_or_emplace(0, [&]() { return ++value; });

		REQUIRE(it->second == 1);

		it = map.find_or_emplace(1, [&]() { return ++value; });

		REQUIRE(it->second == 2);
	}

	SECTION("replace")
	{
		vkb::CacheMap<int, int> map;

		int value = 0;

		auto it = map.replace(0, value++);

		REQUIRE(it->second == 0);

		it = map.replace(0, value++);

		REQUIRE(it->second == 1);

		it = map.replace(1, value++);

		REQUIRE(it->second == 2);
	}
}