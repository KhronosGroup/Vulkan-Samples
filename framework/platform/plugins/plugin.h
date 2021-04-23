/* Copyright (c) 2020-2021, Arm Limited and Contributors
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

#include <cassert>
#include <string>
#include <typeindex>
#include <vector>

#include "common/tags.h"
#include "platform/parser.h"

namespace vkb
{
class Platform;
class RenderContext;
class Plugin;

/**
 * @brief Tags are used to define a plugins behaviour. This is useful to dictate which plugins will work together
 * 	      and which will not without directly specifying an exclusion or inclusion list. Tags are struct types so that they can
 * 		  be used in the tagging system (See plugin implementation).
 *		  
 * Entrypoint - An entrypoint is a starting point for the application that will load a vkb::Application (see start_app)
 * FullControl - The plugin wants full control over how the application executes. Stopping plugins will be ignored (see batch_mode)
 * Stopping - The plugin will stop the app through its own mechanism (see stop_after)	
 * Passive - These plugins provide non intrusive behaviour (see fps_logger)
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
 * @brief Associate how plugins can interact with each other. This interoperability is decided by comparing tags of different plugins. The plugins inclusion and exclusion lists are populated by this function
 * 
 * @param plugins A list of plugins which are used together
 * @return std::vector<Plugin *> A list of plugins which are used together
 */
std::vector<Plugin *> associate_plugins(const std::vector<Plugin *> &plugins);

/**
 * @brief Hooks are points in the project that an plugin can subscribe too. These can be expanded on to implement more behaviour in the future
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
	OnAppError,
	OnPlatformClose,
	PostDraw
};

/**
 * @brief Plugins are used to define custom behaviour. This allows the addition of features without directly
 * 		  interfering with the applications core implementation
 */
class Plugin
{
  public:
	Plugin(const std::string name, const std::string description) :
	    name{name}, description{description} {};
	virtual ~Plugin() = default;

	/**
	 * @brief Conducts the process of activating and initializing an plugin
	 * 
	 * @param platform The platform
	 * @param parseer The parser used to check if the plugins flags are present
	 * @param force_activation Force a plugin to be activated, not advised unless the plugin works without inputs
	 * @return true If the plugin is to be activated
	 * @return false If the plugin is not active
	 */
	bool activate_plugin(Platform *platform, const CommandParser &parser, bool force_activation = false);

	virtual const std::vector<Command *> &get_cli_commands() const = 0;

	/**
	 * @brief Return a list of hooks that an plugin wants to subscribe to
	 * 
	 * @return Hooks that the plugin wants to use
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
	 * @brief Handle when an application errors
	 * 
	 * @param app_id The ID of the app which errored
	 */
	virtual void on_app_error(const std::string &app_id) = 0;

	/**
	 * @brief Called when the platform has been requested to close
	 */
	virtual void on_platform_close() = 0;

	/**
	 * @brief Post Draw
	 */
	virtual void on_post_draw(RenderContext &context) = 0;

	const std::string &          get_name() const;
	const std::string &          get_description() const;
	void                         excludes(Plugin *plugin);
	const std::vector<Plugin *> &get_exclusions() const;
	void                         includes(Plugin *plugin);
	const std::vector<Plugin *> &get_inclusions() const;

	/**
	 * @brief Test whether the plugin contains a given tag
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
	 * @brief Tests whether the plugins contains multiple tags
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
	 * @brief Implemented by plugin base to return if the plugin contains a tag
	 * 
	 * @param id The tag id of a tag
	 * @return true contains tag
	 * @return false does not contain tag
	 */
	virtual bool has_tag(TagID id) const = 0;

  protected:
	/**
	 * @brief An plugin will override this method so that it can check if it will be activated
	 * 
	 * @param parser A parser that has parsed the command line arguments when the app starts
	 * @return true If the plugin should be activated
	 * @return false If the plugin should be ignored
	 */
	virtual bool is_active(const CommandParser &parser) = 0;

	/**
	 * @brief Sets up an plugin by using values from the parser
	 * 
	 * @param platform The platform
	 * @param parser The parser
	 */
	virtual void init(const CommandParser &parser) = 0;

	Platform *platform = nullptr;

  private:
	std::string name;
	std::string description;

	std::vector<Plugin *> exclusions;
	std::vector<Plugin *> inclusions;
};

/**
 * The following section provides helper functions for filtering containers of plugins
 */
namespace plugins
{
/**
 * @brief Get all plugins with tags
 * 		  Plugin must include one or more tags
 * 
 * @tparam TAGS Tags that an plugin must contain
 * @param domain The list of plugins to query
 * @return const std::vector<Plugin *> A list of plugins containing one or more TAGS
 */
template <typename... TAGS>
const std::vector<Plugin *> with_tags(const std::vector<Plugin *> &domain = {})
{
	std::vector<TagID>    tags = {Tag<TAGS>::ID...};
	std::vector<Plugin *> compatable;
	for (auto ext : domain)
	{
		assert(ext != nullptr);

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
 * @brief Get all plugins without the given tags
 * 		  Plugin must not include one or more tags
 * 		  Essentially the opoposite of plugins::with_tags<...TAGS>()
 * 
 * @tparam TAGS Tags that an plugin must not contain
 * @param domain The list of plugins to query
 * @return const std::vector<Plugin *> A list of plugins containing one or more TAGS
 */
template <typename... TAGS>
const std::vector<Plugin *> without_tags(const std::vector<Plugin *> &domain = {})
{
	std::vector<TagID>    tags = {Tag<TAGS>::ID...};
	std::vector<Plugin *> compatable;
	for (auto ext : domain)
	{
		assert(ext != nullptr);

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
}        // namespace plugins
}        // namespace vkb