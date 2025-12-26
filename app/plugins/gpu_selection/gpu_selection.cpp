/* Copyright (c) 2023-2025, Sascha Willems
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

#include "gpu_selection.h"
#include "core/instance.h"

#include <algorithm>

namespace plugins
{
GpuSelection::GpuSelection() :
    GpuSelectionTags("GPU selection",
                     "A collection of flags to select the GPU to run the samples on",
                     {},
                     {},
                     {{"gpu", "Zero-based index of the GPU that the sample should use"}})
{
}

bool GpuSelection::handle_option(std::deque<std::string> &arguments)
{
	assert(!arguments.empty() && (arguments[0].substr(0, 2) == "--"));
	std::string option = arguments[0].substr(2);
	if (option == "gpu")
	{
		if (arguments.size() < 2)
		{
			LOGE("Option \"gpu\" is missing the actual gpu index!");
			return false;
		}
		uint32_t gpu_index = static_cast<uint32_t>(std::stoul(arguments[1]));

		vkb::core::InstanceC::selected_gpu_index   = gpu_index;
		vkb::core::InstanceCpp::selected_gpu_index = gpu_index;

		arguments.pop_front();
		arguments.pop_front();
		return true;
	}
	return false;
}
}        // namespace plugins