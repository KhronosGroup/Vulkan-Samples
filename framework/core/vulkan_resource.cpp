/* Copyright (c) 2021, Arm Limited and Contributors
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

#include "core/vulkan_resource.h"

#include "core/device.h"

namespace vkb
{
namespace core
{
namespace detail
{
void set_debug_name(const Device *device, VkObjectType object_type, uint64_t handle, const char *debug_name)
{
	if (!debug_name || *debug_name == '\0' || !device)
	{
		// Can't set name, or no point in setting an empty name
		return;
	}

	device->get_debug_utils().set_debug_name(device->get_handle(), object_type, handle, debug_name);
}

}        // namespace detail
}        // namespace core
}        // namespace vkb