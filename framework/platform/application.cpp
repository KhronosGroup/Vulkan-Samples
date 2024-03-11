/* Copyright (c) 2019-2024, Arm Limited and Contributors
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

#include "application.h"

#include "core/util/logging.hpp"
#include "platform/window.h"

namespace vkb
{
Application::Application() :
    name{"Sample Name"}
{
}

bool Application::prepare(const ApplicationOptions &options)
{
	assert(options.window != nullptr && "Window must be valid");

	auto &_debug_info = get_debug_info();
	_debug_info.insert<field::MinMax, float>("fps", fps);
	_debug_info.insert<field::MinMax, float>("frame_time", frame_time);

	lock_simulation_speed = options.benchmark_enabled;
	window                = options.window;

	return true;
}

void Application::finish()
{
}

bool Application::resize(const uint32_t /*width*/, const uint32_t /*height*/)
{
	return true;
}

void Application::input_event(const InputEvent &input_event)
{
}

Drawer *Application::get_drawer()
{
	return nullptr;
}

void Application::update(float delta_time)
{
	fps        = 1.0f / delta_time;
	frame_time = delta_time * 1000.0f;
}

void Application::update_overlay(float delta_time, const std::function<void()> &additional_ui)
{
}

const std::string &Application::get_name() const
{
	return name;
}

void Application::set_name(const std::string &name_)
{
	name = name_;
}

DebugInfo &Application::get_debug_info()
{
	return debug_info;
}

void Application::change_shader(const vkb::ShaderSourceLanguage &shader_language)
{
	LOGE("Not implemented by sample");
}

const std::map<ShaderSourceLanguage, std::vector<std::pair<VkShaderStageFlagBits, std::string>>> &Application::get_available_shaders() const
{
	return available_shaders;
}

void Application::store_shaders(const vkb::ShaderSourceLanguage &shader_language, const std::vector<std::pair<VkShaderStageFlagBits, std::string>> &list_of_shaders)
{
	available_shaders.insert({shader_language, list_of_shaders});
}

}        // namespace vkb
