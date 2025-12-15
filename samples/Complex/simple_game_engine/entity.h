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

#include <algorithm>
#include <chrono>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "component.h"

/**
 * @brief Entity class that can have multiple components attached to it.
 *
 * Entities are containers for components. They don't have any behavior
 * on their own, but gain functionality through the components attached to them.
 */
class Entity
{
  private:
	std::string                             name;
	bool                                    active = true;
	std::vector<std::unique_ptr<Component>> components;

  public:
	/**
	 * @brief Constructor with a name.
	 * @param entityName The name of the entity.
	 */
	explicit Entity(const std::string &entityName) :
	    name(entityName)
	{}

	/**
	 * @brief Virtual destructor for proper cleanup.
	 */
	virtual ~Entity() = default;

	/**
	 * @brief Get the name of the entity.
	 * @return The name of the entity.
	 */
	const std::string &GetName() const
	{
		return name;
	}

	/**
	 * @brief Check if the entity is active.
	 * @return True if the entity is active, false otherwise.
	 */
	bool IsActive() const
	{
		return active;
	}

	/**
	 * @brief Set the active state of the entity.
	 * @param isActive The new active state.
	 */
	void SetActive(bool isActive)
	{
		active = isActive;
	}

	/**
	 * @brief Initialize all components of the entity.
	 */
	void Initialize();

	/**
	 * @brief Update all components of the entity.
	 * @param deltaTime The time elapsed since the last frame.
	 */
	void Update(std::chrono::milliseconds deltaTime);

	/**
	 * @brief Render all components of the entity.
	 */
	void Render();

	/**
	 * @brief Add a component to the entity.
	 * @tparam T The type of component to add.
	 * @tparam Args The types of arguments to pass to the component constructor.
	 * @param args The arguments to pass to the component constructor.
	 * @return A pointer to the newly created component.
	 */
	template <typename T, typename... Args>
	T *AddComponent(Args &&...args)
	{
		static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");

		// Create the component
		auto component    = std::make_unique<T>(std::forward<Args>(args)...);
		T   *componentPtr = component.get();

		// Set the owner
		componentPtr->SetOwner(this);

		// Add to the vector for ownership and iteration
		components.push_back(std::move(component));

		// Initialize the component
		componentPtr->Initialize();

		return componentPtr;
	}

	/**
	 * @brief Get a component of a specific type.
	 * @tparam T The type of component to get.
	 * @return A pointer to the component, or nullptr if not found.
	 */
	template <typename T>
	T *GetComponent() const
	{
		static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");

		// Search from the back to preserve previous behavior of returning the last-added component of type T
		for (auto it = components.rbegin(); it != components.rend(); ++it)
		{
			if (auto *casted = dynamic_cast<T *>(it->get()))
			{
				return casted;
			}
		}
		return nullptr;
	}

	/**
	 * @brief Remove a component of a specific type.
	 * @tparam T The type of component to remove.
	 * @return True if the component was removed, false otherwise.
	 */
	template <typename T>
	bool RemoveComponent()
	{
		static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");

		for (auto it = components.rbegin(); it != components.rend(); ++it)
		{
			if (dynamic_cast<T *>(it->get()) != nullptr)
			{
				components.erase(std::next(it).base());
				return true;
			}
		}

		return false;
	}

	/**
	 * @brief Check if the entity has a component of a specific type.
	 * @tparam T The type of component to check for.
	 * @return True if the entity has the component, false otherwise.
	 */
	template <typename T>
	bool HasComponent() const
	{
		static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
		return GetComponent<T>() != nullptr;
	}
};
