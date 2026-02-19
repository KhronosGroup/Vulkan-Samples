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

#ifndef YCBCRINFOTBL_H_
#define YCBCRINFOTBL_H_

// Start of plane byte alignment.
#ifndef PLATFORM_YCBCR_PLANES_BYTE_ALIGN
  #define PLATFORM_YCBCR_PLANES_BYTE_ALIGN 32
#endif

// Pitch byte alignment.
#ifndef PLATFORM_YCBCR_PLANES_PITCH_ALIGN
  #define PLATFORM_YCBCR_PLANES_PITCH_ALIGN PLATFORM_YCBCR_PLANES_BYTE_ALIGN
#endif

// Alignment between the planes byte alignment.
#ifndef PLATFORM_YCBCR_PLANES_PLANE_ALIGN
  #define PLATFORM_YCBCR_PLANES_PLANE_ALIGN PLATFORM_YCBCR_PLANES_BYTE_ALIGN
#endif

//                                   byteAlign                              bytePitchAlign                        bytePlaneAlign               reserved
#ifndef YCBCR_ALIGN_PLANES
#define YCBCR_ALIGN_PLANES()    PLATFORM_YCBCR_PLANES_BYTE_ALIGN,  PLATFORM_YCBCR_PLANES_PITCH_ALIGN,   PLATFORM_YCBCR_PLANES_PLANE_ALIGN,        0
#endif // YCBCR_ALIGN_PLANES

#ifndef YCBCR_BASE_FORMAT_SP
#define YCBCR_BASE_FORMAT_SP(vkfmt0)
#endif // YCBCR_BASE_FORMAT_SP
#ifndef YCBCR_BASE_FORMAT_MP
#define YCBCR_BASE_FORMAT_MP(vkfmt0)
#endif // YCBCR_BASE_FORMAT_MP
#ifndef YCBCR_BASE_FORMAT_UNSUPPORTED
#define YCBCR_BASE_FORMAT_UNSUPPORTED()
#endif // YCBCR_BASE_FORMAT_UNSUPPORTED

// YCbCr memory layouts  __VkFormatInfo                              Planes Memory layout               dis bpp   xSS   ySS  numP  ch0 ch1 ch2 ch3         Plane alignment parameters             Per plane VK format.
#define YCBCR_BASE_PLANE_UNNORMALIZED_LAYOUT(        vkfrmt, vkfmt0, bpp,           c0,  vkfmt3) {\
                         YCBCR_BASE_FORMAT_SP(vkfmt0) vkfrmt, {{{ YCBCR_SINGLE_PLANE_UNNORMALIZED,     1,  bpp, false, false, 0,  c0, CC_UN, CC_UN, CC_UN}},  YCBCR_ALIGN_PLANES() },     {vkfmt0,   VK_FORMAT_UNDEFINED,  VK_FORMAT_UNDEFINED, vkfmt3 } }
#define YCBCR_SINGLE_PLANE_UNNORMALIZED_LAYOUT(      vkfrmt, vkfmt0, bpp,           c0, c1, c2, c3        ) {\
                         YCBCR_BASE_FORMAT_SP(vkfmt0) vkfrmt, {{{ YCBCR_SINGLE_PLANE_UNNORMALIZED,     0,  bpp, false, false, 0,   c0, c1, c2, c3}},          YCBCR_ALIGN_PLANES() },      {vkfmt0,   VK_FORMAT_UNDEFINED,  VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED } }
#define YCBCR_SINGLE_PLANE_INTERLEAVED_LAYOUT(      vkfrmt, vkfmt0, bpp,           c0, c1, c2, c3, vkfmt1) {\
                         YCBCR_BASE_FORMAT_SP(vkfmt0) vkfrmt, {{{ YCBCR_SINGLE_PLANE_INTERLEAVED,      0,  bpp, true,  false, 1,   c0, c1, c2, c3}},          YCBCR_ALIGN_PLANES() },      {vkfmt0,   vkfmt1,  VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED } }
#define YCBCR_SEMI_PLANAR_CBCR_INTERLEAVED_LAYOUT(  vkfrmt, vkfmt0, bpp, xSS, ySS, p0, p1, vkfmt1    ) {\
                         YCBCR_BASE_FORMAT_MP(vkfmt0) vkfrmt, {{{ YCBCR_SEMI_PLANAR_CBCR_INTERLEAVED,   0,  bpp, xSS,   ySS,   1,   p0, p1, CC_UN, CC_UN}},   YCBCR_ALIGN_PLANES() },      {vkfmt0,   vkfmt1,  VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED } }
