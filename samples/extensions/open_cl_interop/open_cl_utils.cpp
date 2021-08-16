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

#define OPENCL_EXPORTED_FUNCTION(func_name) std::function<decltype(func_name)> func_name##_ptr = nullptr
#include "open_cl_functions.inl"

static void *handle = nullptr;

bool load_opencl()
{
	handle = dlopen("libOpenCL.so", RTLD_LAZY | RTLD_LOCAL);

	if (handle == nullptr)
	{
		return false;
	}

#define OPENCL_EXPORTED_FUNCTION(func_name) func_name##_ptr = reinterpret_cast<decltype(func_name) *>(dlsym(handle, #func_name));
#include "open_cl_functions.inl"

	return true;
}

void unload_opencl()
{
	if (handle)
	{
		dlclose(handle);
		handle = nullptr;
	}
}
