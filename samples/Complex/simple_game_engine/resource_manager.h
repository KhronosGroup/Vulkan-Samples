/* Copyright (c) 2025 Holochip Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeindex>
#include <unordered_map>

/**
 * @brief Base class for all resources.
 */
class Resource final
{
  protected:
	std::string resourceId;
	bool        loaded = false;

  public:
	/**
	 * @brief Constructor with a resource ID.
	 * @param id The unique identifier for the resource.
	 */
	explicit Resource(const std::string &id) :
	    resourceId(id)
	{}

	/**
	 * @brief Virtual destructor for proper cleanup.
	 */
	virtual ~Resource() = default;

	/**
	 * @brief Get the resource ID.
	 * @return The resource ID.
	 */
	const std::string &GetId() const
	{
		return resourceId;
	}

	/**
	 * @brief Check if the resource is loaded.
	 * @return True if the resource is loaded, false otherwise.
	 */
	bool IsLoaded() const
	{
		return loaded;
	}

	/**
	 * @brief Load the resource.
	 * @return True if the resource was loaded successfully, false otherwise.
	 */
	virtual bool Load();

	/**
	 * @brief Unload the resource.
	 */
	virtual void Unload();
};

/**
 * @brief Template class for resource handles.
 * @tparam T The type of resource.
 */
template <typename T>
class ResourceHandle
{
  private:
	std::string            resourceId;
	class ResourceManager *resourceManager = nullptr;

  public:
	/**
	 * @brief Default constructor.
	 */
	ResourceHandle() = default;

	/**
	 * @brief Constructor with a resource ID and resource manager.
	 * @param id The resource ID.
	 * @param manager The resource manager.
	 */
	ResourceHandle(const std::string &id, class ResourceManager *manager) :
	    resourceId(id), resourceManager(manager)
	{}

	/**
	 * @brief Get the resource.
	 * @return A pointer to the resource, or nullptr if not found.
	 */
	T *Get() const;

	/**
	 * @brief Check if the handle is valid.
	 * @return True if the handle is valid, false otherwise.
	 */
	bool IsValid() const;

	/**
	 * @brief Get the resource ID.
	 * @return The resource ID.
	 */
	const std::string &GetId() const
	{
		return resourceId;
	}

	/**
	 * @brief Convenience operator for accessing the resource.
	 * @return A pointer to the resource.
	 */
	T *operator->() const
	{
		return Get();
	}

	/**
	 * @brief Convenience operator for dereferencing the resource.
	 * @return A reference to the resource.
	 */
	T &operator*() const
	{
		return *Get();
	}

	/**
	 * @brief Convenience operator for checking if the handle is valid.
	 * @return True if the handle is valid, false otherwise.
	 */
	operator bool() const
	{
		return IsValid();
	}
};

/**
 * @brief Class for managing resources.
 *
 * This class implements the resource management system as described in the Engine_Architecture chapter:
 * @see en/Building_a_Simple_Engine/Engine_Architecture/04_resource_management.adoc
 */
class ResourceManager final
{
  private:
	std::unordered_map<std::type_index, std::unordered_map<std::string, std::unique_ptr<Resource>>> resources;

  public:
	/**
	 * @brief Default constructor.
	 */
	ResourceManager() = default;

	/**
	 * @brief Virtual destructor for proper cleanup.
	 */
	virtual ~ResourceManager() = default;

	/**
	 * @brief Load a resource.
	 * @tparam T The type of resource.
	 * @tparam Args The types of arguments to pass to the resource constructor.
	 * @param id The resource ID.
	 * @param args The arguments to pass to the resource constructor.
	 * @return A handle to the resource.
	 */
	template <typename T, typename... Args>
	ResourceHandle<T> LoadResource(const std::string &id, Args &&...args)
	{
		static_assert(std::is_base_of<Resource, T>::value, "T must derive from Resource");

		// Check if the resource already exists
		auto &typeResources = resources[std::type_index(typeid(T))];
		auto  it            = typeResources.find(id);
		if (it != typeResources.end())
		{
			return ResourceHandle<T>(id, this);
		}

		// Create and load the resource
		auto resource = std::make_unique<T>(id, std::forward<Args>(args)...);
		if (!resource->Load())
		{
			throw std::runtime_error("Failed to load resource: " + id);
		}

		// Store the resource
		typeResources[id] = std::move(resource);
		return ResourceHandle<T>(id, this);
	}

	/**
	 * @brief Get a resource.
	 * @tparam T The type of resource.
	 * @param id The resource ID.
	 * @return A pointer to the resource, or nullptr if not found.
	 */
	template <typename T>
	T *GetResource(const std::string &id)
	{
		static_assert(std::is_base_of<Resource, T>::value, "T must derive from Resource");

		auto typeIt = resources.find(std::type_index(typeid(T)));
		if (typeIt == resources.end())
		{
			return nullptr;
		}

		auto &typeResources = typeIt->second;
		auto  resourceIt    = typeResources.find(id);
		if (resourceIt == typeResources.end())
		{
			return nullptr;
		}

		return static_cast<T *>(resourceIt->second.get());
	}

	/**
	 * @brief Check if a resource exists.
	 * @tparam T The type of resource.
	 * @param id The resource ID.
	 * @return True if the resource exists, false otherwise.
	 */
	template <typename T>
	bool HasResource(const std::string &id)
	{
		static_assert(std::is_base_of<Resource, T>::value, "T must derive from Resource");

		auto typeIt = resources.find(std::type_index(typeid(T)));
		if (typeIt == resources.end())
		{
			return false;
		}

		auto &typeResources = typeIt->second;
		return typeResources.contains(id);
	}

	/**
	 * @brief Unload a resource.
	 * @tparam T The type of resource.
	 * @param id The resource ID.
	 * @return True if the resource was unloaded, false otherwise.
	 */
	template <typename T>
	bool UnloadResource(const std::string &id)
	{
		static_assert(std::is_base_of<Resource, T>::value, "T must derive from Resource");

		auto typeIt = resources.find(std::type_index(typeid(T)));
		if (typeIt == resources.end())
		{
			return false;
		}

		auto &typeResources = typeIt->second;
		auto  resourceIt    = typeResources.find(id);
		if (resourceIt == typeResources.end())
		{
			return false;
		}

		resourceIt->second->Unload();
		typeResources.erase(resourceIt);
		return true;
	}

	/**
	 * @brief Unload all resources.
	 */
	void UnloadAllResources();
};

// Implementation of ResourceHandle methods
template <typename T>
T *ResourceHandle<T>::Get() const
{
	if (!resourceManager)
		return nullptr;
	return resourceManager->GetResource<T>(resourceId);
}

template <typename T>
bool ResourceHandle<T>::IsValid() const
{
	if (!resourceManager)
		return false;
	return resourceManager->HasResource<T>(resourceId);
}
