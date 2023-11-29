#pragma once

#include <unordered_map>

namespace vkb
{
// Wraps a container to provide a common interface for further specialization
template <typename Key, typename Value, typename Container = std::unordered_map<Key, Value>>
class ContainerWrapper
{
  public:
	using iterator       = typename Container::iterator;
	using const_iterator = typename Container::const_iterator;

	ContainerWrapper()                                    = default;
	ContainerWrapper(const ContainerWrapper &)            = default;
	ContainerWrapper(ContainerWrapper &&)                 = default;
	ContainerWrapper &operator=(const ContainerWrapper &) = default;
	ContainerWrapper &operator=(ContainerWrapper &&)      = default;
	virtual ~ContainerWrapper()                           = default;

	// Wrappers for container functionality
	virtual iterator begin()
	{
		return container.begin();
	}

	// Wrappers for container functionality
	virtual iterator end()
	{
		return container.end();
	}

	// Wrappers for container functionality
	virtual const_iterator begin() const
	{
		return container.begin();
	}

	// Wrappers for container functionality
	virtual const_iterator end() const
	{
		return container.end();
	}

	// Wrappers for container functionality
	virtual iterator find(const Key &key)
	{
		return container.find(key);
	}

	// Wrappers for container functionality
	virtual const_iterator find(const Key &key) const
	{
		return container.find(key);
	}

	// Wrappers for container functionality
	virtual bool contains(const Key &key) const
	{
		return container.find(key) != container.end();
	}

	// Wrappers for container functionality
	virtual void clear()
	{
		container.clear();
	}

	// Wrappers for container functionality
	virtual void erase(const Key &key)
	{
		container.erase(key);
	}

	// Wrappers for container functionality
	virtual void erase(iterator it)
	{
		container.erase(it);
	}

	// Wrappers for container functionality
	virtual void erase(iterator first, iterator last)
	{
		container.erase(first, last);
	}

	// Wrappers for container functionality
	virtual size_t size() const
	{
		return container.size();
	}

	// Wrappers for container functionality
	virtual bool empty() const
	{
		return container.empty();
	}

  protected:
	Container container;
};
}        // namespace vkb