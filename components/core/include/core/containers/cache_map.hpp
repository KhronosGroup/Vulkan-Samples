#pragma once

#include <functional>
#include <unordered_map>

#include "core/containers/container_wrapper.hpp"

namespace vkb
{
// Stores a map of keys to values, where the values are created on demand using a factory function.
template <typename Key, typename Value, typename Container = std::unordered_map<Key, Value>>
class CacheMap : public ContainerWrapper<Key, Value, Container>
{
  public:
	using iterator       = typename Container::iterator;
	using const_iterator = typename Container::const_iterator;

	CacheMap()                            = default;
	CacheMap(const CacheMap &)            = delete;
	CacheMap(CacheMap &&)                 = default;
	CacheMap &operator=(const CacheMap &) = delete;
	CacheMap &operator=(CacheMap &&)      = default;

	virtual iterator find_or_emplace(const Key &key, std::function<Value()> create)
	{
		auto it = container.find(key);
		if (it == container.end())
		{
			it = container.emplace(key, create()).first;
		}
		return it;
	}

	virtual iterator replace(const Key &key, const Value &value)
	{
		return container.emplace(key, std::forward(value)).first;
	}

  protected:
	using ContainerWrapper<Key, Value, Container>::container;
};
}        // namespace vkb