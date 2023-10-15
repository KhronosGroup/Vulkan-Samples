/* Copyright (c) 2023, Sascha Willems
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

#include <algorithm>

#include "core/hpp_instance.h"
#include "core/instance.h"

namespace plugins
{
GpuSelection::GpuSelection() :
    GpuSelectionTags("GPU selection",
                     "A collection of flags to select the GPU to run the samples on",
                     {}, {&gpu_selection_options_group})
{
}

bool GpuSelection::is_active(const vkb::CommandParser &parser)
{
	return true;
}

void GpuSelection::init(const vkb::CommandParser &parser)
{
	// @todo: required?
	if (parser.contains(&selected_gpu_index))
	{
		vkb::Instance::selected_gpu_index          = parser.as<uint32_t>(&selected_gpu_index);
		vkb::core::HPPInstance::selected_gpu_index = parser.as<uint32_t>(&selected_gpu_index);
	}
}
}        // namespace plugins