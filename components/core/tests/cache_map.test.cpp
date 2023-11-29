#include <catch2/catch_test_macros.hpp>

#include <map>

#include "core/containers/cache_map.hpp"

TEST_CASE("CacheMap", "[containers]")
{
	SECTION("find_or_insert")
	{
		vkb::CacheMap<int, int> map;

		int value = 0;

		auto it = map.find_or_insert(0, [&]() { return value++; });

		REQUIRE(it->second == 0);

		it = map.find_or_insert(0, [&]() { return value++; });

		REQUIRE(it->second == 0);

		it = map.find_or_insert(1, [&]() { return value++; });

		REQUIRE(it->second == 1);
	}

	SECTION("find_or_insert use first", "[containers]")
	{
		vkb::CacheMap<int, int, std::map<int, int>> map;

		int value = 0;

		auto it = map.find_or_insert(0, [&]() { return ++value; });

		REQUIRE(it->second == 1);

		it = map.find_or_insert(0, [&]() { return ++value; });

		REQUIRE(it->second == 1);

		it = map.find_or_insert(1, [&]() { return ++value; });

		REQUIRE(it->second == 2);
	}

	SECTION("replace_emplace")
	{
		vkb::CacheMap<int, int> map;

		auto it = map.replace_emplace(0, 0);
		REQUIRE(it->second == 0);

		it = map.replace_emplace(0, 1);
		REQUIRE(it->second == 1);

		it = map.replace_emplace(1, 2);
		REQUIRE(it->second == 2);
	}

	class OnlyMoveable
	{
	  public:
		OnlyMoveable(int value) :
		    value{value}
		{}
		OnlyMoveable(OnlyMoveable &&)                 = default;
		OnlyMoveable &operator=(OnlyMoveable &&)      = default;
		OnlyMoveable(const OnlyMoveable &)            = delete;
		OnlyMoveable &operator=(const OnlyMoveable &) = delete;

		int value;
	};

	SECTION("replace_emplace move only")
	{
		vkb::CacheMap<int, OnlyMoveable> map;

		OnlyMoveable om{1};
		auto         it = map.replace_emplace(0, std::move(om));
		REQUIRE(it->second.value == 1);

		OnlyMoveable om2{2};
		it = map.replace_emplace(0, std::move(om2));
		REQUIRE(it->second.value == 2);

		it = map.replace_emplace(1, OnlyMoveable{3});
		REQUIRE(it->second.value == 3);
	}

	// Common for cached resources
	class MoveWithoutAssignment
	{
	  public:
		MoveWithoutAssignment(int value) :
		    value{value}
		{}
		MoveWithoutAssignment(MoveWithoutAssignment &&)                 = default;
		MoveWithoutAssignment &operator=(MoveWithoutAssignment &&)      = delete;
		MoveWithoutAssignment(const MoveWithoutAssignment &)            = delete;
		MoveWithoutAssignment &operator=(const MoveWithoutAssignment &) = delete;

		int value;
	};

	SECTION("replace_emplace move only without assignment")
	{
		vkb::CacheMap<int, MoveWithoutAssignment> map;

		MoveWithoutAssignment mwa{1};
		auto                  it = map.replace_emplace(0, std::move(mwa));
		REQUIRE(it->second.value == 1);

		MoveWithoutAssignment mwa2{2};
		it = map.replace_emplace(0, std::move(mwa2));
		REQUIRE(it->second.value == 2);

		it = map.replace_emplace(1, MoveWithoutAssignment{3});
		REQUIRE(it->second.value == 3);
	}
}