#define YCBCR_PLANAR_CBCR_STRIDE_INTERLEAVED_LAYOUT(vkfrmt, vkfmt0, bpp, xSS, ySS, p0, p1, p2, vkfmt1, vkfmt2) {\
                         YCBCR_BASE_FORMAT_MP(vkfmt0) vkfrmt, {{{ YCBCR_PLANAR_CBCR_STRIDE_INTERLEAVED, 0,  bpp, xSS,   ySS,   2,   p0, p1, p2, CC_UN}},      YCBCR_ALIGN_PLANES() },      {vkfmt0,   vkfmt1,  vkfmt2, VK_FORMAT_UNDEFINED } }
#define YCBCR_PLANAR_STRIDE_PADDED_LAYOUT(          vkfrmt, vkfmt0, bpp, xSS, ySS, p0, p1, p2, vkfmt1, vkfmt2) {\
                         YCBCR_BASE_FORMAT_MP(vkfmt0) vkfrmt, {{{ YCBCR_PLANAR_STRIDE_PADDED,           0,  bpp, xSS,   ySS,   2,   p0, p1, p2, CC_UN}},      YCBCR_ALIGN_PLANES() },      {vkfmt0,   vkfmt1,  vkfmt2, VK_FORMAT_UNDEFINED } }
#define YCBCR_UNSUPPORTED_LAYOUT(vkfrmt) {\
                         YCBCR_BASE_FORMAT_UNSUPPORTED() vkfrmt, {{{ YCBCR_SINGLE_PLANE_INTERLEAVED, 0,  0,  false, false, 0,   CC_UN, CC_UN, CC_UN, CC_UN}}, 0, 0, 0, 0    },      {VK_FORMAT_UNDEFINED,   VK_FORMAT_UNDEFINED,  VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED } }

