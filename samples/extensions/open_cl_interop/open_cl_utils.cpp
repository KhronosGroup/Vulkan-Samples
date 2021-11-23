/* Copyright (c) 2021, Arm Limited and Contributors
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

#include "open_cl_utils.h"

#include <dlfcn.h>
#include <string>
#include <vector>

#define OPENCL_EXPORTED_FUNCTION(func_name) std::function<decltype(func_name)> func_name##_ptr = nullptr
#define OPENCL_EXPORTED_EXTENSION_FUNCTION(func_name) std::function<decltype(func_name)> func_name##_ptr = nullptr
#include "open_cl_functions.inl"

static void *handle = nullptr;

cl_platform_id load_opencl()
{
	handle = dlopen("libOpenCL.so", RTLD_LAZY | RTLD_LOCAL);

	if (handle == nullptr)
	{
		return nullptr;
	}

#define OPENCL_EXPORTED_FUNCTION(func_name) func_name##_ptr = reinterpret_cast<decltype(func_name) *>(dlsym(handle, #func_name))
#include "open_cl_functions.inl"

	cl_platform_id platform_id = nullptr;
	cl_uint        num_platforms;
	clGetPlatformIDs_ptr(1, &platform_id, &num_platforms);

#define OPENCL_EXPORTED_EXTENSION_FUNCTION(func_name) func_name##_ptr = \
	                                                      reinterpret_cast<decltype(func_name) *>(clGetExtensionFunctionAddressForPlatform_ptr(platform_id, #func_name))
#include "open_cl_functions.inl"

	return platform_id;
}

void unload_opencl()
{
	if (handle)
	{
		dlclose(handle);
		handle = nullptr;
	}
}
