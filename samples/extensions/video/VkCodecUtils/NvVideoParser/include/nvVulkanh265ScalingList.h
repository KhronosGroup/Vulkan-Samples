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

#ifndef _NVVULKANH265SCALINGLIST_H_
#define _NVVULKANH265SCALINGLIST_H_

#include "nvVulkanVideoParser.h"

struct scaling_list_entry_s
{
    int scaling_list_pred_mode_flag;
    int scaling_list_pred_matrix_id_delta;
    int scaling_list_dc_coef_minus8;
    int8_t scaling_list_delta_coef[64];
};

struct scaling_list_s
{
    scaling_list_entry_s entry[4][6];
};

static void Init4x4ScalingListsH265(uint8_t *scalingFactors, const scaling_list_s *scl)
{
    for (int32_t matrixId = 0; matrixId < 6; matrixId++) {

        uint8_t *scalingFactor = scalingFactors + 4 * 4 * matrixId;
        const scaling_list_entry_s *scle = &scl->entry[0][matrixId];
        if (!scle->scaling_list_pred_mode_flag) {

            if (scle->scaling_list_pred_matrix_id_delta != 0) {

                // duplicate
                int32_t refMatrixId = matrixId - scle->scaling_list_pred_matrix_id_delta;
                assert(refMatrixId >= 0);
                uint8_t *refScalingFactor = scalingFactors + 4*4*refMatrixId;
                memcpy(scalingFactor, refScalingFactor, 4*4*sizeof(*scalingFactor));

            } else {

                // default values (4x4)
                for (int32_t k = 0; k < 16; k++)
                    scalingFactor[k] = 16;
            }
        } else { // (scle->scaling_list_pred_mode_flag)

            int32_t nextCoef = 8;
            for (int32_t k = 0; k < 16; k++) {

                nextCoef = (nextCoef + scle->scaling_list_delta_coef[k]) & 0xff;
                scalingFactor[k] = (uint8_t)nextCoef;
            }
        }
    }
}


static void Init8x8ScalingListsH265(uint8_t *scalingFactors, uint8_t *scalingFactorsDC, const scaling_list_s *scl, int32_t sizeId)
{
    // Specification of default values of ScalingList[1..3][matrixId][i] with i = 0..63
    static const uint8_t DefaultScalingList8x8[2][64] =
        { { 16,16,16,16,16,16,16,16,16,16,17,16,17,16,17,18,17,18,18,17,18,21,19,20,21,20,19,21,24,22,22,24,
            24,22,22,24,25,25,27,30,27,25,25,29,31,35,35,31,29,36,41,44,41,36,47,54,54,47,65,70,65,88,88,115},
          { 16,16,16,16,16,16,16,16,16,16,17,17,17,17,17,18,18,18,18,18,18,20,20,20,20,20,20,20,24,24,24,24,
            24,24,24,24,25,25,25,25,25,25,25,28,28,28,28,28,28,33,33,33,33,33,41,41,41,41,54,54,54,71,71,91 } };

    for (int32_t matrixId = 0; matrixId < ((sizeId == 3) ? 2 : 6); matrixId++)
    {
        uint8_t *scalingFactor = scalingFactors + 8 * 8 * matrixId;
        const scaling_list_entry_s *scle = &scl->entry[sizeId][matrixId];
        if (!scle->scaling_list_pred_mode_flag) {
            if (scle->scaling_list_pred_matrix_id_delta != 0) {
                // duplicate
                int32_t refMatrixId = matrixId - scle->scaling_list_pred_matrix_id_delta;
                assert(refMatrixId >= 0);
                uint8_t *refScalingFactor = scalingFactors + 8*8*refMatrixId;
                memcpy(scalingFactor, refScalingFactor, 8*8*sizeof(*scalingFactor));
                if (sizeId >= 2)
                    scalingFactorsDC[matrixId] = scalingFactorsDC[refMatrixId];
            } else {
                // default values (>= 8x8)
                int32_t listIdx = (matrixId >= ((sizeId != 3) ? 3 : 1));
                for (int32_t k = 0; k < 64; k++) {
                    scalingFactor[k] = DefaultScalingList8x8[listIdx][k];
                }
                if (sizeId >= 2) {
                    scalingFactorsDC[matrixId] = DefaultScalingList8x8[listIdx][0];
                }
            }
        } else { // (scle->scaling_list_pred_mode_flag)

            int32_t nextCoef = (sizeId < 2) ? 8 : scle->scaling_list_dc_coef_minus8 + 8;
            for (int32_t k = 0; k < 64; k++)
            {
                nextCoef = (nextCoef + scle->scaling_list_delta_coef[k]) & 0xff;
                scalingFactor[k] = (uint8_t)nextCoef;
            }
            if (sizeId >= 2) {
                scalingFactorsDC[matrixId] = (uint8_t)(scle->scaling_list_dc_coef_minus8 + 8);
            }
        }
    }
}

#endif /* _NVVULKANH265SCALINGLIST_H_ */
