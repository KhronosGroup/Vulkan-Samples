#pragma once

#include <functional>
#include <typeindex>
#include <unordered_map>

#include "core/containers/cache_map.hpp"

namespace vkb
{
template <typename Value, typename Container = std::unordered_map<std::type_index, Value>>
class TypeMap : public CacheMap<std::type_index, Value, Container>
{
  public:
	using iterator       = typename Container::iterator;
	using const_iterator = typename Container::const_iterator;

	TypeMap()          = default;
	virtual ~TypeMap() = default;

	template <typename T>
	iterator find_or_emplace(std::function<Value()> create)
	{
		return CacheMap<std::type_index, Value, Container>::find_or_emplace(typeid(T), create);
	}

	template <typename T>
	iterator replace(const Value &value)
	{
		return CacheMap<std::type_index, Value, Container>::replace(typeid(T), value);
	}

	template <typename T>
	iterator find()
	{
		return CacheMap<std::type_index, Value, Container>::find(typeid(T));
	}

	template <typename T>
	const_iterator find() const
	{
		return CacheMap<std::type_index, Value, Container>::find(typeid(T));
	}

	template <typename T>
	bool contains() const
	{
		return CacheMap<std::type_index, Value, Container>::contains(typeid(T));
	}

  protected:
	using CacheMap<std::type_index, Value, Container>::container;
};

};        // namespace vkb