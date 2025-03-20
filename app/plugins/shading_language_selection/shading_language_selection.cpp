/* Copyright (c) 2024-2025, Sascha Willems
 * Copyright (c) 2025, NVIDIA CORPORATION. All rights reserved.
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
                                 "A collection of flags to select shader from different shading languages (glsl, hlsl or slang)",
                                 {},
                                 {},
                                 {{"shading-language", "Shading language to use (glsl, hlsl or slang)"}})
{
}

bool ShadingLanguageSelection::handle_option(std::deque<std::string> &arguments)
{
	assert(!arguments.empty() && (arguments[0].substr(0, 2) == "--"));
	std::string option = arguments[0].substr(2);
	if (option == "shading-language")
	{
		if (arguments.size() < 2)
		{
			LOGE("Option \"shading-language\" is missing the actual shading language to use!");
			return false;
		}

		// Make sure it's one of the supported shading languages
		std::string shading_language = arguments[1];
		std::transform(shading_language.begin(), shading_language.end(), shading_language.begin(), ::tolower);
		if (shading_language == "glsl")
		{
			LOGI("Shading language selection: GLSL");
			vkb::Application::set_shading_language(vkb::ShadingLanguage::GLSL);
		}
		else if (shading_language == "hlsl")
		{
			LOGI("Shading language selection: HLSL")
			vkb::Application::set_shading_language(vkb::ShadingLanguage::HLSL);
		}
		else if (shading_language == "slang")
		{
			LOGI("Shading language selection: slang")
			vkb::Application::set_shading_language(vkb::ShadingLanguage::SLANG);
		}
		else
		{
			LOGE("Invalid shading language selection, defaulting to glsl");
		}

		arguments.pop_front();
		arguments.pop_front();
		return true;
	}
	return false;
}
}        // namespace plugins