/* Copyright (c) 2020, Arm Limited and Contributors
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

#include <string>
#include <typeindex>
#include <vector>

#include "common/tags.h"

namespace vkb
{
class FlagGroup;
class Parser;
class Platform;

namespace extensions
{
/**
 * @brief Tags are used to define an extensions behaviour. This is useful to dictacte which extensions will work together
 * 	      and which will not without directly specifying and exclusion or inclusion list. Tags are struct types so that they can
 * 		  be used in the tagging system (See extension implementation).
 *		  
 * Entrypoint - An entrypoint is a starting point for the application that will load a vkb::Application see (see start_app)
 * FullControl - The extension wants full control over how the application is executing and therefor Stopping extensions will be ignored (see batch_mode)
 * Stopping - The extension will stop the app through its own mechanism (see stop_after)	
 * Passive - These extensions provide non intrusive behaviour (see fps_logger)
 */
namespace tags
{
struct Entrypoint
{};
struct FullControl
{};
struct Stopping
{};
struct Passive
{};
}        // namespace tags

/**
 * @brief Hooks are points in the project that an extension can subscribe too. These can be expanded on to implement more behaviour in the future
 * 
 * Update - Executed at each update() loop
 * OnAppStart - Executed when an app starts
 * OnAppClose - Executed when an app closes
 * OnPlatformClose - Executed when the platform closes (End off the apps lifecycle)
 */
enum class Hook
{
	OnUpdate,
	OnAppStart,
	OnAppClose,
	OnPlatformClose
};

/**
 * @brief Extensions are used to define custom behaviour. This allows the addition of features without directly
 * 		  interfering with the applications core implementation
 */
class Extension
{
  public:
	Extension(){};
	virtual ~Extension() = default;

	/**
	 * @brief Conducts the process of activating and initializing an extension
	 * 
	 * @param platform The platform
	 * @param parser The parser used to check if the extensions flags are present
	 * @return true If the extension is to be activated
	 * @return false If the extension is not active
	 */
	bool activate_extension(Platform &platform, const Parser &parser);

	virtual const std::vector<FlagGroup> &get_flag_groups() const = 0;

	/**
	 * @brief Return a list of hooks that an extension wants to subscribe too
	 * 
	 * @return Hooks that the extension wants to use
	 */
	virtual const std::vector<Hook> &get_hooks() const = 0;

	/**
	 * @brief Called when an application has been updated
	 * 
	 * @param delta_time The time taken to compute a frame
	 */
	virtual void on_update(float delta_time) = 0;

	/**
	 * @brief Called when an app has started
	 * 
	 * @param app_id The ID of the app
	 */
	virtual void on_app_start(const std::string &app_id) = 0;

	/**
	 * @brief Called when an app has been closed
	 * 
	 * @param app_id The ID of the app
	 */
	virtual void on_app_close(const std::string &app_id) = 0;

	/**
	 * @brief Called when the platform has been requested to close
	 */
	virtual void on_platform_close() = 0;

	/**
	 * @brief Test whether the extension contains a given tag
	 * 
	 * @tparam C the tag to check for
	 * @return true tag present
	 * @return false tag not present
	 */
	template <typename C>
	bool has_tag() const
	{
		return has_tag(Tag<C>::ID);
	}

	/**
	 * @brief Tests whether the extensions contains multiple tags
	 * 
	 * @tparam C A set of tags
	 * @return true Contains all tags
	 * @return false Does not contain all tags
	 */
	template <typename... C>
	bool has_tags() const
	{
		std::vector<TagID> query = {Tag<C>::ID...};
		bool               res   = true;
		for (auto id : query)
		{
			res &= has_tag(id);
		}
		return res;
	}

	/**
	 * @brief Implemented by extension base to return if the extension contains a tag
	 * 
	 * @param id The tag id of a tag
	 * @return true contains tag
	 * @return false does not contain tag
	 */
	virtual bool has_tag(TagID id) const = 0;

  protected:
	/**
	 * @brief An extension will override this method so that it can check if it will be activated
	 * 
	 * @param parser A parser that has parsed the command line arguments when the app starts
	 * @return true If the extension should be activated
	 * @return false If the extesion should be ignored
	 */
	virtual bool is_active(const Parser &parser) = 0;

	/**
	 * @brief Sets up an extension by using values from the parser
	 * 
	 * @param platform The platform
	 * @param parser The parser
	 */
	virtual void init(Platform &platform, const Parser &parser) = 0;

	Platform *platform = nullptr;
};

}        // namespace extensions

/**
 * The following section provides helper functions for filtering containers of extensions
 */
namespace extensions
{
/**
 * @brief Get all extensions with tags
 * 		  Extension must include one or more tags
 * 
 * @tparam TAGS Tags that an extension must contain
 * @param domain The list of extensions to query
 * @return const std::vector<Extension *> A list of extensions containing one or more TAGS
 */
template <typename... TAGS>
const std::vector<Extension *> with_tags(const std::vector<Extension *> &domain = {})
{
	std::vector<TagID>       tags = {Tag<TAGS>::ID...};
	std::vector<Extension *> compatable;
	for (auto ext : domain)
	{
		bool has_one = false;
		for (auto t : tags)
		{
			has_one |= ext->has_tag(t);
		}

		if (has_one)
		{
			compatable.push_back(ext);
		}
	}
	return compatable;
}

/**
 * @brief Get all extensions without the given tags
 * 		  Extension must not include one or more tags
 * 		  Essentially the opoposite of extensions::with_tags<...TAGS>()
 * 
 * @tparam TAGS Tags that an extension must not contain
 * @param domain The list of extensions to query
 * @return const std::vector<Extension *> A list of extensions containing one or more TAGS
 */
template <typename... TAGS>
const std::vector<Extension *> without_tags(const std::vector<Extension *> &domain = {})
{
	std::vector<TagID>       tags = {Tag<TAGS>::ID...};
	std::vector<Extension *> compatable;
	for (auto ext : domain)
	{
		bool has_any = false;
		for (auto t : tags)
		{
			has_any |= ext->has_tag(t);
		}

		if (!has_any)
		{
			compatable.push_back(ext);
		}
	}
	return compatable;
}
}        // namespace extensions
}        // namespace vkb