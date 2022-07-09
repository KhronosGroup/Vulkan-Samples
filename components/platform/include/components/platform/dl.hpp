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

#pragma once

#include <string>

#define EXPORT
#define IMPORT

#if defined(_WIN32)
#	undef EXPORT
#	undef IMPORT
#	define EXPORT __declspec(dllexport)
#	define IMPORT __declspec(dllimport)
#endif

#define EXPORT_CLIB extern "C" EXPORT
#define IMPORT_CLIB extern "C" IMPORT

namespace components
{
namespace dl
{
const char *os_library_prefix();
const char *os_library_postfix();

/**
 * @brief Convert a library name to an OS specific library name
 *
 * @param name library name
 * @return std::string OS library name
 */
std::string os_library_name(const std::string &name);

/**
 * @brief Open a dynamic library
 *
 * @param library_path library path
 * @return void* library handle
 */
void *open_library(const char *library_path);

/**
 * @brief Load a function ptr from a library
 *
 * @param library library handle
 * @param function_name function name
 * @return void* function ptr
 */
void *load_function(void *library, const char *function_name);

/**
 * @brief Load a function ptr from a library and cast to a specific type
 *
 * @tparam PFN type to cast too
 * @param library library handle
 * @param function_name function name
 * @return PFN function pointer
 */
template <typename PFN>
PFN load_function(void *library, const char *function_name)
{
	return reinterpret_cast<PFN>(load_function(library, function_name));
}
}        // namespace dl
}        // namespace components