// Multi-planar format info
static const VkMpFormatInfo vkMpFormatInfo[] = {
//          Planes Memory layout                 vkColor Format                                            vkformat(0)            bpp        xSS   ySS    ch0   ch1    ch2    ch3         vkformat(1)      vkformat(2)
//  ############################################### 8-bit formats #########################################################################################################################################################################
    YCBCR_SINGLE_PLANE_INTERLEAVED_LAYOUT(VK_FORMAT_G8B8G8R8_422_UNORM,                                 VK_FORMAT_R8G8_UNORM, YCBCRA_8BPP,                CC_Y0, CC_CB, CC_Y1, CC_CR,  VK_FORMAT_R8G8B8A8_UNORM),
    YCBCR_SINGLE_PLANE_INTERLEAVED_LAYOUT(VK_FORMAT_B8G8R8G8_422_UNORM,                                 VK_FORMAT_R8G8_UNORM, YCBCRA_8BPP,                CC_CB, CC_Y0, CC_CR, CC_Y1,  VK_FORMAT_R8G8B8A8_UNORM),
    YCBCR_PLANAR_STRIDE_PADDED_LAYOUT(VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM,                              VK_FORMAT_R8_UNORM, YCBCRA_8BPP,   true,  true,   CC_Y , CC_CB, CC_CR,         VK_FORMAT_R8_UNORM, VK_FORMAT_R8_UNORM),
    YCBCR_SEMI_PLANAR_CBCR_INTERLEAVED_LAYOUT(VK_FORMAT_G8_B8R8_2PLANE_420_UNORM,                       VK_FORMAT_R8_UNORM, YCBCRA_8BPP,   true,  true,   CC_Y , CC_CBCR,              VK_FORMAT_R8G8_UNORM),
    YCBCR_PLANAR_STRIDE_PADDED_LAYOUT(VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM,                              VK_FORMAT_R8_UNORM, YCBCRA_8BPP,   true,  false,  CC_Y , CC_CB, CC_CR,         VK_FORMAT_R8_UNORM,  VK_FORMAT_R8_UNORM),
    YCBCR_SEMI_PLANAR_CBCR_INTERLEAVED_LAYOUT(VK_FORMAT_G8_B8R8_2PLANE_422_UNORM,                       VK_FORMAT_R8_UNORM, YCBCRA_8BPP,   true,  false,  CC_Y , CC_CBCR,              VK_FORMAT_R8G8_UNORM),
    YCBCR_PLANAR_STRIDE_PADDED_LAYOUT(VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM,                              VK_FORMAT_R8_UNORM, YCBCRA_8BPP,   false, false,  CC_Y , CC_CB, CC_CR,         VK_FORMAT_R8_UNORM,  VK_FORMAT_R8_UNORM),
//  ############################################### 10-bit formats ########################################################################################################################################################################
    YCBCR_BASE_PLANE_UNNORMALIZED_LAYOUT(VK_FORMAT_R10X6_UNORM_PACK16,                                  VK_FORMAT_R16_UNORM,    YCBCRA_10BPP,             CC_Y,                        VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16),
    YCBCR_BASE_PLANE_UNNORMALIZED_LAYOUT(VK_FORMAT_R10X6G10X6_UNORM_2PACK16,                            VK_FORMAT_R16G16_UNORM, YCBCRA_10BPP,             CC_CBCR,                     VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16),
    YCBCR_SINGLE_PLANE_UNNORMALIZED_LAYOUT(VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16,                VK_FORMAT_R16G16B16A16_UNORM, YCBCRA_10BPP,       CC_UN, CC_UN, CC_UN, CC_UN),
    YCBCR_SINGLE_PLANE_INTERLEAVED_LAYOUT(VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16,             VK_FORMAT_R16G16_UNORM, YCBCRA_10BPP,             CC_Y0, CC_CB, CC_Y1, CC_CR,  VK_FORMAT_R16G16B16A16_UNORM),
    YCBCR_SINGLE_PLANE_INTERLEAVED_LAYOUT(VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16,             VK_FORMAT_R16G16_UNORM, YCBCRA_10BPP,             CC_CB, CC_Y0, CC_CR, CC_Y1,  VK_FORMAT_R16G16B16A16_UNORM),
    YCBCR_PLANAR_STRIDE_PADDED_LAYOUT(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16,             VK_FORMAT_R16_UNORM, YCBCRA_10BPP, true,  true,   CC_Y , CC_CB, CC_CR,         VK_FORMAT_R16_UNORM, VK_FORMAT_R16_UNORM),
    YCBCR_SEMI_PLANAR_CBCR_INTERLEAVED_LAYOUT(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16,      VK_FORMAT_R16_UNORM, YCBCRA_10BPP, true,  true,   CC_Y , CC_CBCR,              VK_FORMAT_R16G16_UNORM),
    YCBCR_PLANAR_STRIDE_PADDED_LAYOUT(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16,             VK_FORMAT_R16_UNORM, YCBCRA_10BPP, true,  false,  CC_Y , CC_CB, CC_CR,         VK_FORMAT_R16_UNORM, VK_FORMAT_R16_UNORM),
    YCBCR_SEMI_PLANAR_CBCR_INTERLEAVED_LAYOUT(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16,      VK_FORMAT_R16_UNORM, YCBCRA_10BPP, true,  false,  CC_Y , CC_CBCR,              VK_FORMAT_R16G16_UNORM),
    YCBCR_PLANAR_STRIDE_PADDED_LAYOUT(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16,             VK_FORMAT_R16_UNORM, YCBCRA_10BPP, false, false,  CC_Y , CC_CB, CC_CR,         VK_FORMAT_R16_UNORM, VK_FORMAT_R16_UNORM),
//  ############################################### 12-bit formats ########################################################################################################################################################################
    YCBCR_BASE_PLANE_UNNORMALIZED_LAYOUT(VK_FORMAT_R12X4_UNORM_PACK16,                                  VK_FORMAT_R16_UNORM,    YCBCRA_12BPP,             CC_Y,                        VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16),
    YCBCR_BASE_PLANE_UNNORMALIZED_LAYOUT(VK_FORMAT_R12X4G12X4_UNORM_2PACK16,                            VK_FORMAT_R16G16_UNORM, YCBCRA_12BPP,             CC_CBCR,                     VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16),
    YCBCR_SINGLE_PLANE_UNNORMALIZED_LAYOUT(VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16,                VK_FORMAT_R16G16B16A16_UNORM, YCBCRA_12BPP,       CC_UN, CC_UN, CC_UN, CC_UN),
    YCBCR_SINGLE_PLANE_INTERLEAVED_LAYOUT(VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16,             VK_FORMAT_R16G16_UNORM, YCBCRA_12BPP,             CC_Y0, CC_CB, CC_Y1, CC_CR,  VK_FORMAT_R16G16B16A16_UNORM),
    YCBCR_SINGLE_PLANE_INTERLEAVED_LAYOUT(VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16,             VK_FORMAT_R16G16_UNORM, YCBCRA_12BPP,             CC_CB, CC_Y0, CC_CR, CC_Y1,  VK_FORMAT_R16G16B16A16_UNORM),
    YCBCR_PLANAR_STRIDE_PADDED_LAYOUT(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16,             VK_FORMAT_R16_UNORM, YCBCRA_12BPP, true,  true,   CC_Y , CC_CB, CC_CR,         VK_FORMAT_R16_UNORM, VK_FORMAT_R16_UNORM),
    YCBCR_SEMI_PLANAR_CBCR_INTERLEAVED_LAYOUT(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16,      VK_FORMAT_R16_UNORM, YCBCRA_12BPP, true,  true,   CC_Y , CC_CBCR,              VK_FORMAT_R16G16_UNORM),
    YCBCR_PLANAR_STRIDE_PADDED_LAYOUT(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16,             VK_FORMAT_R16_UNORM, YCBCRA_12BPP, true,  false,  CC_Y , CC_CB, CC_CR,         VK_FORMAT_R16_UNORM, VK_FORMAT_R16_UNORM),
    YCBCR_SEMI_PLANAR_CBCR_INTERLEAVED_LAYOUT(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16,      VK_FORMAT_R16_UNORM, YCBCRA_12BPP, true,  false,  CC_Y , CC_CBCR,              VK_FORMAT_R16G16_UNORM),
    YCBCR_PLANAR_STRIDE_PADDED_LAYOUT(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16,             VK_FORMAT_R16_UNORM, YCBCRA_12BPP, false, false,  CC_Y , CC_CB, CC_CR,         VK_FORMAT_R16_UNORM, VK_FORMAT_R16_UNORM),
//  ############################################### 16-bit formats ########################################################################################################################################################################
    YCBCR_SINGLE_PLANE_INTERLEAVED_LAYOUT(VK_FORMAT_G16B16G16R16_422_UNORM,                             VK_FORMAT_R16G16_UNORM, YCBCRA_16BPP,             CC_Y0, CC_CB, CC_Y1, CC_CR,  VK_FORMAT_R16G16B16A16_UNORM),
    YCBCR_SINGLE_PLANE_INTERLEAVED_LAYOUT(VK_FORMAT_B16G16R16G16_422_UNORM,                             VK_FORMAT_R16G16_UNORM, YCBCRA_16BPP,             CC_CB, CC_Y0, CC_CR, CC_Y1,  VK_FORMAT_R16G16B16A16_UNORM),
    YCBCR_PLANAR_STRIDE_PADDED_LAYOUT(VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM,                           VK_FORMAT_R16_UNORM, YCBCRA_16BPP, true,  true,   CC_Y , CC_CB, CC_CR,         VK_FORMAT_R16_UNORM, VK_FORMAT_R16_UNORM),
    YCBCR_SEMI_PLANAR_CBCR_INTERLEAVED_LAYOUT(VK_FORMAT_G16_B16R16_2PLANE_420_UNORM,                    VK_FORMAT_R16_UNORM, YCBCRA_16BPP, true,  true,   CC_Y , CC_CBCR,              VK_FORMAT_R16G16_UNORM),
    YCBCR_PLANAR_STRIDE_PADDED_LAYOUT(VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM,                           VK_FORMAT_R16_UNORM, YCBCRA_16BPP, true,  false,  CC_Y , CC_CB, CC_CR,         VK_FORMAT_R16_UNORM, VK_FORMAT_R16_UNORM),
    YCBCR_SEMI_PLANAR_CBCR_INTERLEAVED_LAYOUT(VK_FORMAT_G16_B16R16_2PLANE_422_UNORM,                    VK_FORMAT_R16_UNORM, YCBCRA_16BPP, true,  false,  CC_Y , CC_CBCR,              VK_FORMAT_R16G16_UNORM),
    YCBCR_PLANAR_STRIDE_PADDED_LAYOUT(VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM,                           VK_FORMAT_R16_UNORM, YCBCRA_16BPP, false, false,  CC_Y , CC_CB, CC_CR,         VK_FORMAT_R16_UNORM, VK_FORMAT_R16_UNORM),
//  ############################################### extra 2 plane 444 formats ########################################################################################################################################################################
    YCBCR_SEMI_PLANAR_CBCR_INTERLEAVED_LAYOUT(VK_FORMAT_G8_B8R8_2PLANE_444_UNORM_EXT,                   VK_FORMAT_R8_UNORM, YCBCRA_8BPP,   false, false,  CC_Y , CC_CBCR,              VK_FORMAT_R8G8_UNORM),
    YCBCR_SEMI_PLANAR_CBCR_INTERLEAVED_LAYOUT(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16_EXT,  VK_FORMAT_R16_UNORM, YCBCRA_10BPP, false, false,  CC_Y , CC_CBCR,              VK_FORMAT_R16G16_UNORM),
    YCBCR_SEMI_PLANAR_CBCR_INTERLEAVED_LAYOUT(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16_EXT,  VK_FORMAT_R16_UNORM, YCBCRA_12BPP, false, false,  CC_Y , CC_CBCR,              VK_FORMAT_R16G16_UNORM),
    YCBCR_SEMI_PLANAR_CBCR_INTERLEAVED_LAYOUT(VK_FORMAT_G16_B16R16_2PLANE_444_UNORM_EXT,                VK_FORMAT_R16_UNORM, YCBCRA_16BPP, false, false,  CC_Y , CC_CBCR,              VK_FORMAT_R16G16_UNORM),
//  ############################################### End of formats ########################################################################################################################################################################
};

