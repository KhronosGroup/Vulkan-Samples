/* Copyright (c) 2018-2020, Arm Limited and Contributors
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
		__pragma(warning(push, 0))  \
		    __pragma(warning(disable : 4702))

#	define VKBP_ENABLE_WARNINGS() \
		__pragma(warning(pop))

#	pragma warning(disable : 4820 4061 4625 4626 4355 5204 5220 5026 5027 4100 4324 4514 4365 5045 4263 4264 4389 4868 4623 4242 4244 5039)

// C4820 - 'bytes' bytes padding added after construct 'member_name'
// C4061 - enumerator 'identifier' in switch of enum 'enumeration' is not explicitly handled by a case label
// C4625 - 'derived class' : copy constructor was implicitly defined as deleted because a base class copy constructor is inaccessible or deleted
// C4626 - 'derived class' : assignment operator was implicitly defined as deleted because a base class assignment operator is inaccessible or deleted
// C4355 - 'this' : used in base member initializer list
// C5026 - 'type': move constructor was implicitly defined as deleted
// C5027 - 'type': move assignment operator was implicitly defined as deleted
// C4100 - unreferenced formal parameter
// C4324 - structure was padded due to alignment specifier
// C4514 - unreferenced inline function has been removed
// C4365 - conversion from x to y, signed/unsigned mismatch
// C5045 - Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified
// C4263 - 'void ApiVulkanSample::render(float)': member function does not override any base class virtual member function
// C4264 - 'void vkb::VulkanSample::render(vkb::CommandBuffer &)': no override available for virtual member function from base 'vkb::VulkanSample'; function is hidden
// C4389 - signed/unsigned mismatch
// C4868 - compiler may not enforce left-to-right evaluation order in braced initializer list
// C4623 - 'vkb::sg::AnimationChannel': default constructor was implicitly defined as deleted
// C4242 C4244 -  conversion from 'X' to 'Y', possible loss of data (occurs in algorithm annoyingly...)
// C5039 - pointer or reference to potentially throwing function passed to 'extern "C"' function under -EHc. Undefined behavior may occur if this function throws an exception (GLFW window callbacks)

// Concerns C4263, C4264, C5039
#endif