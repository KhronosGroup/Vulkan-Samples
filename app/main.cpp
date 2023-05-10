/* Copyright (c) 2019-2021, Arm Limited and Contributors
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

#include "common/logging.h"
#include "platform/platform.h"
#include "plugins/plugins.h"

#if defined(VK_USE_PLATFORM_METAL_EXT)
#include <TargetConditionals.h>
#endif

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
#	include "platform/android/android_platform.h"
void android_main(android_app *state)
{
	vkb::AndroidPlatform platform{state};
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
#	include "platform/windows/windows_platform.h"
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     PSTR lpCmdLine, INT nCmdShow)
{
	vkb::WindowsPlatform platform{hInstance, hPrevInstance,
	                              lpCmdLine, nCmdShow};
#elif defined(VK_USE_PLATFORM_DISPLAY_KHR)
#	include "platform/unix/unix_d2d_platform.h"
int main(int argc, char *argv[])
{
	vkb::UnixD2DPlatform platform{argc, argv};
#elif defined(VK_USE_PLATFORM_METAL_EXT) && (defined(TARGET_IPHONE_SIMULATOR) || defined(TARGET_OS_IPHONE))
#	include "platform/ios/ios_platform.h"
int main(int argc, char** argv)
{
	vkb::IosPlatform platform(argc, argv);
#else
#	include "platform/unix/unix_platform.h"
int main(int argc, char *argv[])
{
#	if defined(VK_USE_PLATFORM_METAL_EXT)
	vkb::UnixPlatform platform{vkb::UnixType::Mac, argc, argv};
#	elif defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_XLIB_KHR) || defined(VK_USE_PLATFORM_WAYLAND_KHR)
	vkb::UnixPlatform platform{vkb::UnixType::Linux, argc, argv};
#	endif
#endif

	auto code = platform.initialize(plugins::get_all());

	if (code == vkb::ExitCode::Success)
	{
		code = platform.main_loop();
	}

	platform.terminate(code);

#ifndef VK_USE_PLATFORM_ANDROID_KHR
	return EXIT_SUCCESS;
#endif
}
