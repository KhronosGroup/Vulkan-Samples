/*
* Copyright 2020 NVIDIA Corporation.
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

#ifndef INCLUDE_YCBCRVKINFO_H_
#define INCLUDE_YCBCRVKINFO_H_

#include <string.h>
#include <cassert>
#include <math.h>
#include "ycbcr_utils.h"
#include "vulkan_interfaces.h"

#ifndef VK_MAX_NUM_IMAGE_PLANES_EXT
#define VK_MAX_NUM_IMAGE_PLANES_EXT 4
#endif

// Multi-planar formats info
typedef struct VkMpFormatInfo {
    VkFormat              vkFormat;
    YcbcrPlanesLayoutInfo planesLayout;     // Plane memory layout;
    VkFormat              vkPlaneFormat[VK_MAX_NUM_IMAGE_PLANES_EXT]; // VkFormats for the corresponding plane.
} VkMpFormatInfo;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief YcbcrVkFormatInfo returns the ycbcr VkFormat description.
 *
 * @param vkFormat  Vulkan format to be described.
 * @retval pointer to VkMpFormatInfo structure describing the ycbcr format.
 */
const VkMpFormatInfo * YcbcrVkFormatInfo(const VkFormat format);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_YCBCRVKINFO_H_ */
