/*
* Copyright 2021 - 2023 NVIDIA Corporation.
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

#include "string.h"
#include "vkvideo_parser/VulkanVideoParserIf.h"
#include "VulkanH264Decoder.h"
#include "nvVulkanh264ScalingList.h"

/////////////////////////////////////////////////////////////////////////////////////
//
// Scaling
//

static unsigned char Flat_4x4_16[4][4] = {
    {16, 16, 16, 16},
    {16, 16, 16, 16},
    {16, 16, 16, 16},
    {16, 16, 16, 16}
};
static const unsigned char Default_4x4_Intra[4][4] = {
    { 6, 13, 20, 28},
    {13, 20, 28, 32},
    {20, 28, 32, 37},
    {28, 32 ,37, 42}
};
static const unsigned char Default_4x4_Inter[4][4] = {
    {10, 14, 20, 24},
    {14, 20, 24, 27},
    {20, 24, 27, 30},
    {24, 27, 30, 34}
};
static const unsigned char Flat_8x8_16[8][8] = {
    {16, 16, 16, 16, 16, 16, 16, 16},
    {16, 16, 16, 16, 16, 16, 16, 16},
    {16, 16, 16, 16, 16, 16, 16, 16},
    {16, 16, 16, 16, 16, 16, 16, 16},
    {16, 16, 16, 16, 16, 16, 16, 16},
    {16, 16, 16, 16, 16, 16, 16, 16},
    {16, 16, 16, 16, 16, 16, 16, 16},
    {16, 16, 16, 16, 16, 16, 16, 16}
};
static const unsigned char Default_8x8_Intra[8][8] = {
    { 6, 10, 13, 16, 18, 23, 25, 27},
    {10, 11, 16, 18, 23, 25, 27, 29},
    {13, 16, 18, 23, 25, 27, 29, 31},
    {16, 18, 23, 25, 27, 29, 31, 33},
    {18, 23, 25, 27, 29, 31, 33, 36},
    {23, 25, 27, 29, 31, 33, 36, 38},
    {25, 27, 29, 31, 33, 36, 38, 40},
    {27, 29, 31, 33, 36, 38, 40, 42}
};
static const unsigned char Default_8x8_Inter[8][8] = {
    { 9, 13, 15, 17, 19, 21, 22, 24},
    {13, 13, 17, 19, 21, 22, 24, 25},
    {15, 17, 19, 21, 22, 24, 25, 27},
    {17, 19, 21, 22, 24, 25, 27, 28},
    {19, 21, 22, 24, 25, 27, 28, 30},
    {21, 22, 24, 25, 27, 28, 30, 32},
    {22, 24, 25, 27, 28, 30, 32, 33},
    {24, 25, 27, 28, 30, 32, 33, 35}
};

void matrix_from_matrix_4x4(unsigned char dst[4][4], const unsigned char src[4][4])
{
    memcpy(dst, src, sizeof(unsigned char [4][4]));
}

void matrix_from_matrix_8x8(unsigned char dst[8][8], const unsigned char src[8][8])
{
    memcpy(dst, src, sizeof(unsigned char [8][8]));
}

void matrix_from_list_4x4(unsigned char matrix[4][4], const unsigned char list[16])
{
    // Table 8-12
    static const int map[16][2] = {
        {0,0}, {0,1}, {1,0}, {2,0}, {1,1}, {0,2}, {0,3}, {1,2}, {2,1}, {3,0}, {3,1}, {2,2}, {1,3}, {2,3}, {3,2}, {3,3}
    };
    int i, j, k;

    for (k = 0; k < 16; k++)
    {
        i = map[k][0];
        j = map[k][1];
        matrix[i][j] = list[k];
    }
}

void matrix_from_list_8x8(unsigned char matrix[8][8], const unsigned char list[64])
{
    // Table 8-12a
    static const int map[64][2] = {
        {0,0},  {0,1},  {1,0},  {2,0},  {1,1},  {0,2},  {0,3},  {1,2},  {2,1},  {3,0},  {4,0},  {3,1},  {2,2},  {1,3},  {0,4},  {0,5},
        {1,4},  {2,3},  {3,2},  {4,1},  {5,0},  {6,0},  {5,1},  {4,2},  {3,3},  {2,4},  {1,5},  {0,6},  {0,7},  {1,6},  {2,5},  {3,4},
        {4,3},  {5,2},  {6,1},  {7,0},  {7,1},  {6,2},  {5,3},  {4,4},  {3,5},  {2,6},  {1,7},  {2,7},  {3,6},  {4,5},  {5,4},  {6,3},
        {7,2},  {7,3},  {6,4},  {5,5},  {4,6},  {3,7},  {4,7},  {5,6},  {6,5},  {7,4},  {7,5},  {6,6},  {5,7},  {6,7},  {7,6},  {7,7}
    };
    int i, j, k;

    for (k = 0; k < 64; k++)
    {
        i = map[k][0];
        j = map[k][1];
        matrix[i][j] = list[k];
    }
}

NVPARSER_EXPORT
bool SetSpsScalingListsH264(const NvScalingListH264 *pSeqScalingList,
                            uint8_t pSeqWeightScale4x4[6][4][4],
                            uint8_t pSeqWeightScale8x8[2][8][8])
{

    if (!pSeqScalingList || !pSeqScalingList->scaling_matrix_present_flag)
    {
        for (int32_t i = 0; i < 6; i++) {
            matrix_from_matrix_4x4(pSeqWeightScale4x4[i], Flat_4x4_16);
        }
        for (int32_t i = 0; i < 2; i++) {
            matrix_from_matrix_8x8(pSeqWeightScale8x8[i], Flat_8x8_16);
        }
        return false;
    }

    for (int32_t i = 0; i < 6; i++)
    {
        if (pSeqScalingList->scaling_list_type[i] != SCALING_LIST_NOT_PRESENT)
        {
            if (pSeqScalingList->scaling_list_type[i] == SCALING_LIST_USE_DEFAULT) {
                matrix_from_matrix_4x4(pSeqWeightScale4x4[i], (i < 3) ? Default_4x4_Intra : Default_4x4_Inter);
            } else {
                matrix_from_list_4x4(pSeqWeightScale4x4[i], pSeqScalingList->ScalingList4x4[i]);
            }
        }
        else
        {
            // fall-back rule set A
            if ((i == 0) || (i == 3)) {
                matrix_from_matrix_4x4(pSeqWeightScale4x4[i], (i < 3) ? Default_4x4_Intra : Default_4x4_Inter);
            } else {
                matrix_from_matrix_4x4(pSeqWeightScale4x4[i], pSeqWeightScale4x4[i-1]);
            }
        }
    }
    for (int32_t i = 6; i < 8; i++)
    {
        if (pSeqScalingList->scaling_list_type[i] != SCALING_LIST_NOT_PRESENT)
        {
            if (pSeqScalingList->scaling_list_type[i] == SCALING_LIST_USE_DEFAULT) {
                matrix_from_matrix_8x8(pSeqWeightScale8x8[i-6], (i < 7) ? Default_8x8_Intra : Default_8x8_Inter);
            } else {
                matrix_from_list_8x8(pSeqWeightScale8x8[i-6], pSeqScalingList->ScalingList8x8[i-6]);
            }
        }
        else
        {
            // fall-back rule set A
            matrix_from_matrix_8x8(pSeqWeightScale8x8[i-6], (i < 7) ? Default_8x8_Intra : Default_8x8_Inter);
        }
    }

    return pSeqScalingList->scaling_matrix_present_flag;
}

NVPARSER_EXPORT
bool SetPpsScalingListsH264(const NvScalingListH264 *pPicScalingList,
                            bool seq_scaling_matrix_present_flag,
                            const uint8_t pSpsWeightScale4x4[6][4][4],
                            const uint8_t pSpsWeightScale8x8[2][8][8],
                            uint8_t pWeightScale4x4[6][4][4],
                            uint8_t pWeightScale8x8[2][8][8])
{
    if (!pPicScalingList || !pPicScalingList->scaling_matrix_present_flag)
    {
        if (seq_scaling_matrix_present_flag && pSpsWeightScale4x4) {
            for (int32_t i = 0; i < 6; i++) {
                matrix_from_matrix_4x4(pWeightScale4x4[i], pSpsWeightScale4x4[i]);
            }
        } else {
            for (int32_t i = 0; i < 6; i++) {
                matrix_from_matrix_4x4(pWeightScale4x4[i], Flat_4x4_16);
            }
        }
        if (seq_scaling_matrix_present_flag && pSpsWeightScale8x8) {
            for (int32_t i = 0; i < 2; i++) {
                matrix_from_matrix_8x8(pWeightScale8x8[i], pSpsWeightScale8x8[i]);
            }
        } else {
            for (int32_t i = 0; i < 2; i++) {
                matrix_from_matrix_8x8(pWeightScale8x8[i], Flat_8x8_16);
            }
        }

        return false;
    }

    for (int32_t i = 0; i < 6; i++)
    {
        if (pPicScalingList->scaling_list_type[i] != SCALING_LIST_NOT_PRESENT)
        {
            if (pPicScalingList->scaling_list_type[i] == SCALING_LIST_USE_DEFAULT) {
                matrix_from_matrix_4x4(pWeightScale4x4[i], (i < 3) ? Default_4x4_Intra : Default_4x4_Inter);
            } else {
                matrix_from_list_4x4(pWeightScale4x4[i], pPicScalingList->ScalingList4x4[i]);
            }
        }
        else
        {
            if (!seq_scaling_matrix_present_flag)
            {
                // fall-back rule set A
                if ((i == 0) || (i == 3)) {
                    matrix_from_matrix_4x4(pWeightScale4x4[i], (i < 3) ? Default_4x4_Intra : Default_4x4_Inter);
                } else {
                    matrix_from_matrix_4x4(pWeightScale4x4[i], pWeightScale4x4[i-1]);
                }
            }
            else
            {
                // fall-back rule set B
                if ((i == 0) || (i == 3)) {
                    matrix_from_matrix_4x4(pWeightScale4x4[i], pSpsWeightScale4x4[i]);
                } else {
                    matrix_from_matrix_4x4(pWeightScale4x4[i], pWeightScale4x4[i-1]);
                }
            }
        }
    }
    for (int32_t i = 6; i < 8; i++)
    {
        if (pPicScalingList->scaling_list_type[i] != SCALING_LIST_NOT_PRESENT)
        {
            if (pPicScalingList->scaling_list_type[i] == SCALING_LIST_USE_DEFAULT) {
                matrix_from_matrix_8x8(pWeightScale8x8[i-6], (i < 7) ? Default_8x8_Intra : Default_8x8_Inter);
            } else {
                matrix_from_list_8x8(pWeightScale8x8[i-6], pPicScalingList->ScalingList8x8[i-6]);
            }
        }
        else
        {
            if (!seq_scaling_matrix_present_flag)
            {
                // fall-back rule set A
                matrix_from_matrix_8x8(pWeightScale8x8[i-6], (i < 7) ? Default_8x8_Intra : Default_8x8_Inter);
            }
            else
            {
                // fall-back rule set B
                matrix_from_matrix_8x8(pWeightScale8x8[i-6], pSpsWeightScale8x8[i-6]);
            }
        }
    }

    return pPicScalingList->scaling_matrix_present_flag;
}


NVPARSER_EXPORT
bool SetSeqPicScalingListsH264(const NvScalingListH264 *pSeqScalingList,
                               const NvScalingListH264 *pPicScalingList,
                               uint8_t pWeightScale4x4[6][4][4],
                               uint8_t pWeightScale8x8[2][8][8])
{
    uint8_t pSpsWeightScale4x4[6][4][4];
    uint8_t pSpsWeightScale8x8[2][8][8];
    bool seq_scaling_matrix_present_flag =
            SetSpsScalingListsH264(pSeqScalingList,
                                   pSpsWeightScale4x4,
                                   pSpsWeightScale8x8);

    return SetPpsScalingListsH264(pPicScalingList,
                                  seq_scaling_matrix_present_flag,
                                  pSpsWeightScale4x4,
                                  pSpsWeightScale8x8,
                                  pWeightScale4x4,
                                  pWeightScale8x8);
}



