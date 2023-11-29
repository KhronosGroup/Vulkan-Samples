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

	using Wrapper = CacheMap<std::type_index, Value, Container>;

	// Finds the value at key, or inserts a new value if it doesn't exist.
	template <typename T>
	iterator find_or_insert(std::function<Value()> &&create)
	{
		return Wrapper::find_or_insert(typeid(T), std::move(create));
	}

	// Replaces the value at key with the given value.
	template <typename T>
	iterator replace_emplace(Value &&value)
	{
		return Wrapper::replace_emplace(typeid(T), std::move(value));
	}

	template <typename T>
	iterator find()
	{
		return Wrapper::find(typeid(T));
	}

	template <typename T>
	const_iterator find() const
	{
		return Wrapper::find(typeid(T));
	}

	template <typename T>
	bool contains() const
	{
		return Wrapper::contains(typeid(T));
	}

	using Wrapper::begin;
	using Wrapper::clear;
	using Wrapper::empty;
	using Wrapper::end;

  protected:
	using Wrapper::container;
};

};        // namespace vkb