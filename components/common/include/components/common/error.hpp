/* Copyright (c) 2023, Thomas Atkinson
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

#include <fmt/format.h>

#if defined(__GNUC__) && !defined(__llvm__) && !defined(__INTEL_COMPILER)
#	define __GCC__ __GNUC__
#endif

#if defined(__clang__)
// CLANG ENABLE/DISABLE WARNING DEFINITION
#	define VKBP_DISABLE_WARNINGS()                             \
		_Pragma("clang diagnostic push")                        \
		    _Pragma("clang diagnostic ignored \"-Wall\"")       \
		        _Pragma("clang diagnostic ignored \"-Wextra\"") \
		            _Pragma("clang diagnostic ignored \"-Wtautological-compare\"")

#	define VKBP_ENABLE_WARNINGS() \
		_Pragma("clang diagnostic pop")
#elif defined(__GNUC__) || defined(__GNUG__)
// GCC ENABLE/DISABLE WARNING DEFINITION
#	define VKBP_DISABLE_WARNINGS()                             \
		_Pragma("GCC diagnostic push")                          \
		    _Pragma("GCC diagnostic ignored \"-Wall\"")         \
		        _Pragma("clang diagnostic ignored \"-Wextra\"") \
		            _Pragma("clang diagnostic ignored \"-Wtautological-compare\"")

#	define VKBP_ENABLE_WARNINGS() \
		_Pragma("GCC diagnostic pop")
#elif defined(_MSC_VER)
// MSVC ENABLE/DISABLE WARNING DEFINITION
#	define VKBP_DISABLE_WARNINGS() \
		__pragma(warning(push, 0))

#	define VKBP_ENABLE_WARNINGS() \
		__pragma(warning(pop))
#endif

#define ERROR(message)       \
	throw std::runtime_error \
	{                        \
		message              \
	}

#define ERRORF(format_, ...) ERROR(fmt::format(format_, __VA_ARGS__))

#define NOT_IMPLEMENTED() ERROR("not implemented");
