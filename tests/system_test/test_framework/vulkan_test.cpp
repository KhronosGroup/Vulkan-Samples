/* Copyright (c) 2019, Arm Limited and Contributors
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

#include "vulkan_test.h"

#include "gltf_loader.h"
#include "gui.h"
#include "platform/platform.h"
#include "stats/stats.h"
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
#	include "platform/android/android_platform.h"
#endif

namespace vkbtest
{
bool VulkanTest::prepare(vkb::Platform &platform)
{
	if (!vkb::VulkanSample::prepare(platform))
	{
		return false;
	}

	this->platform = &platform;

	return true;
}

void VulkanTest::update(float delta_time)
{
	VulkanSample::update(delta_time);

	screenshot(get_render_context(), get_name());

	end();
}

void VulkanTest::end()
{
	platform->close();
	exit(0);
}
}        // namespace vkbtest
