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
                                {vkb::Hook::OnAppStart, vkb::Hook::OnUpdateUi},
                                {&realtimeshaderselection_flag}),
	activeShader(0)
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
	if (platform->get_available_shaders().size() < min_size_for_shaders)
	{
		LOGE("Sample doesn't support RealTimeShaderSelection plugin, sample should add available shaders please see ApiVulkanSample::store_shader.");
		LOGE("Sample, defined {} shaders, minimum number of defined shaders is {}", platform->get_available_shaders().size(), min_size_for_shaders);
		return;
	}
	availableShaders = platform->get_available_shaders();
	for (auto const& shader : availableShaders)
	{
		switch(shader.first)
		{
			case vkb::ShaderSourceLanguage::VK_GLSL:
				shaderName.emplace_back("GLSL");
			break;
			case vkb::ShaderSourceLanguage::VK_HLSL:
				shaderName.emplace_back("HLSL");
			break;
			case vkb::ShaderSourceLanguage::VK_SPV:
				shaderName.emplace_back("SPV");
			break;
		}
	}
}

void RealTimeShaderSelection::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (availableShaders.size() >= min_size_for_shaders)
	{
		if (drawer.header("Real Time Shader Selection"))
		{
			if (drawer.combo_box("Shader language", &activeShader, shaderName))
			{
				std::string selectedShader = shaderName[activeShader];
				vkb::ShaderSourceLanguage shaderType = vkb::ShaderSourceLanguage::VK_GLSL;
				if (selectedShader == "GLSL")
				{
					shaderType = vkb::ShaderSourceLanguage::VK_GLSL;
				}
				else if (selectedShader == "HLSL")
				{
					shaderType = vkb::ShaderSourceLanguage::VK_HLSL;
				}
				else if (selectedShader == "SPV")
				{
					shaderType = vkb::ShaderSourceLanguage::VK_SPV;
				}
				else
				{
					return;
				}
				auto it = availableShaders.find(shaderType);
				auto app = &platform->get_app();
				app->change_shader(it->first, it->second);
			}
		}
	}
}

}        // namespace plugins