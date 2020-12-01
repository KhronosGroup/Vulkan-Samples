/* Copyright (c) 2019-2020, Arm Limited and Contributors
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
#else
#	include "platform/unix/unix_platform.h"
int main(int argc, char *argv[])
{
#	if defined(VK_USE_PLATFORM_MACOS_MVK)
	vkb::UnixPlatform platform{vkb::UnixType::Mac, argc, argv};
#	elif defined(VK_USE_PLATFORM_XCB_KHR)
	vkb::UnixPlatform platform{vkb::UnixType::Linux, argc, argv};
#	endif
#endif

	// #ifndef DEBUG
	// 	try
	// 	{
	// #endif
	// 		if (platform.initialize())
	// 		{
	// 			platform.main_loop();
	// 			platform.terminate(vkb::ExitCode::Success);
	// 		}
	// 		else
	// 		{
	// 			platform.terminate(vkb::ExitCode::UnableToRun);
	// 		}
	// #ifndef DEBUG
	// 	}
	// 	catch (const std::exception &e)
	// 	{
	// 		LOGE(e.what());
	// 		platform.terminate(vkb::ExitCode::FatalError);
	// 	}
	// #endif

#ifdef VK_USE_PLATFORM_ANDROID_KHR
	platform.main_loop();        // Continue to process events until onDestroy()
#else
	return EXIT_SUCCESS;
#endif
}
