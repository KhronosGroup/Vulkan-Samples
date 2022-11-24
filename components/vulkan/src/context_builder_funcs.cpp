/* Copyright (c) 2022, Arm Limited and Contributors
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

#include "components/vulkan/context/context_builder.hpp"

namespace components
{
namespace vulkan
{
namespace funcs
{
void apply_debug_configuration(ContextBuilder &builder)
{
}

void apply_default_configuration(ContextBuilder &builder)
{
	// clang-format off

	builder
	    .ConfigureInstance()
            .application_info([]() -> VkApplicationInfo {
                return {
                    VK_STRUCTURE_TYPE_APPLICATION_INFO,
                    nullptr,
                    "Vulkan Samples",
                    VK_MAKE_VERSION(1, 0, 0),
                    "Vulkan Samples Framework",
                    VK_MAKE_VERSION(1, 0, 0),
                    VK_API_VERSION_1_3,
                };
            })
            .Done()
	    .Build();

	// clang-format on
}
}        // namespace funcs
}        // namespace vulkan
}        // namespace components