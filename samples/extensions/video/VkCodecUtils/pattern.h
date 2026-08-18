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

#ifndef CMDS_VK_VIDEO_PLAYER_PATTERN_H_
#define CMDS_VK_VIDEO_PLAYER_PATTERN_H_

#include <stdint.h>
#include "VkCodecUtils/VulkanDeviceContext.h"

namespace Pattern {

typedef enum ColorPattern {
    ColorPatternClear,
    ColorPatternColorBars,
    ColorPatternGrad
} ColorPattern;

void generateColorPatternRgba8888(
    ColorPattern pattern, uint8_t *dataPtr, uint32_t width, uint32_t height,
    uint32_t strideBytes, uint32_t channelsPerColor = 4, uint8_t maxC = 255,
    uint8_t minC = 0, uint8_t alphaMax = 255,
    const uint8_t clearColor[4] = nullptr, uint32_t skipChannelsMask = 0,
    bool incOnSkip = false);

void generateColorPatternRgba16161616(
    ColorPattern pattern, uint16_t *dataPtr, uint32_t width, uint32_t height,
    uint32_t strideBytes, uint32_t channelsPerColor = 4, uint16_t maxC = 255,
    uint16_t minC = 0, uint16_t alphaMax = 255,
    const uint16_t clearColor[4] = nullptr, uint32_t skipChannelsMask = 0,
    bool incOnSkip = false);

}

#include "nvidia_utils/vulkan/ycbcrvkinfo.h"


typedef struct ImageData {
    VkFormat     imageFormat;
    uint32_t     width;
    uint32_t     height;
    Pattern::ColorPattern patttern;
    uint8_t      clearColor[4];
    void const * data;
} ImageData;

namespace Pattern {

typedef struct VkFormatDesc {
    VkFormat    format;
    uint8_t     numberOfChannels;
    uint8_t     numberOfBytes;
    const char* name;
} VkFormatDesc;

class VkFillYuv {
public:

    VkFillYuv()
    {
    };

    ~VkFillYuv()
    {

    }

    void fillVkImage(VkImage vkImage, const ImageData *pImageData,
                     VkDeviceMemory mem, uint8_t *mappedHostPtr, const VkSamplerYcbcrConversionCreateInfo* pSamplerYcbcrConversionCreateInfo,
                     VkImageAspectFlags aspectMask = 0, VkFormat aspectMainFormat = VK_FORMAT_UNDEFINED);

    void fillVkCommon(const ImageData *pImageData, VkSubresourceLayout layouts[3], const VkSamplerYcbcrConversionCreateInfo* pSamplerYcbcrConversionCreateInfo,
                      const VkMpFormatInfo *mpInfo, uint8_t *ptr, VkDeviceSize size,
                      VkImageAspectFlags aspectMask = 0, VkFormat aspectMainFormat = VK_FORMAT_UNDEFINED);

#if defined(NV_ANDROID)
    int fillAndroidNB(AHardwareBufferHandle aHardwareBufferHandle, const ImageData *pImageData, const VkSamplerYcbcrConversionCreateInfo* pSamplerYcbcrConversionCreateInfo);
    int fillRmSurface(const NvRmSurface *surf, uint32_t numSurfaces, const ImageData *pImageData, const VkSamplerYcbcrConversionCreateInfo* pSamplerYcbcrConversionCreateInfo);
#endif // defined(NV_ANDROID)

private:

};

int ConvertRgbToYcbcr(const VkSamplerYcbcrConversionCreateInfo * pYcbcrConversionInfo, YcbcrLevelsRange levelRange,
                      const void * colRgbaPtr, uint32_t width, uint32_t height, uint32_t rgbStrideBytes, uint32_t rgbChannelsPerColor, uint32_t rgbBpp,
                      void *yuv, VkSubresourceLayout layouts[3], uint32_t skipChannelsMask);

}

#endif /* CMDS_VK_VIDEO_PLAYER_PATTERN_H_ */
