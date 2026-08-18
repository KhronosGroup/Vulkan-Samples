/*
 * SPDX-FileCopyrightText: Copyright (c) 2021 - 2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef _NVVULKANH264SCALINGLIST_H_
#define _NVVULKANH264SCALINGLIST_H_

#include "nvVulkanVideoParser.h"

enum NvScalingListTypeH264 {
    SCALING_LIST_NOT_PRESENT = 0,
    SCALING_LIST_PRESENT,
    SCALING_LIST_USE_DEFAULT,
};

struct NvScalingListH264
{
    int32_t scaling_matrix_present_flag:1;
    uint8_t scaling_list_type[8]; // scaling_list_type_e
    uint8_t ScalingList4x4[6][16];
    uint8_t ScalingList8x8[2][64];
};

NVPARSER_EXPORT
bool SetSeqPicScalingListsH264(const NvScalingListH264 *pSeqScalingList,
                               const NvScalingListH264 *pPicScalingList,
                               uint8_t pWeightScale4x4[6][4][4],
                               uint8_t pWeightScale8x8[2][8][8]);

#endif /* _NVVULKANH264SCALINGLIST_H_ */
