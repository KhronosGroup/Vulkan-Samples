/* Copyright (c) 2020-2023, Arm Limited and Contributors
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

#include "common/tags.h"
#include "platform/parser.h"
#include "platform/platform.h"
#include "plugin.h"

namespace vkb
{
/**
 * @brief PluginBase is the base class that plugins inherit from. The class enforces the use of tags when creating new plugins.
 * 		  For method information see Plugin
 */
template <typename... TAGS>
class PluginBase : public Plugin, public Tag<TAGS...>
{
  public:
	PluginBase(const std::string name, const std::string description, const std::vector<Hook> &hooks = {}, const std::vector<Command *> &commands = {});

	virtual ~PluginBase() = default;

	virtual const std::vector<Command *> &get_cli_commands() const override;
	virtual const std::vector<Hook>      &get_hooks() const override;
	virtual bool                          has_tag(TagID id) const override;

	// hooks that can be implemented by plugins
	virtual void on_update(float delta_time) override{};
	virtual void on_app_start(const std::string &app_id) override{};
	virtual void on_app_close(const std::string &app_id) override{};
	virtual void on_platform_close() override{};
	virtual void on_post_draw(RenderContext &context) override{};
	virtual void on_app_error(const std::string &app_id) override{};

  private:
	Tag<TAGS...> *tags = reinterpret_cast<Tag<TAGS...> *>(this);

	std::vector<Hook>      hooks;
	std::vector<Command *> commands;
};

template <typename... TAGS>
PluginBase<TAGS...>::PluginBase(const std::string name, const std::string description, const std::vector<Hook> &hooks, const std::vector<Command *> &commands) :
    Plugin(name, description), hooks{hooks}, commands{commands}
{
}

template <typename... TAGS>
const std::vector<Command *> &PluginBase<TAGS...>::get_cli_commands() const
{
	return commands;
}

template <typename... TAGS>
bool PluginBase<TAGS...>::has_tag(TagID id) const
{
	return tags->has_tag(id);
}

template <typename... TAGS>
const std::vector<Hook> &PluginBase<TAGS...>::get_hooks() const
{
	return hooks;
}
}        // namespace vkb