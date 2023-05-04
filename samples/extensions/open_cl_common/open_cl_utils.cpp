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

// @todo: move to sample unit

#include "open_cl_utils.h"

#if defined(__linux__)
#	include <dlfcn.h>
#elif defined(_WIN32)
#	include <windows.h>
#endif
#include <string>
#include <vector>

#define OPENCL_EXPORTED_FUNCTION(func_name) std::function<decltype(func_name)> func_name##_ptr = nullptr
#define OPENCL_EXPORTED_EXTENSION_FUNCTION(func_name) std::function<decltype(func_name)> func_name##_ptr = nullptr
#include "open_cl_functions.inl"

static void *handle = nullptr;

cl_platform_id load_opencl()
{
	// Check if OpenCL is supported by trying to load the OpenCL library and getting a valid function pointer
	bool openCLAvailable = false;

#if defined(__ANDROID__)
	// Try to find the OepenCL library on one of the following paths
	static const char *libraryPaths[] = {
	    // Generic
	    "/system/vendor/lib64/libOpenCL.so",
	    "/system/lib64/libOpenCL.so",
	    "/system/vendor/lib/libOpenCL.so",
	    "/system/lib/libOpenCL.so",
	    // ARM Mali
	    "/system/vendor/lib64/egl/libGLES_mali.so",
	    "/system/lib64/egl/libGLES_mali.so",
	    "/system/vendor/lib/egl/libGLES_mali.so",
	    "/system/lib/egl/libGLES_mali.so",
	    // PowerVR
	    "/system/vendor/lib64/libPVROCL.so",
	    "/system/lib64/libPVROCL.so",
	    "/system/vendor/lib/libPVROCL.so",
	    "/system/lib/libPVROCL.so"};
	for (auto libraryPath : libraryPaths)
	{
		handle = dlopen(libraryPath, RTLD_LAZY);
		if (handle)
		{
			break;
		}
	}
#elif defined(__linux__)
	// Try to find the OepenCL library on one of the following paths
	static const char *libraryPaths[] = {
	    "libOpenCL.so",
	    "/usr/lib/libOpenCL.so",
	    "/usr/local/lib/libOpenCL.so",
	    "/usr/local/lib/libpocl.so",
	    "/usr/lib64/libOpenCL.so",
	    "/usr/lib32/libOpenCL.so",
	    "libOpenCL.so.1",
	    "/usr/lib/libOpenCL.so.1",
	    "/usr/local/lib/libOpenCL.so.1",
	    "/usr/local/lib/libpocl.so.1",
	    "/usr/lib64/libOpenCL.so.1",
	    "/usr/lib32/libOpenCL.so.1"};
	for (auto libraryPath : libraryPaths)
	{
		handle = dlopen(libraryPath, RTLD_LAZY);
		if (handle)
		{
			break;
		}
	}
#elif defined(_WIN32)
	handle = LoadLibraryA("OpenCL.dll");
	if (handle)
	{
		char libPath[MAX_PATH] = {0};
		GetModuleFileNameA(static_cast<HMODULE>(handle), libPath, sizeof(libPath));
	}
#endif

	if (handle == nullptr)
	{
		return nullptr;
	}

#if defined(_WIN32)
#define OPENCL_EXPORTED_FUNCTION(func_name) func_name##_ptr = reinterpret_cast<decltype(func_name) *>(GetProcAddress((HMODULE)handle, #func_name))
#else
#define OPENCL_EXPORTED_FUNCTION(func_name) func_name##_ptr = reinterpret_cast<decltype(func_name) *>(dlsym(handle, #func_name))
#endif
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
#if defined(_WIN32)
		FreeLibrary((HMODULE) handle);
#else
		dlclose(handle);
#endif
		handle = nullptr;
	}
}
