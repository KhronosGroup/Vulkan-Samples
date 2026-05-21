/*
 *  Copyright 2012 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 *  Original code is derived from https://android.googlesource.com/platform/external/libyuv
*/
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


#ifndef _VKCODECUTILS_YCBCRCONVUTILSCPU_H_
#define _VKCODECUTILS_YCBCRCONVUTILSCPU_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

template <typename planeType>  // T can be uint8_t for 8-bit or uint16_t for 16-bit
class YCbCrConvUtilsCpu
{
public:

    YCbCrConvUtilsCpu()
    {
        // TODO Auto-generated constructor stub

    }

    virtual ~YCbCrConvUtilsCpu()
    {
        // TODO Auto-generated destructor stub
    }

    static void CopyRow(const planeType* src, planeType* dst, int count, int /* shiftBits */) {

        count *= (int)sizeof(planeType);
        memcpy(dst, src, count);
    }

    static void CopyRowShiftLeft(const planeType* src, planeType* dst, int count, int shiftBits) {

        for (int col = 0; col < count; col++) {
            *dst = static_cast<planeType>(*src << shiftBits);
            dst++;
            src++;
        }
    }

    static void CopyPlane(const planeType* src_y,
                          int src_stride_y,
                          planeType* dst_y,
                          int dst_stride_y,
                          int width,
                          int height,
                          int shiftBits) {
        int y;
        if ((width <= 0) || (height == 0)) {
            return;
        }

        // Negative height means invert the image.
        if (height < 0) {
            height = -height;
            dst_y = dst_y + (height - 1) * dst_stride_y;
            dst_stride_y = -dst_stride_y;
        }

        // Nothing to do.
        if ((src_y == dst_y) && (src_stride_y == dst_stride_y)) {
            return;
        }

        // Coalesce rows.
        if ((src_stride_y == width) && (dst_stride_y == width)) {
            width *= height;
            height = 1;
            src_stride_y = dst_stride_y = 0;
        }

        void (*pfCopyRow)(const planeType* src, planeType* dst, int width, int shiftBits) =
                (shiftBits == 0) ? CopyRow : CopyRowShiftLeft;

        // Copy plane
        for (y = 0; y < height; ++y) {
            pfCopyRow(src_y, dst_y, width, shiftBits);
            src_y += src_stride_y;
            dst_y += dst_stride_y;
        }
    }

    static void MergeUVRow(const planeType* src_u,
                           const planeType* src_v,
                           planeType* dst_uv,
                           int width,
                           int /* shiftBits */) {

        for (int x = 0; x < width - 1; x += 2) {
            dst_uv[0] = src_u[x];
            dst_uv[1] = src_v[x];
            dst_uv[2] = src_u[x + 1];
            dst_uv[3] = src_v[x + 1];
            dst_uv += 4;
        }
        if (width & 1) {
            dst_uv[0] = src_u[width - 1];
            dst_uv[1] = src_v[width - 1];
        }
    }

    static void MergeUVRowShiftLeft(const planeType* src_u,
                           const planeType* src_v,
                           planeType* dst_uv,
                           int width,
                           int shiftBits) {

        for (int x = 0; x < width - 1; x += 2) {
            dst_uv[0] = static_cast<planeType>(src_u[x] << shiftBits);
            dst_uv[1] = static_cast<planeType>(src_v[x] << shiftBits);
            dst_uv[2] = static_cast<planeType>(src_u[x + 1] << shiftBits);
            dst_uv[3] = static_cast<planeType>(src_v[x + 1] << shiftBits);
            dst_uv += 4;
        }
        if (width & 1) {
            dst_uv[0] = static_cast<planeType>(src_u[width - 1] << shiftBits);
            dst_uv[1] = static_cast<planeType>(src_v[width - 1] << shiftBits);
        }
    }

    static void MergeUVPlane(const planeType* src_u,
                             int src_stride_u,
                             const planeType* src_v,
                             int src_stride_v,
                             planeType* dst_uv,
                             int dst_stride_uv,
                             int width,
                             int height,
                             int shiftBits) {

        if ((width <= 0) || (height == 0)) {
            return;
        }

        // Negative height means invert the image.
        if (height < 0) {
            height = -height;
            dst_uv = dst_uv + (height - 1) * dst_stride_uv;
            dst_stride_uv = -dst_stride_uv;
        }

        // Coalesce rows.
        if ((src_stride_u == width) && (src_stride_v == width) &&
                (dst_stride_uv == width * 2)) {
            width *= height;
            height = 1;
            src_stride_u = src_stride_v = dst_stride_uv = 0;
        }

        void (*pfMergeUVRow)(const planeType* src_u,
                const planeType* src_v,
                planeType* dst_uv,
                int width,
                int shiftBits) =
                (shiftBits == 0) ? MergeUVRow : MergeUVRowShiftLeft;

        for (int y = 0; y < height; ++y) {
            // Merge a row of U and V into a row of UV.
            pfMergeUVRow(src_u, src_v, dst_uv, width, shiftBits);
            src_u += src_stride_u;
            src_v += src_stride_v;
            dst_uv += dst_stride_uv;
        }

    }

    static int I420ToNV12(const planeType* src_y,
                          int src_stride_y,
                          const planeType* src_u,
                          int src_stride_u,
                          const planeType* src_v,
                          int src_stride_v,
                          planeType* dst_y,
                          int dst_stride_y,
                          planeType* dst_uv,
                          int dst_stride_uv,
                          int width,
                          int height,
                          int shiftBits = 0) {

        int halfwidth = (width + 1) / 2;
        int halfheight = (height + 1) / 2;
        if (!src_y || !src_u || !src_v || !dst_uv || width <= 0 || height == 0) {
            return -1;
        }

        src_stride_y /= (int)sizeof(planeType);
        dst_stride_y /= (int)sizeof(planeType);

        src_stride_u /= (int)sizeof(planeType);
        src_stride_v /= (int)sizeof(planeType);

        dst_stride_uv /= (int)sizeof(planeType);

        // Negative height means invert the image.
        if (height < 0) {
            height = -height;
            halfheight = (height + 1) >> 1;
            src_y = src_y + (height - 1) * src_stride_y;
            src_u = src_u + (halfheight - 1) * src_stride_u;
            src_v = src_v + (halfheight - 1) * src_stride_v;
            src_stride_y = -src_stride_y;
            src_stride_u = -src_stride_u;
            src_stride_v = -src_stride_v;
        }

        if (dst_y) {
            CopyPlane(src_y, src_stride_y, dst_y, dst_stride_y, width, height, shiftBits);
        }

        MergeUVPlane(src_u, src_stride_u, src_v, src_stride_v, dst_uv, dst_stride_uv,
                     halfwidth, halfheight, shiftBits);

        return 0;
    }
};

#endif /* _VKCODECUTILS_YCBCRCONVUTILSCPU_H_ */
