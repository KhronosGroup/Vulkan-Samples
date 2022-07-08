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

#include "components/platform/dl.hpp"

#include <cassert>
#include <sstream>

#ifdef _WIN32
#	include <windows.h>
#else
#	include <dlfcn.h>
#endif

namespace components
{
namespace dl
{
std::string os_library_name(const std::string &name)
{
	std::stringstream lib_name;
#ifdef _WIN32
	lib_name << name << ".dll";
#elif defined(__APPLE__)
	lib_name << "lib" << name << ".dylib";
#else
	lib_name << "lib" << name << ".so";
#endif

	return lib_name.str();
}

void *open_library(const char *library_name)
{
#if defined(_WIN32)
	HMODULE module = LoadLibraryA(library_name);
	if (!module)
	{
		return nullptr;
	}

	return reinterpret_cast<void *>(module);
#elif defined(__APPLE__)
	return dlopen(library_name, RTLD_NOW | RTLD_LOCAL);
#else
	return dlopen(library_name, RTLD_NOW | RTLD_LOCAL);
#endif
}

void *load_function(void *library, const char *function_name)
{
	assert(library && "library must be a pointer to a dl handle. see open_library()");

#if defined(_WIN32)
	HMODULE module = static_cast<HMODULE>(library);
	return reinterpret_cast<void *>(GetProcAddress(module, function_name));
#elif defined(__APPLE__)
	return dlsym(library, function_name);
#else
	return dlsym(library, function_name);
#endif
}
}        // namespace dl
}        // namespace components