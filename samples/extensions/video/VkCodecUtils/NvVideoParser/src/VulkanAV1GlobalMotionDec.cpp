/*
* Copyright 2023 NVIDIA Corporation.
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

#include <stdint.h>

#include "VulkanVideoParserIf.h"

#include "VulkanAV1Decoder.h"


#define DIV_LUT_PREC_BITS 14
#define DIV_LUT_BITS 8
#define DIV_LUT_NUM (1 << DIV_LUT_BITS)

#define ROUND_POWER_OF_TWO(value, n) (((value) + (1 << ((n)-1))) >> (n))

/* Shift down with rounding for use when n >= 0, value >= 0 for (64 bit) */
#define ROUND_POWER_OF_TWO_64(value, n) \
  (((value) + ((((int64_t)1 << (n)) >> 1))) >> (n))

/* Shift down with rounding for signed integers, for use when n >= 0 (64 bit) */
#define ROUND_POWER_OF_TWO_SIGNED_64(value, n)           \
  (((value) < 0) ? -ROUND_POWER_OF_TWO_64(-(value), (n)) \
                 : ROUND_POWER_OF_TWO_64((value), (n)))

/* Shift down with rounding for signed integers, for use when n >= 0 */
#define ROUND_POWER_OF_TWO_SIGNED(value, n)           \
  (((value) < 0) ? -ROUND_POWER_OF_TWO(-(value), (n)) \
                 : ROUND_POWER_OF_TWO((value), (n)))

// Bits of precision used for the model
#define WARPEDMODEL_PREC_BITS 16
#define WARPEDMODEL_ROW3HOMO_PREC_BITS 16

// Bits of subpel precision for warped interpolation
#define WARPEDPIXEL_PREC_BITS 6
#define WARPEDPIXEL_PREC_SHIFTS (1 << WARPEDPIXEL_PREC_BITS)

#define SUBEXPFIN_K 3
#define GM_TRANS_PREC_BITS 6
#define GM_ABS_TRANS_BITS 12
#define GM_ABS_TRANS_ONLY_BITS (GM_ABS_TRANS_BITS - GM_TRANS_PREC_BITS + 3)
#define GM_TRANS_PREC_DIFF (WARPEDMODEL_PREC_BITS - GM_TRANS_PREC_BITS)
#define GM_TRANS_ONLY_PREC_DIFF (WARPEDMODEL_PREC_BITS - 3)
#define GM_TRANS_DECODE_FACTOR (1 << GM_TRANS_PREC_DIFF)
#define GM_TRANS_ONLY_DECODE_FACTOR (1 << GM_TRANS_ONLY_PREC_DIFF)

#define GM_ALPHA_PREC_BITS 15
#define GM_ABS_ALPHA_BITS 12
#define GM_ALPHA_PREC_DIFF (WARPEDMODEL_PREC_BITS - GM_ALPHA_PREC_BITS)
#define GM_ALPHA_DECODE_FACTOR (1 << GM_ALPHA_PREC_DIFF)

#define GM_ROW3HOMO_PREC_BITS 16
#define GM_ABS_ROW3HOMO_BITS 11
#define GM_ROW3HOMO_PREC_DIFF \
  (WARPEDMODEL_ROW3HOMO_PREC_BITS - GM_ROW3HOMO_PREC_BITS)
#define GM_ROW3HOMO_DECODE_FACTOR (1 << GM_ROW3HOMO_PREC_DIFF)

#define GM_ROW3HOMO_MAX (1 << GM_ABS_ROW3HOMO_BITS)

#define GM_TRANS_MAX (1 << GM_ABS_TRANS_BITS)
#define GM_ALPHA_MAX (1 << GM_ABS_ALPHA_BITS)
#define GM_ROW3HOMO_MAX (1 << GM_ABS_ROW3HOMO_BITS)

#define GM_TRANS_MIN -GM_TRANS_MAX
#define GM_ALPHA_MIN -GM_ALPHA_MAX
#define GM_ROW3HOMO_MIN -GM_ROW3HOMO_MAX

#define WARP_PARAM_REDUCE_BITS 6
#define WARPEDMODEL_PREC_BITS 16

int get_msb(unsigned int n)
{
    int log = 0;
    unsigned int value = n;
    int i;

    assert(n != 0);

    for (i = 4; i >= 0; --i) {
        const int shift = (1 << i);
        const unsigned int x = value >> shift;
        if (x != 0) {
            value = x;
            log += shift;
        }
    }

    return log;
}

// Inverse recenters a non-negative literal v around a reference r
static uint16_t inv_recenter_nonneg(uint16_t r, uint16_t v)
{
    if (v > (r << 1))
        return v;
    else if ((v & 1) == 0)
        return (v >> 1) + r;
    else
        return r - ((v + 1) >> 1);
}

// Inverse recenters a non-negative literal v in [0, n-1] around a
// reference r also in [0, n-1]
static uint16_t inv_recenter_finite_nonneg(uint16_t n, uint16_t r, uint16_t v)
{
    if ((r << 1) <= n) {
        return inv_recenter_nonneg(r, v);
    } else {
        return n - 1 - inv_recenter_nonneg(n - 1 - r, v);
    }
}

uint16_t VulkanAV1Decoder::Read_primitive_quniform(uint16_t n)
{
    if (n <= 1) return 0;
    const int l = get_msb(n - 1) + 1;
    const int m = (1 << l) - n;
    const int v = u(l - 1);
    return v < m ? v : (v << 1) - m + u(1);
}

