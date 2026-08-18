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

#include <vulkan_interfaces.h>
#include "Helpers.h"
#include "VkCodecUtils/VulkanDeviceContext.h"
#include "pattern.h"

namespace Pattern {

template <class colorType>
void generateColorPatternRgba(ColorPattern pattern, colorType *dataPtr,
                              uint32_t width, uint32_t height,
                              uint32_t strideBytes, uint32_t channelsPerColor,
                              colorType maxC, colorType minC,
                              colorType alphaMax, const colorType clearColor[4],
                              uint32_t skipChannelsMask, bool incOnSkip)
{
    const uint32_t enabledChannelsMask = ~skipChannelsMask;
    switch (pattern) {
    case ColorPatternColorBars: {
        const colorType rgbBarColors[][4] = {
            {maxC, maxC, maxC, alphaMax},  // White
            {maxC, maxC, minC, alphaMax},  // Yellow
            {minC, maxC, maxC, alphaMax},  // Cyan
            {minC, maxC, minC, alphaMax},  // Green
            {maxC, minC, maxC, alphaMax},  // Magenta
            {maxC, minC, minC, alphaMax},  // Red
            {minC, minC, maxC, alphaMax},  // Blue
            {minC, minC, minC, alphaMax},  // Black
        };

        if (channelsPerColor > 4) {
            channelsPerColor = 4;
        }

        const uint32_t numColorBars = 8;

        uint32_t barWidthPixels = width / numColorBars;  // FIXME: if 0 then 1;

        colorType *rowStartPtr = dataPtr;
        for (unsigned int row = 0; row < height; row++) {
            colorType *rowPtr = rowStartPtr;
            for (unsigned int bar = 0; bar < numColorBars; bar++) {
                for (unsigned int barCol = 0; barCol < barWidthPixels; barCol++) {
                    unsigned int channelMask = 1;
                    for (unsigned int pix = 0; pix < channelsPerColor; pix++) {
                        if (channelMask & enabledChannelsMask) {
                            *rowPtr++ = rgbBarColors[bar][pix];
                        } else if (incOnSkip) {
                            rowPtr++;
                        }
                        channelMask <<= 1;
                    }
                }
            }
            rowStartPtr = (colorType *)((uint8_t *)rowStartPtr + strideBytes);
        }
    }
    break;
    case ColorPatternGrad: {
        colorType grad[4];
        grad[0] = grad[1] = grad[2] = minC;
        grad[3] = alphaMax;
        colorType step = (colorType)((maxC - minC) / height);

        colorType *rowStartPtr = dataPtr;
        for (unsigned int i = 0; i < height; i++) {
            colorType *rowPtr = rowStartPtr;
            for (unsigned int j = 0; j < width; j++) {
                unsigned int channelMask = 1;
                for (unsigned int pix = 0; pix < channelsPerColor; pix++) {
                    if (channelMask & enabledChannelsMask) {
                        *rowPtr++ = grad[pix];
                    } else if (incOnSkip) {
                        rowPtr++;
                    }
                    channelMask <<= 1;
                }
            }

            rowStartPtr = (colorType *)((uint8_t *)rowStartPtr + strideBytes);

            grad[0] = (colorType)(grad[0] + step);
            grad[1] = (colorType)(grad[1] + step);
            grad[2] = (colorType)(grad[2] + step);
        }
    }
    break;
    case ColorPatternClear: {
        colorType *rowStartPtr = dataPtr;
        for (unsigned int i = 0; i < height; i++) {
            colorType *rowPtr = rowStartPtr;
            for (unsigned int j = 0; j < width; j++) {
                unsigned int channelMask = 1;
                for (unsigned int pix = 0; pix < channelsPerColor; pix++) {
                    if (channelMask & enabledChannelsMask) {
                        *rowPtr++ = clearColor[pix];
                    } else if (incOnSkip) {
                        rowPtr++;
                    }
                    channelMask <<= 1;
                }
            }
            rowStartPtr = (colorType *)((uint8_t *)rowStartPtr + strideBytes);
        }
    }
    break;
    }
}

void generateColorPatternRgba8888(ColorPattern pattern, uint8_t *dataPtr,
                                  uint32_t width, uint32_t height,
                                  uint32_t strideBytes,
                                  uint32_t channelsPerColor, uint8_t maxC,
                                  uint8_t minC, uint8_t alphaMax,
                                  const uint8_t clearColor[4],
                                  uint32_t skipChannelsMask, bool incOnSkip)
{
    generateColorPatternRgba(pattern, dataPtr, width, height, strideBytes,
                             channelsPerColor, maxC, minC, alphaMax, clearColor,
                             skipChannelsMask, incOnSkip);
}

void generateColorPatternRgba16161616(
    ColorPattern pattern, uint16_t *dataPtr, uint32_t width, uint32_t height,
    uint32_t strideBytes, uint32_t channelsPerColor, uint16_t maxC,
    uint16_t minC, uint16_t alphaMax, const uint16_t clearColor[4],
    uint32_t skipChannelsMask, bool incOnSkip)
{
    generateColorPatternRgba(pattern, dataPtr, width, height, strideBytes,
                             channelsPerColor, maxC, minC, alphaMax, clearColor,
                             skipChannelsMask, incOnSkip);
}

#include <stdio.h>
#define ABORT_IF_TRUE(cond) \
    if (cond) { \
        printf("condition at %s %d failed, aborting\n", __FILE__, __LINE__); \
        return; \
    }

const VkFormatDesc vkFormatInfo[] = {
    { VK_FORMAT_R8_UNORM,                       1,   1,        "r8",                },
    { VK_FORMAT_R8G8_UNORM,                     2,   2,        "rg8",               },
    { VK_FORMAT_R8G8B8_UNORM,                   3,   3,        "rgb8",              },
    { VK_FORMAT_R8G8B8A8_UNORM,                 4,   4,        "rgba8",             },
    { VK_FORMAT_R32G32B32A32_SFLOAT,            4,  16,       "rgba32f",            },
    { VK_FORMAT_R16G16B16A16_SFLOAT,            4,   8,        "rgba16f",           },
    { VK_FORMAT_R32G32_SFLOAT,                  2,   8,        "rg32f",             },
    { VK_FORMAT_R16G16_SFLOAT,                  2,   4,        "rg16f",             },
    { VK_FORMAT_B10G11R11_UFLOAT_PACK32,        3,   4,        "r11f_g11f_b10f",    },
    { VK_FORMAT_R32_SFLOAT,                     1,   4,        "r32f",              },
    { VK_FORMAT_R16_SFLOAT,                     1,   2,        "r16f",              },
    { VK_FORMAT_R16G16B16A16_UNORM,             4,   8,        "rgba16",            },
    { VK_FORMAT_A2B10G10R10_UNORM_PACK32,       4,   4,        "rgb10_a2",          },
    { VK_FORMAT_R16G16_UNORM,                   2,   4,        "rg16",              },
    { VK_FORMAT_R16_UNORM,                      1,   2,        "r16",               },
    { VK_FORMAT_R16G16B16A16_SNORM,             4,   8,        "rgba16_snorm",      },
    { VK_FORMAT_R8G8B8A8_SNORM,                 4,   4,        "rgba8_snorm",       },
    { VK_FORMAT_R16G16_SNORM,                   2,   4,        "rg16_snorm",        },
    { VK_FORMAT_R8G8_SNORM,                     2,   2,        "rg8_snorm",         },
    { VK_FORMAT_R16_SNORM,                      1,   2,        "r16_snorm",         },
    { VK_FORMAT_R8_SNORM,                       1,   1,        "r8_snorm",          },
    { VK_FORMAT_R32G32B32A32_SINT,              4,  16,       "rgba32i",            },
    { VK_FORMAT_R16G16B16A16_SINT,              4,   8,        "rgba16i",           },
    { VK_FORMAT_R8G8B8A8_SINT,                  4,   4,        "rgba8i",            },
    { VK_FORMAT_R32G32_SINT,                    2,   8,        "rg32i",             },
    { VK_FORMAT_R16G16_SINT,                    2,   4,        "rg16i",             },
    { VK_FORMAT_R8G8_SINT,                      2,   2,        "rg8i",              },
    { VK_FORMAT_R32_SINT,                       1,   4,        "r32i",              },
    { VK_FORMAT_R16_SINT,                       1,   2,        "r16i",              },
    { VK_FORMAT_R8_SINT,                        1,   1,        "r8i",               },
    { VK_FORMAT_R32G32B32A32_UINT,              4,  16,       "rgba32ui",           },
    { VK_FORMAT_R16G16B16A16_UINT,              4,   8,        "rgba16ui",          },
    { VK_FORMAT_R8G8B8A8_UINT,                  4,   4,        "rgba8ui",           },
    { VK_FORMAT_R32G32_UINT,                    2,   8,        "rg32ui",            },
    { VK_FORMAT_R16G16_UINT,                    2,   4,        "rg16ui",            },
    { VK_FORMAT_R8G8_UINT,                      2,   2,        "rg8ui",             },
    { VK_FORMAT_R32_UINT,                       1,   4,        "r32ui",             },
    { VK_FORMAT_R16_UINT,                       1,   2,        "r16ui",             },
    { VK_FORMAT_R8_UINT,                        1,   1,        "r8ui",              },
    { VK_FORMAT_A2B10G10R10_UINT_PACK32,        4,   4,        "rgb10_a2ui",        },
};

static const VkFormatDesc* vkFormatLookUp(VkFormat format)
{
    const VkFormatDesc* pVkFormatDesc = NULL;
    for (unsigned int i = 0; i < sizeof(vkFormatInfo)/sizeof(vkFormatInfo[0]); i++) {
        if (vkFormatInfo[i].format == format) {
            pVkFormatDesc = &vkFormatInfo[i];
            break;
        }
    }

    return pVkFormatDesc;
}

static YcbcrBtStandard GetYcbcrPrimariesConstantsId(VkSamplerYcbcrModelConversion modelConversion)
{
    switch (modelConversion) {
    case VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_709:
        return YcbcrBtStandardBt709;
    case VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_601:
        return YcbcrBtStandardBt601Ebu;
    case VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_2020:
        return YcbcrBtStandardBt709;
    default:
        ;// assert(0);
    }

    return YcbcrBtStandardUnknown;
}

template <class rgbType, class yuvType>
static int rgbToYcbcr(const YcbcrBtMatrix* pYcbcrBtMatrix, const YcbcrNormalizeColorRange * pNormalizeColorRange, const  VkSamplerYcbcrConversionCreateInfo *,
                      const rgbType rgb[4], uint32_t rgbBpp, yuvType* yuv)
{

    if (!pYcbcrBtMatrix) {
        // Y   <-   G
        yuv[0] = rgb[1];
        // Cb  <-   B
        yuv[1] = rgb[2];
        // Cr  <-   R
        yuv[2] = rgb[0];

        return 0;
    }

    // 1. Normalize RGB values.
    const unsigned int rgbNormDiv = (1 << rgbBpp) - 1;
    float normRgbColor[3];

    for (unsigned int i = 0; i < 3; i++) {
        normRgbColor[i] = (float)rgb[i] / (float)rgbNormDiv;
    }

    // 2. Gamma correction in RGB space.
    // 3. Convert to Ycbcr
    float yuvNormColor[3];
    if (pYcbcrBtMatrix) {
        pYcbcrBtMatrix->ConvertRgbToYcbcr(yuvNormColor, normRgbColor);
    } else {
        // Y            <-       G
        yuvNormColor[0] = normRgbColor[1];
        // Cb           <-       B
        yuvNormColor[1] = normRgbColor[2];
        // Cr           <-       R
        yuvNormColor[2] = normRgbColor[0];
    }

    // 4. Clamp and un-normalize the YUV.
    pNormalizeColorRange->getIntValues(yuvNormColor, yuv);

    return 0;
}

template <class yuvType>
static int InterpoalteCbCr(const  VkSamplerYcbcrConversionCreateInfo*, const yuvType yuvSamples[2][2][3], uint32_t, yuvType yuv[3])
{
    // const uint32_t samplesX = (numSamples > 1) ? 2 : 1;
    // const uint32_t samplesY = (numSamples == 4) ? 2 : 1;

    // TODO: interpolate the uv samples.
    yuv[0] = yuvSamples[0][0][0];
    // Cb Sample;
    yuv[1] = yuvSamples[0][0][1];
    // Cr Sample;
    yuv[2] = yuvSamples[0][0][2];

    return 0;
}

// YCBCR_SINGLE_PLANE_INTERLEAVED: Single Plane Interleaved Layout
// Interleaved YUV format (1 plane) ex. YUY2, AYUV, UYVY;
template <class rgbType, class yuvType>
static int ConvertRgbToYcbcrSinglePlaneInterleavedLayout(const VkMpFormatInfo * mpInfo, const YcbcrBtMatrix* pYcbcrBtMatrix, const YcbcrNormalizeColorRange * pNormalizeColorRange,
        const  VkSamplerYcbcrConversionCreateInfo * pYcbcrConversionInfo,
        const rgbType *rgbaPtr, uint32_t width, uint32_t height, uint32_t rgbStrideBytes, uint32_t rgbChannelsPerColor, uint32_t rgbBpp, yuvType *yuvPtr, VkSubresourceLayout layouts[3], uint32_t)
{
    assert(mpInfo->planesLayout.layout == YCBCR_SINGLE_PLANE_INTERLEAVED);

    const bool yFirst = (mpInfo->planesLayout.channel0 == CC_Y0);

    const unsigned int cY0 = yFirst ? 0 : 1;
    const unsigned int cY1 = yFirst ? 2 : 3;
    const unsigned int cCb = yFirst ? 1 : 0;
    const unsigned int cCr = yFirst ? 3 : 2;

    const unsigned int samplesX = 2;

    const rgbType *rowRgbaStartPtr = rgbaPtr;
    yuvType *rowYuvStartPtr =  yuvPtr;
    for (unsigned int i = 0; i < height; i++) {
        const rgbType *rowRgbaPtr = rowRgbaStartPtr;
        yuvType *rowYuvPtr =  rowYuvStartPtr;
        for (unsigned int j = 0; j < width; j+= samplesX) {

            // Advance 2 rgba colors per iteration to get 2x1 RGBA pixels.
            //  yFirst == true  sequence is Y0 Cb Y1 Cr  Y0 Cb Y1 Cr
            //  yFirst == false sequence is Cb Y0 Cr Y1  Cb Y0 Cr Y1
            // Here we are sampling the color at Y0 (even luma).

            yuvType yuv[3];
            yuvType yuvSamples[2][2][3];

            for (unsigned int xSample = 0; xSample < samplesX; xSample++) {
                rgbToYcbcr(pYcbcrBtMatrix, pNormalizeColorRange, pYcbcrConversionInfo, rowRgbaPtr, rgbBpp, &yuvSamples[xSample][0][0]);
                rowRgbaPtr += rgbChannelsPerColor;
            }

            rowYuvPtr[cY0] = yuvSamples[0][0][0];
            rowYuvPtr[cY1] = yuvSamples[1][0][0];

            InterpoalteCbCr(pYcbcrConversionInfo, yuvSamples, samplesX, yuv);
            rowYuvPtr[cCb] = yuv[1];
            rowYuvPtr[cCr] = yuv[2];

            rowYuvPtr += 4; // 4 element in yuyv/uyvy packet.
        }
        rowRgbaStartPtr = (const rgbType *)((const uint8_t*)rowRgbaStartPtr + rgbStrideBytes);
        rowYuvStartPtr = (yuvType*)((uint8_t*)rowYuvStartPtr + layouts[0].rowPitch);
    }

    return 0;
}

// YCBCR_SEMI_PLANAR_CBCR_INTERLEAVED: SemiPlanar CbCr Interleaved
// Y plane and interleaved UV plane (2 planes) ex. NV12;
// YCBCR_PLANAR_CBCR_STRIDE_INTERLEAVED: (3)-PlanarCbCrStrideInterleaved
// Y plane and separate, side by side U and V planes (3 planes) ex. IMC2/4;
template <class rgbType, class yuvType>
static int ConvertRgbToYcbcrMultiPlanarCbCrInterleaved(const VkMpFormatInfo * mpInfo, const YcbcrBtMatrix* pYcbcrBtMatrix, const YcbcrNormalizeColorRange * pNormalizeColorRange,
        const  VkSamplerYcbcrConversionCreateInfo * pYcbcrConversionInfo,
        const rgbType *colRgbaPtr, uint32_t width, uint32_t height, uint32_t rgbStrideBytes, uint32_t rgbChannelsPerColor, uint32_t rgbBpp, yuvType *yuvPtr,
        VkSubresourceLayout layouts[3], uint32_t skipChannelsMask)
{
    assert((mpInfo->planesLayout.layout == YCBCR_SINGLE_PLANE_UNNORMALIZED)   || \
           (mpInfo->planesLayout.layout == YCBCR_SEMI_PLANAR_CBCR_INTERLEAVED)   || \
           (mpInfo->planesLayout.layout == YCBCR_PLANAR_CBCR_STRIDE_INTERLEAVED) ||
           (mpInfo->planesLayout.layout == YCBCR_PLANAR_STRIDE_PADDED));

    if (mpInfo->planesLayout.layout == YCBCR_SINGLE_PLANE_UNNORMALIZED) {
        assert(skipChannelsMask);
    }

    const uint32_t enabledChannelsMask = ~skipChannelsMask;

    const bool threePlane = (mpInfo->planesLayout.numberOfExtraPlanes == 2);

    yuvType *colYPtr = yuvPtr + layouts[0].offset;
    yuvType *colCbPtr = (yuvType *)((uint8_t*)yuvPtr + layouts[1].offset);
    yuvType *colCrPtr = threePlane ? (yuvType *)((uint8_t*)yuvPtr + layouts[2].offset) : colCbPtr + 1;
    const uint32_t CbCrPtrIncr = threePlane ? 1 : 2;

    VkDeviceSize rowPitchCr = threePlane ? layouts[2].rowPitch : layouts[1].rowPitch;

    const uint32_t samplesX = mpInfo->planesLayout.secondaryPlaneSubsampledX ? 2 : 1;
    const uint32_t samplesY = mpInfo->planesLayout.secondaryPlaneSubsampledY ? 2 : 1;
    const uint32_t numSamples = samplesX * samplesY;

    for (unsigned int i = 0; i < height; i += samplesY) {
        const rgbType *rowRgbaPtr = colRgbaPtr;
        yuvType *rowYPtr  =  colYPtr;
        yuvType *rowCbPtr =  colCbPtr;
        yuvType *rowCrPtr =  colCrPtr;
        for (unsigned int j = 0; j < width; j += samplesX) {

            yuvType yuv[3];
            yuvType yuvSamples[2][2][3];

            for (unsigned int ySample = 0; ySample < samplesY; ySample++) {
                const rgbType *sampleRgbaPtr = (const rgbType*)((const uint8_t*)rowRgbaPtr + (ySample * rgbStrideBytes));
                yuvType *sampleYPtr = (yuvType*)((uint8_t*)rowYPtr + (ySample * layouts[0].rowPitch));
                for (unsigned int xSample = 0; xSample < samplesX; xSample++) {
                    rgbToYcbcr(pYcbcrBtMatrix, pNormalizeColorRange, pYcbcrConversionInfo, sampleRgbaPtr, rgbBpp, &yuvSamples[xSample][ySample][0]);
                    if (enabledChannelsMask & (1 << 0)) {
                        *sampleYPtr++ = yuvSamples[xSample][ySample][0];
                    }
                    sampleRgbaPtr += rgbChannelsPerColor;
                }
            }

            rowRgbaPtr += samplesX * rgbChannelsPerColor;
            rowYPtr    += samplesX;

            InterpoalteCbCr(pYcbcrConversionInfo, yuvSamples, numSamples, yuv);

            if (enabledChannelsMask & (1 << 1)) {
                *rowCbPtr = yuv[1];
            }
            rowCbPtr += CbCrPtrIncr;

            if (enabledChannelsMask & (1 << 2)) {
                *rowCrPtr = yuv[2];
            }
            rowCrPtr += CbCrPtrIncr;
        }
        colRgbaPtr = (const rgbType *)((const uint8_t*)colRgbaPtr + (samplesY * rgbStrideBytes));
        colYPtr =  (yuvType*)((uint8_t*)colYPtr  + (samplesY * layouts[0].rowPitch));
        colCbPtr = (yuvType*)((uint8_t*)colCbPtr + layouts[1].rowPitch);
        colCrPtr = (yuvType*)((uint8_t*)colCrPtr + rowPitchCr);
    }

    return 0;
}

int ConvertRgbToYcbcr(const VkSamplerYcbcrConversionCreateInfo * pYcbcrConversionInfo, YcbcrLevelsRange levelRange,
                      const void * colRgbaPtr, uint32_t width, uint32_t height, uint32_t rgbStrideBytes,
                      uint32_t rgbChannelsPerColor, uint32_t rgbBpp,
                      void *yuvData, VkSubresourceLayout layouts[3], uint32_t skipChannelsMask)
{
    const VkMpFormatInfo * mpInfo = YcbcrVkFormatInfo(pYcbcrConversionInfo->format);
    const unsigned int bpp = (8 + mpInfo->planesLayout.bpp * 2);
    YcbcrNormalizeColorRange yCbCrNormalizeColorRange(bpp,
            (pYcbcrConversionInfo->ycbcrModel == VK_SAMPLER_YCBCR_MODEL_CONVERSION_RGB_IDENTITY) ?
                    YCBCR_COLOR_RANGE_NATURAL : (YCBCR_COLOR_RANGE)pYcbcrConversionInfo->ycbcrRange,
                    false, false);

    YcbcrBtStandard btStandard = GetYcbcrPrimariesConstantsId(pYcbcrConversionInfo->ycbcrModel);

    if (btStandard == YcbcrBtStandardUnknown) {
        // return -1;
    }

    const YcbcrPrimariesConstants primariesConstants = GetYcbcrPrimariesConstants(btStandard);
    const YcbcrRangeConstants rangeConstants = GetYcbcrRangeConstants(levelRange);
    const YcbcrBtMatrix YcbcrMatrix(primariesConstants.kb,
                                    primariesConstants.kr,
                                    rangeConstants.cbMax,
                                    rangeConstants.crMax);


    const YcbcrBtMatrix* pYcbcrMatrix = (btStandard == YcbcrBtStandardUnknown) ? NULL : &YcbcrMatrix;

    if (mpInfo->planesLayout.bpp) { // 16-bit
        uint16_t* yuvPtr = (uint16_t*)yuvData;
        const uint16_t* rgbPtr = (const uint16_t*)colRgbaPtr;

        if (mpInfo->planesLayout.layout == YCBCR_SINGLE_PLANE_INTERLEAVED) {
            return ConvertRgbToYcbcrSinglePlaneInterleavedLayout(mpInfo, pYcbcrMatrix, &yCbCrNormalizeColorRange, pYcbcrConversionInfo,
                    rgbPtr, width, height, rgbStrideBytes, rgbChannelsPerColor, rgbBpp,
                    yuvPtr, layouts, skipChannelsMask);
        } else {
            return ConvertRgbToYcbcrMultiPlanarCbCrInterleaved(mpInfo, pYcbcrMatrix, &yCbCrNormalizeColorRange, pYcbcrConversionInfo,
                    rgbPtr, width, height, rgbStrideBytes, rgbChannelsPerColor, rgbBpp,
                    yuvPtr, layouts, skipChannelsMask);
        }
    } else { // 8-bit
        uint8_t* yuvPtr = (uint8_t*)yuvData;
        const uint8_t* rgbPtr = (const uint8_t*)colRgbaPtr;

        if (mpInfo->planesLayout.layout == YCBCR_SINGLE_PLANE_INTERLEAVED) {
            return ConvertRgbToYcbcrSinglePlaneInterleavedLayout(mpInfo, pYcbcrMatrix, &yCbCrNormalizeColorRange, pYcbcrConversionInfo,
                    rgbPtr, width, height, rgbStrideBytes, rgbChannelsPerColor, rgbBpp,
                    yuvPtr, layouts, skipChannelsMask);
        } else {
            return ConvertRgbToYcbcrMultiPlanarCbCrInterleaved(mpInfo, pYcbcrMatrix, &yCbCrNormalizeColorRange, pYcbcrConversionInfo,
                    rgbPtr, width, height, rgbStrideBytes, rgbChannelsPerColor, rgbBpp,
                    yuvPtr, layouts, skipChannelsMask);
        }
    }
    return -1;
}

void VkFillYuv::fillVkCommon(const ImageData *pImageData, VkSubresourceLayout layouts[3], const VkSamplerYcbcrConversionCreateInfo* pSamplerYcbcrConversionCreateInfo,
                             const VkMpFormatInfo *mpInfo, uint8_t *ptr, VkDeviceSize, VkImageAspectFlags aspectMask, VkFormat aspectMainFormat)
{
    VkDeviceSize offset = 0;
    VkDeviceSize rgbaPitch = 0;
    unsigned int rgbaChannelsPerColor = 4; // default 4
    unsigned int rgbBitsPerColor = 8;      // default 8

    VkFormat imageFormat = pSamplerYcbcrConversionCreateInfo->format;
    VkFormat rgbVkFormat = imageFormat;
    int32_t isPlaneAspect = (uint32_t)aspectMask & uint32_t(VK_IMAGE_ASPECT_PLANE_0_BIT | VK_IMAGE_ASPECT_PLANE_1_BIT | VK_IMAGE_ASPECT_PLANE_2_BIT);
    const VkMpFormatInfo *mpAspectInfo =  (isPlaneAspect && (aspectMainFormat != VK_FORMAT_UNDEFINED)) ? YcbcrVkFormatInfo(aspectMainFormat) : NULL;

    uint32_t imageWidth  = pImageData->width;
    uint32_t imageHeight = pImageData->height;

    if (mpAspectInfo && (aspectMask & uint32_t(VK_IMAGE_ASPECT_PLANE_1_BIT | VK_IMAGE_ASPECT_PLANE_2_BIT))) {
        if (mpAspectInfo->planesLayout.secondaryPlaneSubsampledX) {
            imageWidth *= 2;
        }

        if (mpAspectInfo->planesLayout.secondaryPlaneSubsampledY) {
            imageHeight *= 2;
        }
    }

    bool isUnnormalizedRgba = false;
    if (mpInfo && (mpInfo->planesLayout.layout == YCBCR_SINGLE_PLANE_UNNORMALIZED) && !(mpInfo->planesLayout.disjoint)) {
        isUnnormalizedRgba = true;
        rgbVkFormat = mpInfo->vkPlaneFormat[0];
    }

    uint8_t* rgbImageData = NULL;
    const unsigned int rgbMaxSize   = (unsigned int)(imageWidth * imageHeight * 4 * sizeof(uint16_t));
    if (mpInfo && !isUnnormalizedRgba) {

        if (!rgbImageData) {
            rgbImageData = new uint8_t[rgbMaxSize];
        }
        ABORT_IF_TRUE(rgbImageData == NULL);

        memset(rgbImageData, 0x00, rgbMaxSize);
        rgbVkFormat = mpInfo->planesLayout.bpp ? VK_FORMAT_R16G16B16A16_UNORM : VK_FORMAT_R8G8B8A8_UNORM;

    } else {
        offset    = layouts[0].offset;
        rgbaPitch = layouts[0].rowPitch;
    }

    const VkFormatDesc* pRgbFormatDesc = vkFormatLookUp(rgbVkFormat);
    ABORT_IF_TRUE(pRgbFormatDesc == NULL);

    rgbaChannelsPerColor = pRgbFormatDesc->numberOfChannels;
    rgbBitsPerColor = (pRgbFormatDesc->numberOfBytes / pRgbFormatDesc->numberOfChannels) * 8;

    uint8_t *rgbaColPtr = NULL;
    if (mpInfo && !isUnnormalizedRgba) {
        rgbaColPtr = rgbImageData;
        rgbaPitch = imageWidth * rgbaChannelsPerColor * ((rgbBitsPerColor == 16) ? sizeof(uint16_t) : sizeof(uint8_t));
    } else {
        rgbaColPtr = ptr + offset;
    }

    const uint16_t maxC = (uint16_t)((1 << rgbBitsPerColor) - 1);
    const uint16_t minC = 0;
    const uint16_t alphaMax = (uint16_t)((1 << rgbBitsPerColor) - 1);

    ColorPattern patttern = pImageData->patttern;
    unsigned int rgbaSkipChannelsMask = 0;
    bool incOnSkip = rgbaSkipChannelsMask ? true : false;
    if (rgbBitsPerColor == 8) {
        uint8_t      clearColor[4];

        for (unsigned int i = 0; i < 4; i++ ) {
            clearColor[i] = (uint8_t)pImageData->clearColor[i];
        }

        generateColorPatternRgba8888(patttern, (uint8_t *)rgbaColPtr, (uint32_t)imageWidth, (uint32_t)imageHeight,
                                     (uint32_t)rgbaPitch,
                                     rgbaChannelsPerColor, (uint8_t)maxC, (uint8_t)minC, (uint8_t)alphaMax, clearColor, rgbaSkipChannelsMask, incOnSkip);
    } else {
        assert(rgbBitsPerColor == 16);
        uint16_t  clearColor[4];

        for (unsigned int i = 0; i < 4; i++ ) {
            clearColor[i] = (uint16_t)(pImageData->clearColor[i] * 256);
        }

        generateColorPatternRgba16161616(patttern, (uint16_t *)rgbaColPtr, (uint32_t)imageWidth, (uint32_t)imageHeight,
                                         (uint32_t)rgbaPitch,
                                         rgbaChannelsPerColor, maxC, minC, alphaMax, clearColor, rgbaSkipChannelsMask, incOnSkip);
    }


    if (mpInfo && !isUnnormalizedRgba) {

        VkSamplerYcbcrConversionCreateInfo samplerColorConversionCreateInfo = VkSamplerYcbcrConversionCreateInfo();
        memcpy(&samplerColorConversionCreateInfo, pSamplerYcbcrConversionCreateInfo, sizeof(VkSamplerYcbcrConversionCreateInfo));
        samplerColorConversionCreateInfo.format = (isPlaneAspect && (aspectMainFormat != VK_FORMAT_UNDEFINED)) ? aspectMainFormat : imageFormat;

        if (mpAspectInfo == NULL)
            mpAspectInfo = mpInfo;

        unsigned int yCbCrSkipChannelsMask = 0;
        switch (aspectMask) {
        case VK_IMAGE_ASPECT_PLANE_0_BIT:
            // Select the first plane only.
            yCbCrSkipChannelsMask |= ((1 << 2) | (1 << 1));
            break;
        case VK_IMAGE_ASPECT_PLANE_1_BIT:
            // Select the second plane only.

            if (mpAspectInfo->planesLayout.numberOfExtraPlanes == 2) {
                yCbCrSkipChannelsMask |= ((1 << 2) | (1 << 0));
            } else if (mpAspectInfo->planesLayout.numberOfExtraPlanes == 1) {
                yCbCrSkipChannelsMask |= (1 << 0);
            } else {
                assert(0);
            }
            if (aspectMainFormat != VK_FORMAT_UNDEFINED) {
                layouts[1] = layouts[0];
            }
            break;
        case VK_IMAGE_ASPECT_PLANE_2_BIT:
            // Select the third plane only.
            assert(mpAspectInfo->planesLayout.numberOfExtraPlanes == 2);
            yCbCrSkipChannelsMask |= ((1 << 1) | (1 << 0));
            if (aspectMainFormat != VK_FORMAT_UNDEFINED) {
                layouts[2] = layouts[1] = layouts[0];
            }
            break;
        default:
            ;
        }

        ConvertRgbToYcbcr(&samplerColorConversionCreateInfo, YcbcrLevelsDigital, rgbaColPtr, imageWidth, imageHeight,
                          (uint32_t)rgbaPitch, rgbaChannelsPerColor, rgbBitsPerColor,
                          ptr, layouts, yCbCrSkipChannelsMask);

    }

    if (rgbImageData) {
        delete [] rgbImageData;
    }
}

// Initialize the texture data, either directly into the texture itself
// or into buffer memory.
void VkFillYuv::fillVkImage(VkImage vkImage, const ImageData *pImageData,
                            VkDeviceMemory mem, uint8_t *mappedHostPtr,
                            const VkSamplerYcbcrConversionCreateInfo* pSamplerYcbcrConversionCreateInfo,
                            VkImageAspectFlags aspectMask, VkFormat aspectMainFormat)
{
    VkResult result;

    VkImageSubresource subResource = {};
    VkSubresourceLayout layouts[3];
    VkDeviceSize size   = 0;

    // Clean it
    memset(layouts, 0x00, sizeof(layouts));

    VkFormat imageFormat = pSamplerYcbcrConversionCreateInfo->format;
    const VkMpFormatInfo *mpInfo =  YcbcrVkFormatInfo(imageFormat);
    bool isUnnormalizedRgba = false;
    if (mpInfo && (mpInfo->planesLayout.layout == YCBCR_SINGLE_PLANE_UNNORMALIZED) && !(mpInfo->planesLayout.disjoint)) {
        isUnnormalizedRgba = true;
    }

    if (mpInfo && !isUnnormalizedRgba) {
        VkMemoryRequirements memReqs = { };
        VulkanDeviceContext::GetThe()->GetImageMemoryRequirements(VulkanDeviceContext::GetThe()->getDevice(), vkImage, &memReqs);
        size      = memReqs.size;
        // alignment = memReqs.alignment;
        switch (mpInfo->planesLayout.layout) {
        case YCBCR_SINGLE_PLANE_UNNORMALIZED:
        case YCBCR_SINGLE_PLANE_INTERLEAVED:
            subResource.aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT;
            VulkanDeviceContext::GetThe()->GetImageSubresourceLayout(VulkanDeviceContext::GetThe()->getDevice(), vkImage, &subResource, &layouts[0]);
            break;
        case YCBCR_SEMI_PLANAR_CBCR_INTERLEAVED:
            subResource.aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT;
            VulkanDeviceContext::GetThe()->GetImageSubresourceLayout(VulkanDeviceContext::GetThe()->getDevice(), vkImage, &subResource, &layouts[0]);
            subResource.aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT;
            VulkanDeviceContext::GetThe()->GetImageSubresourceLayout(VulkanDeviceContext::GetThe()->getDevice(), vkImage, &subResource, &layouts[1]);
            break;
        case YCBCR_PLANAR_CBCR_STRIDE_INTERLEAVED:
        case YCBCR_PLANAR_CBCR_BLOCK_JOINED:
        case YCBCR_PLANAR_STRIDE_PADDED:
            subResource.aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT;
            VulkanDeviceContext::GetThe()->GetImageSubresourceLayout(VulkanDeviceContext::GetThe()->getDevice(), vkImage, &subResource, &layouts[0]);
            subResource.aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT;
            VulkanDeviceContext::GetThe()->GetImageSubresourceLayout(VulkanDeviceContext::GetThe()->getDevice(), vkImage, &subResource, &layouts[1]);
            subResource.aspectMask = VK_IMAGE_ASPECT_PLANE_2_BIT;
            VulkanDeviceContext::GetThe()->GetImageSubresourceLayout(VulkanDeviceContext::GetThe()->getDevice(), vkImage, &subResource, &layouts[2]);
            break;
        default:
            assert(0);
        }

    } else {

        VulkanDeviceContext::GetThe()->GetImageSubresourceLayout(VulkanDeviceContext::GetThe()->getDevice(), vkImage, &subResource, &layouts[0]);
        size = layouts[0].size;
    }

    fillVkCommon(pImageData, layouts, pSamplerYcbcrConversionCreateInfo, mpInfo, mappedHostPtr, size, aspectMask, aspectMainFormat);

    const VkMappedMemoryRange   range           = {
        VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,  // sType
        NULL,                                   // pNext
        mem,                                    // memory
        0,                                      // offset
        size,                                   // size
    };

    result = VulkanDeviceContext::GetThe()->FlushMappedMemoryRanges(VulkanDeviceContext::GetThe()->getDevice(), 1u, &range);
    ABORT_IF_TRUE(result != VK_SUCCESS);
}

} // namespace Pattern
