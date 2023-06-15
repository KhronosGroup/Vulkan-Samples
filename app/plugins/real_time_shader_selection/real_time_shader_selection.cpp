/* Copyright (c) 2023, Mobica Limited
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

#include "real_time_shader_selection.h"

#include "platform/platform.h"

namespace plugins
{
RealTimeShaderSelection::RealTimeShaderSelection() :
    RealTimeShaderSelectionTags("Real Time Shader Selection",
                                "Enable dynamic shader selection for samples.",
                                {vkb::Hook::OnUpdate, vkb::Hook::OnAppStart, vkb::Hook::OnUpdateUi},
                                {&realtimeshaderselection_flag})
{
	LOGI("Starting RealTimeShaderSelection");
}

bool RealTimeShaderSelection::is_active(const vkb::CommandParser &parser)
{
	bool a = parser.contains(&realtimeshaderselection_flag);
	LOGI("is_active RealTimeShaderSelection = {}", a);
	return a;
}

void RealTimeShaderSelection::init(const vkb::CommandParser &parser)
{
	LOGI("init RealTimeShaderSelection");
}

void RealTimeShaderSelection::on_update(float delta_time)
{
	LOGI("on_update RealTimeShaderSelection");
}

void RealTimeShaderSelection::on_app_start(const std::string &app_info)
{
	LOGI("on_app_start RealTimeShaderSelection");
}

void RealTimeShaderSelection::on_update_ui_overlay(vkb::Drawer &drawer)
{
	LOGI("on_update_ui_overlay RealTimeShaderSelection");
	if (drawer.header("TEST DAWID LORENZ"))
	{
	}
}

}        // namespace plugins