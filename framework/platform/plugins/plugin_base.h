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

#include <spdlog/fmt/fmt.h>

#include "common/tags.h"
#include "plugin.h"
#include "flag.h"
#include "parser.h"
#include "platform/platform.h"

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
	PluginBase(const std::vector<Hook> &hooks = {}, const std::vector<FlagGroup> &groups = {});

	virtual ~PluginBase() = default;

	virtual const std::vector<FlagGroup> &get_flag_groups() const override;
	virtual const std::vector<Hook> &     get_hooks() const override;
	virtual bool                          has_tag(TagID id) const override;

	// hooks that can be implemented by plugins
	virtual void on_update(float delta_time) override{};
	virtual void on_app_start(const std::string &app_id) override{};
	virtual void on_app_close(const std::string &app_id) override{};
	virtual void on_platform_close() override{};

  private:
	Tag<TAGS...> *tags = reinterpret_cast<Tag<TAGS...> *>(this);

	std::vector<Hook>      hooks;
	std::vector<FlagGroup> groups;
};

template <typename... TAGS>
PluginBase<TAGS...>::PluginBase(const std::vector<Hook> &hooks, const std::vector<FlagGroup> &groups) :
    hooks{hooks}, groups{groups}
{
}

template <typename... TAGS>
const std::vector<FlagGroup> &PluginBase<TAGS...>::get_flag_groups() const
{
	return groups;
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