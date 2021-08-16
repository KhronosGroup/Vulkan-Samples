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

#if !defined(OPENCL_EXPORTED_FUNCTION)
#	define OPENCL_EXPORTED_FUNCTION(fun)
#endif

OPENCL_EXPORTED_FUNCTION(clCreateContext);
OPENCL_EXPORTED_FUNCTION(clGetDeviceIDs);
OPENCL_EXPORTED_FUNCTION(clGetPlatformIDs);
OPENCL_EXPORTED_FUNCTION(clCreateBuffer);
OPENCL_EXPORTED_FUNCTION(clReleaseMemObject);
OPENCL_EXPORTED_FUNCTION(clImportMemoryARM);
OPENCL_EXPORTED_FUNCTION(clCreateProgramWithSource);
OPENCL_EXPORTED_FUNCTION(clBuildProgram);
OPENCL_EXPORTED_FUNCTION(clCreateKernel);
OPENCL_EXPORTED_FUNCTION(clSetKernelArg);
OPENCL_EXPORTED_FUNCTION(clEnqueueNDRangeKernel);
OPENCL_EXPORTED_FUNCTION(clFlush);
OPENCL_EXPORTED_FUNCTION(clFinish);
OPENCL_EXPORTED_FUNCTION(clCreateCommandQueue);
OPENCL_EXPORTED_FUNCTION(clReleaseContext);

#undef OPENCL_EXPORTED_FUNCTION