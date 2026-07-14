/*
* Copyright 2021 NVIDIA Corporation.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef _NVVULKANVIDEOPARSER_H_
#define _NVVULKANVIDEOPARSER_H_

// NVPARSER_EXPORT tags symbol that will be exposed by the shared library.
#if defined(NVPARSER_SHAREDLIB)
#if defined(_WIN32)
#if defined(NVPARSER_IMPLEMENTATION)
#define NVPARSER_EXPORT __declspec(dllexport)
#else
#define NVPARSER_EXPORT __declspec(dllimport)
#endif
#else
#if defined(NVPARSER_IMPLEMENTATION)
#define NVPARSER_EXPORT __attribute__((visibility("default")))
#else
#define NVPARSER_EXPORT
#endif
#endif
#else
#define NVPARSER_EXPORT
#endif

typedef void (*nvParserLogFuncType)(const char* format, ...);

class VulkanVideoDecodeParser;
NVPARSER_EXPORT
VkResult CreateVulkanVideoDecodeParser(VkVideoCodecOperationFlagBitsKHR videoCodecOperation,
                                       const VkExtensionProperties* pStdExtensionVersion,
                                       nvParserLogFuncType pParserLogFunc, int logLevel,
                                       const VkParserInitDecodeParameters* pParserPictureData,
                                       VkSharedBaseObj<VulkanVideoDecodeParser>& nvVideoDecodeParser);

#endif /* _NVVULKANVIDEOPARSER_H_ */
