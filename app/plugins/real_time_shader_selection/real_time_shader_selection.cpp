/* Copyright (c) 2024, Mobica Limited
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
                                {vkb::Hook::OnAppStart, vkb::Hook::OnUpdateUi},
                                {&realtimeshaderselection_flag}),
    active_shader(0),
    min_size_for_shaders(2)
{
}

bool RealTimeShaderSelection::is_active(const vkb::CommandParser &parser)
{
	return parser.contains(&realtimeshaderselection_flag);
}

void RealTimeShaderSelection::init(const vkb::CommandParser &parser)
{
}

void RealTimeShaderSelection::on_app_start(const std::string &app_info)
{
	if (platform->get_app().get_available_shaders().size() < min_size_for_shaders)
	{
		LOGE("Sample doesn't support RealTimeShaderSelection plugin, sample should add available shaders please see Application::store_shaders.");
		LOGE("Sample, defined {} shaders, minimum number of defined shaders is {}", platform->get_app().get_available_shaders().size(), min_size_for_shaders);
		return;
	}

	for (auto const &shader : platform->get_app().get_available_shaders())
	{
		switch (shader.first)
		{
			case vkb::ShaderSourceLanguage::GLSL:
				language_names.emplace_back("GLSL");
				break;
			case vkb::ShaderSourceLanguage::HLSL:
				language_names.emplace_back("HLSL");
				break;
			case vkb::ShaderSourceLanguage::SPV:
				language_names.emplace_back("SPV");
				break;
			default:
				LOGE("Not supported shader language");
				assert(false);
		}
	}
}

void RealTimeShaderSelection::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (platform->get_app().get_available_shaders().size() >= min_size_for_shaders)
	{
		if (drawer.header("Real Time Shader Selection"))
		{
			if (drawer.combo_box("Shader language", &active_shader, language_names))
			{
				std::string               selectedShader = language_names[active_shader];
				vkb::ShaderSourceLanguage shaderType     = vkb::ShaderSourceLanguage::GLSL;
				if (selectedShader == "GLSL")
				{
					shaderType = vkb::ShaderSourceLanguage::GLSL;
				}
				else if (selectedShader == "HLSL")
				{
					shaderType = vkb::ShaderSourceLanguage::HLSL;
				}
				else if (selectedShader == "SPV")
				{
					shaderType = vkb::ShaderSourceLanguage::SPV;
				}
				else
				{
					LOGE("Not supported shader language");
					assert(false);
				}
				auto it = platform->get_app().get_available_shaders().find(shaderType);
				platform->get_app().change_shader(it->first);
			}
		}
	}
}

}        // namespace plugins