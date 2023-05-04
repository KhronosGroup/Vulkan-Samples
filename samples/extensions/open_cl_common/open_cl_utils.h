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

#pragma once

#define CL_TARGET_OPENCL_VERSION 300
#include <CL/opencl.h>
#include <CL/cl_ext.h>
#include <functional>

#define OPENCL_EXPORTED_FUNCTION(func_name) extern std::function<decltype(func_name)> func_name##_ptr
#define OPENCL_EXPORTED_EXTENSION_FUNCTION(func_name) extern std::function<decltype(func_name)> func_name##_ptr
#include "open_cl_functions.inl"
#undef OPENCL_EXPORTED_FUNCTION
#undef OPENCL_EXPORTED_EXTENSION_FUNCTION

cl_platform_id load_opencl();
void           unload_opencl();

#ifdef CL_FUNCTION_DEFINITIONS

#	define clCreateContext clCreateContext_ptr
#	define clGetDeviceIDs clGetDeviceIDs_ptr
#	define clGetPlatformIDs clGetPlatformIDs_ptr
#	define clCreateBuffer clCreateBuffer_ptr
#	define clReleaseMemObject clReleaseMemObject_ptr
#	define clCreateProgramWithSource clCreateProgramWithSource_ptr
#	define clBuildProgram clBuildProgram_ptr
#	define clCreateKernel clCreateKernel_ptr
#	define clSetKernelArg clSetKernelArg_ptr
#	define clEnqueueNDRangeKernel clEnqueueNDRangeKernel_ptr
#	define clFlush clFlush_ptr
#	define clFinish clFinish_ptr
#	define clCreateCommandQueue clCreateCommandQueue_ptr
#	define clReleaseContext clReleaseContext_ptr
#	define clGetPlatformInfo clGetPlatformInfo_ptr
#	define clGetExtensionFunctionAddressForPlatform clGetExtensionFunctionAddressForPlatform_ptr
#	define clImportMemoryARM clImportMemoryARM_ptr
#	define clCreateImageWithProperties clCreateImageWithProperties_ptr
#	define clEnqueueWaitSemaphoresKHR clEnqueueWaitSemaphoresKHR_ptr
#	define clEnqueueSignalSemaphoresKHR clEnqueueSignalSemaphoresKHR_ptr
#	define clEnqueueAcquireExternalMemObjectsKHR clEnqueueAcquireExternalMemObjectsKHR_ptr
#	define clEnqueueReleaseExternalMemObjectsKHR clEnqueueReleaseExternalMemObjectsKHR_ptr
#	define clCreateSemaphoreWithPropertiesKHR clCreateSemaphoreWithPropertiesKHR_ptr
#	define clReleaseSemaphoreKHR clReleaseSemaphoreKHR_ptr 

#endif