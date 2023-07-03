/* Copyright (c) 2019-2023, Arm Limited and Contributors
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

#include <core/platform/entrypoint.hpp>

#if defined(PLATFORM__ANDROID)
#	include "platform/android/android_platform.h"
#elif defined(PLATFORM__WINDOWS)
#	include "platform/windows/windows_platform.h"
#elif defined(PLATFORM__LINUX_D2D)
#	include "platform/unix/unix_d2d_platform.h"
#elif defined(PLATFORM__LINUX) || defined(PLATFORM__MACOS)
#	include "platform/unix/unix_platform.h"
#else
#	error "Platform not supported"
#endif

CUSTOM_MAIN(context)
{
#if defined(PLATFORM__ANDROID)
	vkb::AndroidPlatform platform{context};
#elif defined(PLATFORM__WINDOWS)
	vkb::WindowsPlatform platform{context};
#elif defined(PLATFORM__LINUX_D2D)
	vkb::UnixD2DPlatform platform{context};
#elif defined(PLATFORM__LINUX)
	vkb::UnixPlatform platform{context, vkb::UnixType::Linux};
#elif defined(PLATFORM__MACOS)
	vkb::UnixPlatform platform{context, vkb::UnixType::Mac};
#else
#	error "Platform not supported"
#endif

auto code = platform.initialize(plugins::get_all());

	if (code == vkb::ExitCode::Success)
	{
		code = platform.main_loop();
	}

	platform.terminate(code);

	return 0;
}