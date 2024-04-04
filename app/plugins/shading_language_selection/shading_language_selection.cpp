/* Copyright (c) 2024, Sascha Willems
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

#include "shading_language_selection.h"

#include <algorithm>

#include "platform/application.h"

namespace plugins
{
ShadingLanguageSelection::ShadingLanguageSelection() :
    ShadingLanguageSelectionTags("Shading language selection",
                                 "A collection of flags to select shader from different shading languages (glsl, hlsl)",
                                 {}, {&shading_language_selection_options_group})
{
}

bool ShadingLanguageSelection::is_active(const vkb::CommandParser &parser)
{
	return true;
}

void ShadingLanguageSelection::init(const vkb::CommandParser &parser)
{
	if (parser.contains(&selected_shading_language))
	{
		// Make sure it's one of the supported shading languages
		std::string shading_language = parser.as<std::string>(&selected_shading_language);
		std::transform(shading_language.begin(), shading_language.end(), shading_language.begin(), ::tolower);
		if (shading_language == "glsl")
		{
			LOGI("Shading language selection: GLSL");
			vkb::Application::set_shading_language(vkb::ShadingLanguage::GLSL);
		}
		else if (shading_language == "hlsl")
		{
			LOGI("Shading langauge selection: HLSL")
			vkb::Application::set_shading_language(vkb::ShadingLanguage::HLSL);
		}
		else
		{
			LOGE("Invalid shading language selection, defaulting to glsl");
		}
	}
}
}        // namespace plugins