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

#include <components/platform/platform.hpp>

using namespace components;

CUSTOM_MAIN(context)
{
	if (context == nullptr)
	{
		throw "context should not be null";
	}

#if defined(_WIN32)
	if (dynamic_cast<WindowsContext *>(context) == nullptr)
#elif defined(__ANDROID__)
	if (dynamic_cast<AndroidContext *>(context) == nullptr)
#elif defined(__APPLE__) || defined(__MACH__)
	if (dynamic_cast<MacOSXContext *>(context) == nullptr)
#elif defined(__linux__)
	if (dynamic_cast<UnixContext *>(context) == nullptr)
#endif
	{
		throw "incorrect context provided for this platform";
	}

	return EXIT_SUCCESS;
}