#undef YCBCR_ALIGN_PLANES
#undef YCBCR_BASE_PLANE_UNNORMALIZED_LAYOUT
#undef YCBCR_SINGLE_PLANE_UNNORMALIZED_LAYOUT
#undef YCBCR_SINGLE_PLANE_INTERLEAVED_LAYOUT
#undef YCBCR_SEMI_PLANAR_CBCR_INTERLEAVED_LAYOUT
#undef YCBCR_PLANAR_CBCR_STRIDE_INTERLEAVED_LAYOUT
#undef YCBCR_PLANAR_STRIDE_PADDED_LAYOUT
#undef YCBCR_UNSUPPORTED_LAYOUT

#define VK_YCBCR_FORMAT_BEGIN_RANGE  (VkFormat)VK_FORMAT_G8B8G8R8_422_UNORM
#define VK_YCBCR_FORMAT_END_RANGE    (VkFormat)VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM
#define VK_YCBCR_FORMAT_RANGE_SIZE   (VkFormat)(VK_YCBCR_FORMAT_END_RANGE - VK_YCBCR_FORMAT_BEGIN_RANGE + 1)

#define VK_YCBCR_FORMAT_EXT_TBL_START    VK_YCBCR_FORMAT_RANGE_SIZE
#define VK_YCBCR_FORMAT_EXT_BEGIN_RANGE  (VkFormat)VK_FORMAT_G8_B8R8_2PLANE_444_UNORM_EXT
#define VK_YCBCR_FORMAT_EXT_END_RANGE    (VkFormat)VK_FORMAT_G16_B16R16_2PLANE_444_UNORM_EXT
#define VK_YCBCR_FORMAT_EXT_RANGE_SIZE   (VkFormat)(VK_YCBCR_FORMAT_EXT_END_RANGE - VK_YCBCR_FORMAT_EXT_BEGIN_RANGE + 1)

