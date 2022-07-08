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

#include "components/platform/sample_main.hpp"

int sample_main(components::PlatformContext *context)
{
#if defined(_WIN32)
	if (dynamic_cast<components::WindowsContext *>(context) == nullptr)
#elif defined(__ANDROID__)
	if (dynamic_cast<components::AndroidContext *>(context) == nullptr)
#elif defined(__APPLE__) || defined(__MACH__)
	if (dynamic_cast<components::MacOSXContext *>(context) == nullptr)
#elif defined(__linux__)
	if (dynamic_cast<components::UnixContext *>(context) == nullptr)
#endif
	{
		return false;
	}

	// sample was loaded correctly with the correct context
	return true;
}