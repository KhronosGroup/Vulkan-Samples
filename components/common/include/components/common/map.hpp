#pragma once

#include <unordered_map>

namespace components
{
template <typename Key, typename Type, class Container = std::unordered_map<Key, Type>>
class Map : public Container
{
  public:
	Map()          = default;
	virtual ~Map() = default;

	auto find_or_create(const Key &key, std::function<Type()> create_fn)
	{
		if (!create_fn)
		{
			throw std::runtime_error{"create_fn must not be null"};
		}

		auto it = this->find(key);
		if (it == this->end())
		{
			it = this->emplace(key, create_fn()).first;
		}
		return it;
	}
};
}        // namespace components