static const VkMpFormatInfo * __ycbcrVkFormatInfo(const VkFormat format)
{
    const VkMpFormatInfo *info = NULL;

    assert((sizeof(vkMpFormatInfo)/sizeof(vkMpFormatInfo[0])) == (VK_YCBCR_FORMAT_RANGE_SIZE + VK_YCBCR_FORMAT_EXT_RANGE_SIZE));

    if ((format >= static_cast<VkFormat>(VK_YCBCR_FORMAT_BEGIN_RANGE)) &&
            (format <= static_cast<VkFormat>(VK_YCBCR_FORMAT_END_RANGE))) {

        info = &vkMpFormatInfo[format - VK_YCBCR_FORMAT_BEGIN_RANGE];

        assert(vkMpFormatInfo[format - VK_YCBCR_FORMAT_BEGIN_RANGE].vkFormat == format);
    } else if ((format >= static_cast<VkFormat>(VK_YCBCR_FORMAT_EXT_BEGIN_RANGE)) &&
            (format <= static_cast<VkFormat>(VK_YCBCR_FORMAT_EXT_END_RANGE))) {

        uint32_t extFormatIndex = VK_YCBCR_FORMAT_EXT_TBL_START + (format - VK_YCBCR_FORMAT_EXT_BEGIN_RANGE);

        info = &vkMpFormatInfo[extFormatIndex];

        assert(vkMpFormatInfo[extFormatIndex].vkFormat == format);
    }

    return info;
}

#endif /* YCBCRINFOTBL_H_ */
