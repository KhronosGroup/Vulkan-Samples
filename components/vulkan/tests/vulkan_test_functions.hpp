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

#include <volk.h>

namespace components
{
namespace vulkan
{
namespace test
{
/**
 * @brief utility used to wrap a vulkan function pointer
 * 
 * @tparam Override PFN_vkFunctionX
 * @tparam Func A function used to override the original
 * @param func See Func
 * @return Override A new PFN_vkFunctionX function
 */
template <typename Override, typename Func>
Override wrapper(Func &&func)
{
	return (Override) func;
}

// not a complete list of vulkan functions.
// add default implementations here when they are used in testing
extern PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties;
extern PFN_vkCreateInstance                   vkCreateInstance;
extern PFN_vkDestroyDebugReportCallbackEXT    vkDestroyDebugReportCallbackEXT;
extern PFN_vkDestroyDebugUtilsMessengerEXT    vkDestroyDebugUtilsMessengerEXT;
extern PFN_vkDestroyInstance                  vkDestroyInstance;
}        // namespace test
}        // namespace vulkan
}        // namespace components