uint16_t VulkanAV1Decoder::Read_primitive_subexpfin(uint16_t n, uint16_t k)
{
    int i = 0;
    int mk = 0;

    while (1) {
        int b = (i ? k + i - 1 : k);
        int a = (1 << b);

        if (n <= mk + 3 * a) {
            return Read_primitive_quniform(n - mk) + mk;
        }

        if (!u(1)) {
        return u(b) + mk;
        }

        i = i + 1;
        mk += a;
    }

    assert(0);
    return 0;
}

uint16_t VulkanAV1Decoder::Read_primitive_refsubexpfin(uint16_t n, uint16_t k, uint16_t ref)
{
    return inv_recenter_finite_nonneg(n, ref,
                                    Read_primitive_subexpfin(n, k));
}

int16_t VulkanAV1Decoder::Read_signed_primitive_refsubexpfin(uint16_t n, uint16_t k, int16_t ref)
{
    ref += n - 1;
    const uint16_t scaled_n = (n << 1) - 1;
    return Read_primitive_refsubexpfin(scaled_n, k, ref) - n + 1;
}

int VulkanAV1Decoder::ReadGlobalMotionParams(AV1WarpedMotionParams *params, const AV1WarpedMotionParams *ref_params, int allow_hp)
{
    AV1_TRANSFORMATION_TYPE type = (AV1_TRANSFORMATION_TYPE)u(1);
    if (type != IDENTITY)
    {
        if (u(1))
        type = ROTZOOM;
        else
        type = u(1) ? TRANSLATION : AFFINE;
    }

    *params = default_warp_params;
    params->wmtype = type;

    if (type >= ROTZOOM) {
        params->wmmat[2] = Read_signed_primitive_refsubexpfin(
                           GM_ALPHA_MAX + 1, SUBEXPFIN_K,
                           (ref_params->wmmat[2] >> GM_ALPHA_PREC_DIFF) -
                               (1 << GM_ALPHA_PREC_BITS)) *
                           GM_ALPHA_DECODE_FACTOR +
                       (1 << WARPEDMODEL_PREC_BITS);
        params->wmmat[3] = Read_signed_primitive_refsubexpfin(
                           GM_ALPHA_MAX + 1, SUBEXPFIN_K,
                           (ref_params->wmmat[3] >> GM_ALPHA_PREC_DIFF)) *
                       GM_ALPHA_DECODE_FACTOR;
    }

    if (type >= AFFINE) {
        params->wmmat[4] = Read_signed_primitive_refsubexpfin(
                           GM_ALPHA_MAX + 1, SUBEXPFIN_K,
                           (ref_params->wmmat[4] >> GM_ALPHA_PREC_DIFF)) *
                       GM_ALPHA_DECODE_FACTOR;
        params->wmmat[5] = Read_signed_primitive_refsubexpfin(
                           GM_ALPHA_MAX + 1, SUBEXPFIN_K,
                           (ref_params->wmmat[5] >> GM_ALPHA_PREC_DIFF) -
                               (1 << GM_ALPHA_PREC_BITS)) *
                           GM_ALPHA_DECODE_FACTOR +
                       (1 << WARPEDMODEL_PREC_BITS);
    } else {
        params->wmmat[4] = -params->wmmat[3];
        params->wmmat[5] = params->wmmat[2];
    }

    if (type >= TRANSLATION) {
        const int trans_bits = (type == TRANSLATION)
                                   ? GM_ABS_TRANS_ONLY_BITS - !allow_hp
                                   : GM_ABS_TRANS_BITS;
        const int trans_dec_factor =
            (type == TRANSLATION) ? GM_TRANS_ONLY_DECODE_FACTOR * (1 << (allow_hp ? 0 : 1)) : GM_TRANS_DECODE_FACTOR;
        const int trans_prec_diff = (type == TRANSLATION)
                                        ? GM_TRANS_ONLY_PREC_DIFF + !allow_hp
                                        : GM_TRANS_PREC_DIFF;
        params->wmmat[0] = Read_signed_primitive_refsubexpfin(
                               (1 << trans_bits) + 1, SUBEXPFIN_K,
                               (ref_params->wmmat[0] >> trans_prec_diff)) *
                           trans_dec_factor;
        params->wmmat[1] = Read_signed_primitive_refsubexpfin(
                               (1 << trans_bits) + 1, SUBEXPFIN_K,
                               (ref_params->wmmat[1] >> trans_prec_diff)) *
                               trans_dec_factor;
    }

    return 1;
}

uint32_t VulkanAV1Decoder::DecodeGlobalMotionParams()
{
    StdVideoDecodeAV1PictureInfo* pStd = &m_PicData.std_info;

    AV1WarpedMotionParams prev_models[GM_GLOBAL_MODELS_PER_FRAME];

    for(int i=0;i<GM_GLOBAL_MODELS_PER_FRAME;++i)
        prev_models[i] = default_warp_params;

    if(pStd->primary_ref_frame != STD_VIDEO_AV1_PRIMARY_REF_NONE) {
        if (m_pBuffers[ref_frame_idx[pStd->primary_ref_frame]].buffer)
            memcpy(prev_models, m_pBuffers[ref_frame_idx[pStd->primary_ref_frame]].global_models, sizeof(AV1WarpedMotionParams)*GM_GLOBAL_MODELS_PER_FRAME);
    }

    for (int frame = 0; frame < GM_GLOBAL_MODELS_PER_FRAME; ++frame) {
        AV1WarpedMotionParams *ref_params = &prev_models[frame];

        int good_params = ReadGlobalMotionParams(&global_motions[frame], ref_params, pStd->flags.allow_high_precision_mv);
        if (!good_params) {
          global_motions[frame].invalid = 1;
        }
    }
    return 1;
}
