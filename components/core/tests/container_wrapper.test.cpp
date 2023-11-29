#include <catch2/catch_test_macros.hpp>

#include <map>
#include <unordered_map>

#include "core/containers/container_wrapper.hpp"

namespace vkb
{
template <typename Key, typename Value, typename Container = std::unordered_map<Key, Value>>
class TestContainer : public ContainerWrapper<Key, Value, Container>
{
  public:
	TestContainer()          = default;
	virtual ~TestContainer() = default;

	using iterator       = typename ContainerWrapper<Key, Value, Container>::iterator;
	using const_iterator = typename ContainerWrapper<Key, Value, Container>::const_iterator;

	iterator add(const Key &key, const Value &value)
	{
		return container.emplace(key, value).first;
	}

  private:
	using ContainerWrapper<Key, Value, Container>::container;
};
}        // namespace vkb

using namespace vkb;

TEST_CASE("ContainerWrapper", "[containers]")
{
	SECTION("find")
	{
		TestContainer<int, int> map;

		int value = 0;

		map.add(0, value++);

		auto it = map.find(0);

		REQUIRE(it->second == 0);

		it = map.find(1);

		REQUIRE(it == map.end());
	}

	SECTION("begin")
	{
		TestContainer<int, int> map;

		int value = 0;

		map.add(0, value++);

		auto it = map.begin();

		REQUIRE(it->second == 0);
	}

	SECTION("end")
	{
		TestContainer<int, int> map;

		int value = 0;

		map.add(0, value++);

		auto it = map.end();

		REQUIRE(it == map.find(1));
	}

	SECTION("contains")
	{
		TestContainer<int, int> map;

		int value = 0;

		map.add(0, value++);

		REQUIRE(map.contains(0));
		REQUIRE(!map.contains(1));
	}

	SECTION("erase")
	{
		TestContainer<int, int> map;

		int value = 0;

		map.add(0, value++);

		map.erase(0);

		REQUIRE(map.find(0) == map.end());
	}

	SECTION("clear")
	{
		TestContainer<int, int> map;

		int value = 0;

		map.add(0, value++);

		map.clear();

		REQUIRE(map.find(0) == map.end());
	}

	SECTION("size")
	{
		TestContainer<int, int> map;

		int value = 0;

		map.add(0, value++);

		REQUIRE(map.size() == 1);
	}

	SECTION("empty")
	{
		TestContainer<int, int> map;

		REQUIRE(map.empty());

		int value = 0;

		map.add(0, value++);

		REQUIRE(!map.empty());
	}

	SECTION("iterator")
	{
		TestContainer<int, int> map;

		int value = 0;

		map.add(0, value++);

		auto it = map.begin();

		REQUIRE(it->second == 0);

		++it;

		REQUIRE(it == map.end());
	}

	SECTION("range-based for loop")
	{
		// Cache needs to be ordered for check to pass
		TestContainer<int, int, std::map<int, int>> map;

		int value = 0;

		map.add(0, value++);
		map.add(1, value++);

		REQUIRE(map.size() == 2);

		int i = 0;

		for (auto pair : map)
		{
			REQUIRE(pair.second == i++);
		}
	}

	SECTION("const range-based for loop")
	{
		// Cache needs to be ordered for check to pass
		TestContainer<int, int, std::map<int, int>> map;

		int value = 0;

		map.add(0, value++);
		map.add(1, value++);

		REQUIRE(map.size() == 2);

		int i = 0;

		for (const auto pair : map)
		{
			REQUIRE(pair.second == i++);
		}
	}
}