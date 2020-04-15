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

#include "sponza.h"

#include "gltf_loader.h"
#include "gui.h"
#include "platform/platform.h"
#include "stats/stats.h"
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
#	include "platform/android/android_platform.h"
#endif

SponzaTest::SponzaTest() :
    vkbtest::GLTFLoaderTest("scenes/sponza/Sponza01.gltf")
{
}

std::unique_ptr<vkb::VulkanSample> create_sponza_test()
{
	return std::make_unique<SponzaTest>();
}
