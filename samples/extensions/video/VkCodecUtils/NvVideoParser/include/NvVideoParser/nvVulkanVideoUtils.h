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

#ifndef _NVVULKANVIDEOUTILS_H_
#define _NVVULKANVIDEOUTILS_H_

#include <algorithm>
#include <limits.h>

#define ALIGN16(p) ((unsigned char*)((((size_t)(p)) + 15) & ~15))
#define ALIGN32(p) ((unsigned char*)((((size_t)(p)) + 31) & ~31))
#define ALIGN64(p) ((unsigned char*)((((size_t)(p)) + 63) & ~63))
#define ALIGN128(p) ((unsigned char*)((((size_t)(p)) + 127) & ~127))

#ifndef ARRAYSIZE
#define ARRAYSIZE(a) ((sizeof(a) / sizeof(a[0])))
#endif

#define MAKEFRAMERATE(num, den) (((num) << 14) | (den))
#define NV_FRAME_RATE_NUM(rate) ((rate) >> 14)
#define NV_FRAME_RATE_DEN(rate) ((rate)&0x3fff)

enum {
    NV_FRAME_RATE_12 = MAKEFRAMERATE(12000, 1000),
    NV_FRAME_RATE_12_5 = MAKEFRAMERATE(12500, 1000),
    NV_FRAME_RATE_14_98 = MAKEFRAMERATE(15000, 1001),
    NV_FRAME_RATE_15 = MAKEFRAMERATE(15000, 1000),
    NV_FRAME_RATE_23_97 = MAKEFRAMERATE(24000, 1001),
    NV_FRAME_RATE_24 = MAKEFRAMERATE(24000, 1000),
    NV_FRAME_RATE_25 = MAKEFRAMERATE(25000, 1000),
    NV_FRAME_RATE_29_97 = MAKEFRAMERATE(30000, 1001),
    NV_FRAME_RATE_30 = MAKEFRAMERATE(30000, 1000),
    NV_FRAME_RATE_50 = MAKEFRAMERATE(50000, 1000),
    NV_FRAME_RATE_59_94 = MAKEFRAMERATE(60000, 1001),
    NV_FRAME_RATE_60 = MAKEFRAMERATE(60000, 1000),
    NV_FRAME_RATE_UNKNOWN = 0 // Unknown/unspecified frame rate (or variable)
};

static inline int32_t Log2U31(int32_t n)
{
    assert(n >= 0);
    int32_t sz = 0;
    while (n) {
        sz++;
        n >>= 1;
    }
    return sz;
}

static inline FrameRate PackFrameRate(uint32_t numerator, uint32_t denominator)
{
    while ((numerator >= (1 << 18)) || (denominator >= (1 << 14))) {
        if (!(numerator % 5) && !(denominator % 5)) {
            numerator /= 5;
            denominator /= 5;
        } else if (((numerator | denominator) & 1) && !(numerator % 3) && !(denominator % 3)) {
            numerator /= 3;
            denominator /= 3;
        } else {
            numerator = (numerator + 1) >> 1;
            denominator = (denominator + 1) >> 1;
        }
    }
    return MAKEFRAMERATE(numerator, denominator);
}

// Simplify an aspect ratio fraction (both inputs must be positive)
static inline void SimplifyAspectRatio(int32_t* pARWidth, int32_t* pARHeight)
{
    uint32_t a = abs(*pARWidth), b = abs(*pARHeight);
    while (a) {
        uint32_t tmp = a;
        a = b % tmp;
        b = tmp;
    }
    if (b) {
        *pARWidth /= (int32_t)b;
        *pARHeight /= (int32_t)b;
    }
}

#endif /* _NVVULKANVIDEOUTILS_H_ */
