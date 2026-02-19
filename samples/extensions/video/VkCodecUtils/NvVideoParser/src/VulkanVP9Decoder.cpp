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

#include "VulkanVideoParserIf.h"

#include "VulkanVP9Decoder.h"

VulkanVP9Decoder::VulkanVP9Decoder(VkVideoCodecOperationFlagBitsKHR std)
    : VulkanVideoDecoder(std)
{
    memset(&m_EntropyLast, 0, sizeof(m_EntropyLast));
    memset(&m_PrevCtx, 0, sizeof(m_PrevCtx));
    memset(&reader, 0, sizeof(vp9_reader));
    m_pCompressedHeader = NULL;
}
void VulkanVP9Decoder::vp9_init_mbmode_probs(vp9_prob_update_s *pProbSetup)
{
    uint32_t i, j;

    for (i = 0; i < BLOCK_SIZE_GROUPS; i++)
    {
        for (j = 0; j < 8; j++)
            pProbSetup->pProbTab->a.sb_ymode_prob[i][j] = default_if_y_probs[i][j];
        pProbSetup->pProbTab->a.sb_ymode_probB[i][0] = default_if_y_probs[i][8];
    }

    for (i = 0; i < VP9_INTRA_MODES; i++)
    {
        for (j = 0; j < 8; j++)
            pProbSetup->pProbTab->kf_uv_mode_prob[i][j] = default_kf_uv_probs[i][j];
        pProbSetup->pProbTab->kf_uv_mode_probB[i][0] = default_kf_uv_probs[i][8];

        for (j = 0; j < 8; j++)
            pProbSetup->pProbTab->a.uv_mode_prob[i][j] = default_if_uv_probs[i][j];
        pProbSetup->pProbTab->a.uv_mode_probB[i][0] = default_if_uv_probs[i][8];
    }

    memcpy(pProbSetup->pProbTab->a.switchable_interp_prob, vp9_switchable_interp_prob,
             sizeof(vp9_switchable_interp_prob));
    memcpy(pProbSetup->pProbTab->a.partition_prob, vp9_partition_probs,
             sizeof(vp9_partition_probs));
    memcpy(pProbSetup->pProbTab->a.intra_inter_prob, default_intra_inter_p,
             sizeof(default_intra_inter_p));
    memcpy(pProbSetup->pProbTab->a.comp_inter_prob, default_comp_inter_p,
             sizeof(default_comp_inter_p));
    memcpy(pProbSetup->pProbTab->a.comp_ref_prob, default_comp_ref_p,
             sizeof(default_comp_ref_p));
    memcpy(pProbSetup->pProbTab->a.single_ref_prob, default_single_ref_p,
             sizeof(default_single_ref_p));
    memcpy(pProbSetup->pProbTab->a.tx32x32_prob, vp9_default_tx_probs_32x32p,
             sizeof(vp9_default_tx_probs_32x32p));
    memcpy(pProbSetup->pProbTab->a.tx16x16_prob, vp9_default_tx_probs_16x16p,
             sizeof(vp9_default_tx_probs_16x16p));
    memcpy(pProbSetup->pProbTab->a.tx8x8_prob, vp9_default_tx_probs_8x8p,
             sizeof(vp9_default_tx_probs_8x8p));
    memcpy(pProbSetup->pProbTab->a.mbskip_probs, vp9_default_mbskip_probs,
             sizeof(vp9_default_mbskip_probs));

    for (i = 0; i < VP9_INTRA_MODES; i++)
    {
        for (j = 0; j < VP9_INTRA_MODES; j++)
        {
            memcpy(pProbSetup->pProbTab->kf_bmode_prob[i][j], vp9_kf_default_bmode_probs[i][j], 8);
            pProbSetup->pProbTab->kf_bmode_probB[i][j][0] = vp9_kf_default_bmode_probs[i][j][8];
        }
    }
}

void VulkanVP9Decoder::ResetProbs(vp9_prob_update_s *pProbSetup)
{
    //reset segmentMap (buffers going to HWIF_SEGMENT_READ_BASE_LSB and HWIF_SEGMENT_WRITE_BASE_LSB)

    uint32_t i, j, k, l, m;

    memcpy(pProbSetup->pProbTab->a.inter_mode_prob, vp9_default_inter_mode_prob, sizeof(vp9_default_inter_mode_prob));
    vp9_init_mbmode_probs(pProbSetup);
    memcpy(&pProbSetup->pProbTab->a.nmvc, &vp9_default_nmv_context, sizeof(nvdec_nmv_context));

    /* Copy the default probs into two separate prob tables: part1 and part2. */

    for( i = 0; i < VP9_BLOCK_TYPES; i++ ) {
        for ( j = 0; j < VP9_REF_TYPES; j++ ) {
            for ( k = 0; k < VP9_COEF_BANDS; k++ ) {
                for ( l = 0; l < VP9_PREV_COEF_CONTEXTS; l++ ) {
                    if (l >= 3 && k == 0)
                        continue;

                    for ( m = 0; m < UNCONSTRAINED_NODES; m++ ) {
                        pProbSetup->pProbTab->a.probCoeffs[i][j][k][l][m] =
                            default_coef_probs_4x4[i][j][k][l][m];
                        pProbSetup->pProbTab->a.probCoeffs8x8[i][j][k][l][m] =
                            default_coef_probs_8x8[i][j][k][l][m];
                        pProbSetup->pProbTab->a.probCoeffs16x16[i][j][k][l][m] =
                            default_coef_probs_16x16[i][j][k][l][m];
                        pProbSetup->pProbTab->a.probCoeffs32x32[i][j][k][l][m] =
                            default_coef_probs_32x32[i][j][k][l][m];
                    }
                }
            }
        }
    }

    /* Store the default probs for all saved contexts */
    if (pProbSetup->keyFrame || pProbSetup->errorResilient || pProbSetup->resetFrameContext == 3)
    {
        for (i = 0; i < FRAME_CONTEXTS; i++)
            memcpy( &m_EntropyLast[i], pProbSetup->pProbTab, sizeof(nvdec_vp9EntropyProbs_t));
    }
    else if (pProbSetup->resetFrameContext == 2)
        memcpy( &m_EntropyLast[pProbSetup->frameContextIdx], pProbSetup->pProbTab, sizeof(nvdec_vp9EntropyProbs_t));
}

void VulkanVP9Decoder::GetProbs(vp9_prob_update_s *pProbSetup)
{
    memcpy(pProbSetup->pProbTab, &m_EntropyLast[pProbSetup->frameContextIdx], sizeof(m_EntropyLast[pProbSetup->frameContextIdx]));
}

/////////////////////////////////////////////////////////////////////////////////


void VulkanVP9Decoder::vp9_reader_fill()
{
    vp9_reader *r = &reader;
    uint32_t buffer_end = r->buffer_end;
    uint32_t buffer = r->buffer;
    VP9_BD_VALUE value = r->value;
    int32_t count = r->count;
    int32_t shift = BD_VALUE_SIZE - 8 - (count + 8);
    int32_t loop_end = 0;
    const int32_t bits_left = (int32_t)((buffer_end - buffer)*CHAR_BIT);
    const int32_t x = shift + CHAR_BIT - bits_left;
    if (x >= 0) {
        count += LOTS_OF_BITS;
        loop_end = x;
    }
    if (x < 0 || bits_left)
    {
        while (shift >= loop_end)
        {
            count += CHAR_BIT;
            uint8_t temp = m_pCompressedHeader[r->pos++]; //u( 8);
            value |= (VP9_BD_VALUE)temp << shift;
            shift -= CHAR_BIT;
            buffer++;
        }
    }
    r->buffer = buffer;
    r->value = value;
    r->count = count;
}

int32_t VulkanVP9Decoder::vp9_reader_init(uint32_t size)
{
    int32_t marker_bit = 0;
    vp9_reader *r = &reader;
    r->buffer_end = 0 + size;
    r->buffer = 0;
    r->value = 0;
    r->count = -8;
    r->range = 255;
    r->pos = 0;

    vp9_reader_fill();
    marker_bit = vp9_read_bit();
    return marker_bit != 0;
}

int32_t VulkanVP9Decoder::vp9_read_bit()
{
    return vp9_read( 128);
}

int32_t VulkanVP9Decoder::vp9_read(int32_t probability)
{

    vp9_reader *br = &reader;
    uint32_t bit = 0;
    VP9_BD_VALUE value;
    VP9_BD_VALUE bigsplit;
    int32_t count;
    uint32_t range;
    uint32_t split = 1 + (((br->range - 1) * probability) >> 8);
    if (br->count < 0)
        vp9_reader_fill();
    value = br->value;
    count = br->count;
    bigsplit = (VP9_BD_VALUE)split << (BD_VALUE_SIZE - 8);

    range = split;
    if (value >= bigsplit)
    {
        range = br->range - split;
        value = value - bigsplit;
        bit = 1;
    }
    register uint32_t shift = vp9dx_bitreader_norm[range];
    range <<= shift;
    value <<= shift;
    count -= shift;
    br->value = value;
    br->count = count;
    br->range = range;
    return bit;
}

int32_t VulkanVP9Decoder::vp9_read_literal( int32_t bits)
{
    int32_t z = 0, bit;

    for (bit = bits - 1; bit >= 0; bit--)
    {
        z |= vp9_read_bit() << bit;
    }
    return z;
}
////////////////////////////////////////////////////////////////////////////////////
//Forward Update
uint32_t VulkanVP9Decoder::UpdateForwardProbability(vp9_prob_update_s *pProbSetup, const unsigned char* pCompressed_Header)
{
    nvdec_vp9EntropyProbs_t *fc = pProbSetup->pProbTab; // Frame context

    uint32_t tmp, i, j, k;

    m_pCompressedHeader = pCompressed_Header;
    m_PrevCtx = pProbSetup->pProbTab->a;

    if (vp9_reader_init(pProbSetup->offsetToDctParts) != 0)
    {
        return NOK;
    }

    if (pProbSetup->lossless)
        pProbSetup->transform_mode = ONLY_4X4;
    else
    {
        pProbSetup->transform_mode = vp9_read_literal( 2);
        if (pProbSetup->transform_mode == ALLOW_32X32)
            pProbSetup->transform_mode += vp9_read_literal( 1);
        if (pProbSetup->transform_mode == TX_MODE_SELECT)
        {
             for (i = 0; i < TX_SIZE_CONTEXTS; ++i)
             {
                for (j = 0; j < TX_SIZE_MAX_SB - 3; ++j)
                {
                    tmp = vp9_read( VP9_DEF_UPDATE_PROB);
                    if (tmp) {
                        uint8_t *prob = &fc->a.tx8x8_prob[i][j];
                        *prob = vp9hwdReadProbDiffUpdate( *prob);
                    }
                }
            }
            for (i = 0; i < TX_SIZE_CONTEXTS; ++i)
            {
                for (j = 0; j < TX_SIZE_MAX_SB - 2; ++j) {
                    tmp = vp9_read( VP9_DEF_UPDATE_PROB);
                    if (tmp) {
                        uint8_t *prob = &fc->a.tx16x16_prob[i][j];
                        *prob = vp9hwdReadProbDiffUpdate( *prob);
                    }
                }
            }
            for (i = 0; i < TX_SIZE_CONTEXTS; ++i)
            {
                for (j = 0; j < TX_SIZE_MAX_SB - 1; ++j) {
                    tmp = vp9_read( VP9_DEF_UPDATE_PROB);
                    if (tmp) {
                        uint8_t *prob = &fc->a.tx32x32_prob[i][j];
                        *prob = vp9hwdReadProbDiffUpdate( *prob);
                    }
                }
            }
        }
    }

    // Coefficient probability update
    tmp = vp9hwdDecodeCoeffUpdate( fc->a.probCoeffs);

    if( tmp != OK ) return (tmp);
    if (pProbSetup->transform_mode > ONLY_4X4) {
        tmp = vp9hwdDecodeCoeffUpdate( fc->a.probCoeffs8x8);
        if( tmp != OK ) return (tmp);
    }
    if (pProbSetup->transform_mode > ALLOW_8X8) {
        tmp = vp9hwdDecodeCoeffUpdate( fc->a.probCoeffs16x16);
        if( tmp != OK ) return (tmp);
    }
    if (pProbSetup->transform_mode > ALLOW_16X16) {
        tmp = vp9hwdDecodeCoeffUpdate( fc->a.probCoeffs32x32);
        if( tmp != OK ) return (tmp);
    }

    pProbSetup->probsDecoded = 1;

    for (k = 0; k < MBSKIP_CONTEXTS; ++k) {
        tmp = vp9_read( VP9_DEF_UPDATE_PROB);
        if (tmp) {
            fc->a.mbskip_probs[k] = vp9hwdReadProbDiffUpdate( fc->a.mbskip_probs[k]);
        }
    }

    if(!pProbSetup->keyFrame)
    {
        for (i = 0; i < INTER_MODE_CONTEXTS; i++) {
            for (j = 0; j < VP9_INTER_MODES - 1; j++) {
                tmp = vp9_read( VP9_DEF_UPDATE_PROB);
                if (tmp) {
                    uint8_t *prob = &fc->a.inter_mode_prob[i][j];
                    *prob = vp9hwdReadProbDiffUpdate( *prob);
                }
            }
        }
        if (pProbSetup->mcomp_filter_type == SWITCHABLE) {
            for (j = 0; j < VP9_SWITCHABLE_FILTERS+1; ++j) {
                for (i = 0; i < VP9_SWITCHABLE_FILTERS-1; ++i) {
                    tmp = vp9_read( VP9_DEF_UPDATE_PROB);
                    if (tmp) {
                        uint8_t *prob = &fc->a.switchable_interp_prob[j][i];
                        *prob = vp9hwdReadProbDiffUpdate( *prob);
                    }
                }
            }
        }

        for (i = 0; i < INTRA_INTER_CONTEXTS; i++) {
            tmp = vp9_read( VP9_DEF_UPDATE_PROB);
            if (tmp) {
                uint8_t *prob = &fc->a.intra_inter_prob[i];
                *prob = vp9hwdReadProbDiffUpdate( *prob);
            }
        }

        // Compound prediction mode probabilities
        if (pProbSetup->allow_comp_inter_inter) {
            tmp = vp9_read_literal( 1);
            pProbSetup->comp_pred_mode = tmp;
            if(tmp) {
                tmp = vp9_read_literal( 1);
                pProbSetup->comp_pred_mode += tmp;
                if (pProbSetup->comp_pred_mode == HYBRID_PREDICTION)
                {
                    for (i = 0; i < COMP_INTER_CONTEXTS; i++)
                    {
                        tmp = vp9_read( VP9_DEF_UPDATE_PROB);
                        if (tmp) {
                            uint8_t *prob = &fc->a.comp_inter_prob[i];
                            *prob = vp9hwdReadProbDiffUpdate( *prob);
                        }
                    }
                }
            }
        } else {
            pProbSetup->comp_pred_mode = SINGLE_PREDICTION_ONLY;
        }

        if (pProbSetup->comp_pred_mode != COMP_PREDICTION_ONLY) {
            for (i = 0; i < REF_CONTEXTS; i++) {
                tmp = vp9_read( VP9_DEF_UPDATE_PROB);
                if (tmp) {
                    uint8_t *prob = &fc->a.single_ref_prob[i][0];
                    *prob = vp9hwdReadProbDiffUpdate( *prob);
                }
                tmp = vp9_read( VP9_DEF_UPDATE_PROB);
                if (tmp) {
                    uint8_t *prob = &fc->a.single_ref_prob[i][1];
                    *prob = vp9hwdReadProbDiffUpdate( *prob);
                }
            }
        }

        if (pProbSetup->comp_pred_mode != SINGLE_PREDICTION_ONLY) {
            for (i = 0; i < REF_CONTEXTS; i++) {
                tmp = vp9_read( VP9_DEF_UPDATE_PROB);
                if (tmp) {
                    uint8_t *prob = &fc->a.comp_ref_prob[i];
                    *prob = vp9hwdReadProbDiffUpdate( *prob);
                }
            }
        }

        // Superblock intra luma pred mode probabilities
        for(j = 0 ; j < BLOCK_SIZE_GROUPS; ++j)
        {
            for( i = 0 ; i < 8; ++i ) {
                tmp = vp9_read( VP9_DEF_UPDATE_PROB);
                if (tmp) {
                    fc->a.sb_ymode_prob[j][i] = vp9hwdReadProbDiffUpdate(
                            fc->a.sb_ymode_prob[j][i]);
                }
            }
            tmp = vp9_read( VP9_DEF_UPDATE_PROB);
            if (tmp) {
                fc->a.sb_ymode_probB[j][0] = vp9hwdReadProbDiffUpdate(
                        fc->a.sb_ymode_probB[j][0]);
            }
        }

        for (j = 0; j < NUM_PARTITION_CONTEXTS; j++) {
            for (i = 0; i < PARTITION_TYPES - 1; i++) {
                tmp = vp9_read( VP9_DEF_UPDATE_PROB);
                if (tmp) {
                    uint8_t *prob = &fc->a.partition_prob[INTER_FRAME][j][i];
                    *prob = vp9hwdReadProbDiffUpdate( *prob);
                }
            }
        }

        // Motion vector tree update
        tmp = vp9hwdDecodeMvUpdate(pProbSetup);
        if( tmp != OK )
            return (tmp);
    }

    return (OK);
}

void VulkanVP9Decoder::update_nmv( vp9_prob *const p, const vp9_prob upd_p)
{
    uint32_t tmp = vp9_read( upd_p);
    if (tmp) {
#if 1 //def LOW_PRECISION_MV_UPDATE
        *p = (vp9_read_literal( 7) << 1) | 1;
#else
        *p = vp9_read_literal( 8);
#endif
    }
}

uint32_t VulkanVP9Decoder::vp9hwdDecodeMvUpdate(vp9_prob_update_s *pProbSetup)
{
    uint32_t i, j, k;
    nvdec_nmv_context *mvctx = &pProbSetup->pProbTab->a.nmvc;

#if 0
    tmp = vp9_read_literal( 1);
    if (!tmp) return HANTRO_OK;
#endif

    for (j = 0; j < MV_JOINTS - 1; ++j) {
      update_nmv( &mvctx->joints[j],
                 VP9_NMV_UPDATE_PROB);
    }
    for (i = 0; i < 2; ++i) {
      update_nmv( &mvctx->sign[i], VP9_NMV_UPDATE_PROB);
      for (j = 0; j < MV_CLASSES - 1; ++j) {
        update_nmv( &mvctx->classes[i][j], VP9_NMV_UPDATE_PROB);
      }
      for (j = 0; j < CLASS0_SIZE - 1; ++j) {
        update_nmv( &mvctx->class0[i][j], VP9_NMV_UPDATE_PROB);
      }
      for (j = 0; j < MV_OFFSET_BITS; ++j) {
        update_nmv( &mvctx->bits[i][j], VP9_NMV_UPDATE_PROB);
      }
    }

    for (i = 0; i < 2; ++i) {
      for (j = 0; j < CLASS0_SIZE; ++j) {
        for (k = 0; k < 3; ++k)
          update_nmv( &mvctx->class0_fp[i][j][k], VP9_NMV_UPDATE_PROB);
      }
      for (j = 0; j < 3; ++j) {
        update_nmv( &mvctx->fp[i][j], VP9_NMV_UPDATE_PROB);
      }
    }

    if (pProbSetup->allow_high_precision_mv) {
      for (i = 0; i < 2; ++i) {
        update_nmv( &mvctx->class0_hp[i], VP9_NMV_UPDATE_PROB);
        update_nmv( &mvctx->hp[i], VP9_NMV_UPDATE_PROB);
      }
    }

    return (OK);
}

uint32_t  VulkanVP9Decoder::vp9hwdDecodeCoeffUpdate(
        uint8_t probCoeffs[VP9_BLOCK_TYPES][VP9_REF_TYPES][VP9_COEF_BANDS][VP9_PREV_COEF_CONTEXTS][ENTROPY_NODES_PART1])
{
    uint32_t i, j, k, l, m;
    uint32_t tmp;
    tmp = vp9_read_literal( 1);
    if (!tmp) return OK;
    for( i = 0; i < VP9_BLOCK_TYPES; i++ )
    {
        for ( j = 0; j < VP9_REF_TYPES; j++ )
        {
            for ( k = 0; k < VP9_COEF_BANDS; k++ )
            {
                for ( l = 0; l < VP9_PREV_COEF_CONTEXTS; l++ )
                {
                    if (l >= 3 && k == 0)
                        continue;

                    for ( m = 0; m < UNCONSTRAINED_NODES; m++ )
                    {
                        tmp = vp9_read( 252);
                        CHECK_END_OF_STREAM(tmp);
                        if ( tmp )
                        {
                            uint8_t old, latest;
                            old = probCoeffs[i][j][k][l][m];
                            latest = vp9hwdReadProbDiffUpdate( old);
                            CHECK_END_OF_STREAM(tmp);

                            probCoeffs[i][j][k][l][m] = latest;
                        }
                    }
                }
            }
        }
    }
    return (OK);
}

int32_t VulkanVP9Decoder::get_unsigned_bits(uint32_t num_values)
{
    int32_t cat = 0;
    if (num_values <= 1)
        return 0;
    num_values--;
    while(num_values > 0)
    {
        cat++;
        num_values >>= 1;
    }
    return cat;
}

uint32_t VulkanVP9Decoder::BoolDecodeUniform( uint32_t n)
{
    int32_t value, v;
    int32_t l = get_unsigned_bits(n);
    int32_t m = (1 << l) - n;
    if (!l) return 0;
    value = vp9_read_literal( l - 1);
    if (value >= m) {
        v = vp9_read_literal( 1);
        value = (value << 1) - m + v;
    }
    return value;
}

uint32_t VulkanVP9Decoder::vp9hwdDecodeSubExp( uint32_t k, uint32_t num_syms)
{
    uint32_t i=0, mk=0, value=0;
    while (1) {
        int32_t b = (i ? k + i - 1 : k);
        uint32_t a = (1 << b);
        if (num_syms <= mk + 3 * a) {
            value = BoolDecodeUniform( num_syms - mk) + mk;
            break;
        } else {
            value = vp9_read_bit();
            if (value) {
                i++;
                mk += a;
            } else {
                value = vp9_read_literal( b) + mk;
                break;
            }
        }
    }
    return value;
}

int32_t VulkanVP9Decoder::merge_index(int32_t v, int32_t n, int32_t modulus)
{
    int32_t max1 = (n - 1 - modulus / 2) / modulus + 1;
    if (v < max1) v = v * modulus + modulus / 2;
    else
    {
        int32_t w;
        v -= max1;
        w = v;
        v += (v + modulus - modulus / 2) / modulus;
        while (v % modulus == modulus / 2 ||
            w != v - (v + modulus - modulus / 2) / modulus) v++;
    }
    return v;
}

int32_t VulkanVP9Decoder::vp9_inv_recenter_nonneg(int32_t v, int32_t m)
{
    if (v > (m << 1)) return v;
    else if ((v & 1) == 0) return (v >> 1) + m;
    else return m - ((v + 1) >> 1);
}

int32_t VulkanVP9Decoder::inv_remap_prob(int32_t v, int32_t m)
{
    const int32_t n = 255;
    v = merge_index(v, n - 1, MODULUS_PARAM);
    m--;
    if ((m << 1) <= n)
        return 1 + vp9_inv_recenter_nonneg(v + 1, m);
    else
        return n - vp9_inv_recenter_nonneg(v + 1, n - 1 - m);
}

vp9_prob VulkanVP9Decoder::vp9hwdReadProbDiffUpdate( uint8_t oldp)
{
    int32_t p;
    int32_t delp = vp9hwdDecodeSubExp( 4, 255 );
    p = (vp9_prob)inv_remap_prob(delp, oldp);
    return p;
}

//Backward update


// this function assumes prob1 and prob2 are already within [1,255] range
vp9_prob VulkanVP9Decoder::weighted_prob(int32_t prob1, int32_t prob2, int32_t factor)
{
    return ROUND_POWER_OF_TWO(prob1 * (256 - factor) + prob2 * factor, 8);
}

vp9_prob VulkanVP9Decoder::clip_prob(uint32_t p)
{
    return (vp9_prob)((p > 255) ? 255u : (p < 1) ? 1u : p);
}

vp9_prob VulkanVP9Decoder::get_prob(uint32_t num, uint32_t den)
{
    return (den == 0) ? 128u : clip_prob((num * 256 + (den >> 1)) / den);
}

vp9_prob VulkanVP9Decoder::get_binary_prob(uint32_t n0, uint32_t n1)
{
    return get_prob(n0, n0 + n1);
}

uint32_t VulkanVP9Decoder::convert_distribution(uint32_t i,
                            const vp9_tree_index * tree,
                            vp9_prob probs[],
                            uint32_t branch_ct[][2],
                            const uint32_t num_events[],
                            uint32_t tok0_offset)
{
    uint32_t left, right;

    if (tree[i] <= 0)
    {
        left = num_events[-tree[i] - tok0_offset];
    }
    else
    {
        left = convert_distribution(tree[i], tree, probs, branch_ct, num_events, tok0_offset);
    }
    if (tree[i + 1] <= 0)
    {
        right = num_events[-tree[i + 1] - tok0_offset];
    }
    else
    {
        right = convert_distribution(tree[i + 1], tree, probs, branch_ct, num_events, tok0_offset);
    }
    probs[i>>1] = get_binary_prob(left, right);
    branch_ct[i>>1][0] = left;
    branch_ct[i>>1][1] = right;
    return left + right;
}

void VulkanVP9Decoder::vp9_tree_probs_from_distribution(const vp9_tree_index * tree,
                                        vp9_prob probs          [ /* n-1 */ ],
                                        uint32_t branch_ct       [ /* n-1 */ ] [2],
                                        const uint32_t num_events[ /* n */ ],
                                        uint32_t tok0_offset)
{
    convert_distribution(0, tree, probs, branch_ct, num_events, tok0_offset);
}

void VulkanVP9Decoder::update_coef_probs(uint8_t dst_coef_probs[VP9_BLOCK_TYPES][VP9_REF_TYPES][VP9_COEF_BANDS][VP9_PREV_COEF_CONTEXTS][ENTROPY_NODES_PART1],
                        uint8_t pre_coef_probs[VP9_BLOCK_TYPES][VP9_REF_TYPES][VP9_COEF_BANDS][VP9_PREV_COEF_CONTEXTS][ENTROPY_NODES_PART1],
                        uint32_t coef_counts[VP9_BLOCK_TYPES][VP9_REF_TYPES][VP9_COEF_BANDS][VP9_PREV_COEF_CONTEXTS][UNCONSTRAINED_NODES+1],
                        uint32_t (*eob_counts)[VP9_REF_TYPES][VP9_COEF_BANDS][VP9_PREV_COEF_CONTEXTS],
                        int32_t count_sat, int32_t update_factor)
{
    int32_t t, i, j, k, l, count;
    uint32_t branch_ct[VP9_ENTROPY_NODES][2];
    vp9_prob coef_probs[VP9_ENTROPY_NODES];
    int32_t factor;

    //int32_t brancharr[VP9_BLOCK_TYPES][VP9_REF_TYPES][36][VP9_PREV_COEF_CONTEXTS] = {0};
    //int32_t coeffprobarr[VP9_BLOCK_TYPES][VP9_REF_TYPES][VP9_COEF_BANDS][VP9_PREV_COEF_CONTEXTS] = {0};
    //memset(brancharr, 0, sizeof(int32_t)*VP9_BLOCK_TYPES*VP9_REF_TYPES*VP9_COEF_BANDS*VP9_PREV_COEF_CONTEXTS);
    //memset(coeffprobarr, 0, sizeof(int32_t)*VP9_BLOCK_TYPES*VP9_REF_TYPES*VP9_COEF_BANDS*VP9_PREV_COEF_CONTEXTS);

    for (i = 0; i < VP9_BLOCK_TYPES; ++i)
    {
        for (j = 0; j < VP9_REF_TYPES; ++j)
        {
            for (k = 0; k < VP9_COEF_BANDS; ++k)
            {
                for (l = 0; l < VP9_PREV_COEF_CONTEXTS; ++l)
                {
                    if (l >= 3 && k == 0)
                        continue;
                    vp9_tree_probs_from_distribution(vp9_coefmodel_tree,
                                                    coef_probs, branch_ct,
                                                     coef_counts[i][j][k][l], 0);
                    branch_ct[0][1] = eob_counts[i][j][k][l] - branch_ct[0][0];
                    coef_probs[0] = get_binary_prob(branch_ct[0][0], branch_ct[0][1]);
                    //brancharr[i][j][k][l] = branch_ct[0][1];
                    //coeffprobarr[i][j][k][l] = coef_probs[0];
                    for (t = 0; t < UNCONSTRAINED_NODES; ++t)
                    {
                        count = branch_ct[t][0] + branch_ct[t][1];
                        count = count > count_sat ? count_sat : count;
                        factor = (update_factor * count / count_sat);
                        dst_coef_probs[i][j][k][l][t] = weighted_prob(pre_coef_probs[i][j][k][l][t], coef_probs[t], factor);
                    }
                }
            }
        }
    }
}

void VulkanVP9Decoder::adaptCoefProbs(vp9_prob_update_s *pProbSetup)
{
    int32_t update_factor; /* denominator 256 */
    int32_t count_sat;

    if(pProbSetup->keyFrame)
    {
        update_factor = COEF_MAX_UPDATE_FACTOR_KEY;
        count_sat = COEF_COUNT_SAT_KEY;
    }
    else if (pProbSetup->prevIsKeyFrame)
    {
        update_factor = COEF_MAX_UPDATE_FACTOR_AFTER_KEY; // adapt quickly
        count_sat = COEF_COUNT_SAT_AFTER_KEY;
    }
    else
    {
        update_factor = COEF_MAX_UPDATE_FACTOR;
        count_sat = COEF_COUNT_SAT;
    }

    update_coef_probs(pProbSetup->pProbTab->a.probCoeffs,
                        m_PrevCtx.probCoeffs,
                        pProbSetup->pCtxCounters->countCoeffs,
                        pProbSetup->pCtxCounters->countEobs[TX_4X4],
                        count_sat, update_factor);
    update_coef_probs(pProbSetup->pProbTab->a.probCoeffs8x8,
                        m_PrevCtx.probCoeffs8x8,
                        pProbSetup->pCtxCounters->countCoeffs8x8,
                        pProbSetup->pCtxCounters->countEobs[TX_8X8],
                        count_sat, update_factor);
    update_coef_probs(pProbSetup->pProbTab->a.probCoeffs16x16,
                        m_PrevCtx.probCoeffs16x16,
                        pProbSetup->pCtxCounters->countCoeffs16x16,
                        pProbSetup->pCtxCounters->countEobs[TX_16X16],
                        count_sat, update_factor);
    update_coef_probs(pProbSetup->pProbTab->a.probCoeffs32x32,
                        m_PrevCtx.probCoeffs32x32,
                        pProbSetup->pCtxCounters->countCoeffs32x32,
                        pProbSetup->pCtxCounters->countEobs[TX_32X32],
                        count_sat, update_factor);
}

int32_t VulkanVP9Decoder::update_mode_ct(vp9_prob pre_prob, vp9_prob prob, uint32_t branch_ct[2])
{
    int32_t factor, count = branch_ct[0] + branch_ct[1];
    count = count > MODE_COUNT_SAT ? MODE_COUNT_SAT : count;
    factor = (MODE_MAX_UPDATE_FACTOR * count / MODE_COUNT_SAT);
    return weighted_prob(pre_prob, prob, factor);
}

int32_t VulkanVP9Decoder::update_mode_ct2(vp9_prob pre_prob, uint32_t branch_ct[2])
{
    return update_mode_ct(pre_prob, get_binary_prob(branch_ct[0], branch_ct[1]), branch_ct);
}

void VulkanVP9Decoder::update_mode_probs(int32_t n_modes,
                        const vp9_tree_index *tree, uint32_t *cnt,
                        vp9_prob *pre_probs, vp9_prob *pre_probsB,
                        vp9_prob *dst_probs, vp9_prob *dst_probsB,
                        uint32_t tok0_offset)
{
    vp9_prob probs[MAX_PROBS];
    uint32_t branch_ct[MAX_PROBS][2];
    int32_t t, count, factor;

    assert(n_modes - 1 < MAX_PROBS);
    vp9_tree_probs_from_distribution(tree, probs, branch_ct, cnt, tok0_offset);
    for (t = 0; t < n_modes - 1; ++t)
    {
        count = branch_ct[t][0] + branch_ct[t][1];
        count = count > MODE_COUNT_SAT ? MODE_COUNT_SAT : count;
        factor = (MODE_MAX_UPDATE_FACTOR * count / MODE_COUNT_SAT);
        if (t < 8 || dst_probsB == NULL)
            dst_probs[t] = weighted_prob(pre_probs[t], probs[t], factor);
        else
            dst_probsB[t-8] = weighted_prob(pre_probsB[t-8], probs[t], factor);
    }
}

void VulkanVP9Decoder::tx_counts_to_branch_counts_32x32(uint32_t *tx_count_32x32p,
                                      uint32_t (*ct_32x32p)[2])
{
    ct_32x32p[0][0] = tx_count_32x32p[TX_4X4];
    ct_32x32p[0][1] = tx_count_32x32p[TX_8X8] + tx_count_32x32p[TX_16X16] + tx_count_32x32p[TX_32X32];
    ct_32x32p[1][0] = tx_count_32x32p[TX_8X8];
    ct_32x32p[1][1] = tx_count_32x32p[TX_16X16] + tx_count_32x32p[TX_32X32];
    ct_32x32p[2][0] = tx_count_32x32p[TX_16X16];
    ct_32x32p[2][1] = tx_count_32x32p[TX_32X32];
}

void VulkanVP9Decoder::tx_counts_to_branch_counts_16x16(uint32_t *tx_count_16x16p,
                                      uint32_t (*ct_16x16p)[2])
{
    ct_16x16p[0][0] = tx_count_16x16p[TX_4X4];
    ct_16x16p[0][1] = tx_count_16x16p[TX_8X8] + tx_count_16x16p[TX_16X16];
    ct_16x16p[1][0] = tx_count_16x16p[TX_8X8];
    ct_16x16p[1][1] = tx_count_16x16p[TX_16X16];
}

void VulkanVP9Decoder::tx_counts_to_branch_counts_8x8(uint32_t *tx_count_8x8p,
                                    uint32_t (*ct_8x8p)[2])
{
    ct_8x8p[0][0] =   tx_count_8x8p[TX_4X4];
    ct_8x8p[0][1] =   tx_count_8x8p[TX_8X8];
}

void VulkanVP9Decoder::adaptModeProbs(vp9_prob_update_s *pProbSetup)
{
    uint32_t i, j;

    for (i = 0; i < INTRA_INTER_CONTEXTS; i++)
        pProbSetup->pProbTab->a.intra_inter_prob[i] = update_mode_ct2(m_PrevCtx.intra_inter_prob[i], pProbSetup->pCtxCounters->intra_inter_count[i]);
    for (i = 0; i < COMP_INTER_CONTEXTS; i++)
        pProbSetup->pProbTab->a.comp_inter_prob[i] = update_mode_ct2(m_PrevCtx.comp_inter_prob[i], pProbSetup->pCtxCounters->comp_inter_count[i]);
    for (i = 0; i < REF_CONTEXTS; i++)
        pProbSetup->pProbTab->a.comp_ref_prob[i] = update_mode_ct2(m_PrevCtx.comp_ref_prob[i], pProbSetup->pCtxCounters->comp_ref_count[i]);
    for (i = 0; i < REF_CONTEXTS; i++)
        for (j = 0; j < 2; j++)
            pProbSetup->pProbTab->a.single_ref_prob[i][j] = update_mode_ct2(m_PrevCtx.single_ref_prob[i][j], pProbSetup->pCtxCounters->single_ref_count[i][j]);

    for (i = 0; i < BLOCK_SIZE_GROUPS; ++i)
    {
        update_mode_probs(VP9_INTRA_MODES, vp9_intra_mode_tree,
                            pProbSetup->pCtxCounters->sb_ymode_counts[i],
                            m_PrevCtx.sb_ymode_prob[i], m_PrevCtx.sb_ymode_probB[i],
                            pProbSetup->pProbTab->a.sb_ymode_prob[i], pProbSetup->pProbTab->a.sb_ymode_probB[i], 0);
    }
    for (i = 0; i < VP9_INTRA_MODES; ++i)
    {
        update_mode_probs(VP9_INTRA_MODES, vp9_intra_mode_tree,
                            pProbSetup->pCtxCounters->uv_mode_counts[i],
                            m_PrevCtx.uv_mode_prob[i],
                            m_PrevCtx.uv_mode_probB[i],
                            pProbSetup->pProbTab->a.uv_mode_prob[i],
                            pProbSetup->pProbTab->a.uv_mode_probB[i], 0);
    }
    for (i = 0; i < NUM_PARTITION_CONTEXTS; i++)
        update_mode_probs(PARTITION_TYPES, vp9_partition_tree,
                            pProbSetup->pCtxCounters->partition_counts[i],
                            m_PrevCtx.partition_prob[INTER_FRAME][i], NULL,
                            pProbSetup->pProbTab->a.partition_prob[INTER_FRAME][i], NULL, 0);

    if (pProbSetup->mcomp_filter_type == SWITCHABLE)
    {
        for (i = 0; i <= VP9_SWITCHABLE_FILTERS; ++i)
        {
            update_mode_probs(VP9_SWITCHABLE_FILTERS, vp9_switchable_interp_tree,
                                pProbSetup->pCtxCounters->switchable_interp_counts[i],
                                m_PrevCtx.switchable_interp_prob[i], NULL,
                                pProbSetup->pProbTab->a.switchable_interp_prob[i], NULL, 0);
        }
    }

    if (pProbSetup->transform_mode == TX_MODE_SELECT)
    {
        uint32_t branch_ct_8x8p[TX_SIZE_MAX_SB - 3][2];
        uint32_t branch_ct_16x16p[TX_SIZE_MAX_SB - 2][2];
        uint32_t branch_ct_32x32p[TX_SIZE_MAX_SB - 1][2];
        for (i = 0; i < TX_SIZE_CONTEXTS; ++i)
        {
            tx_counts_to_branch_counts_8x8(pProbSetup->pCtxCounters->tx8x8_count[i], branch_ct_8x8p);
            for (j = 0; j < TX_SIZE_MAX_SB - 3; ++j)
            {
                int32_t factor;
                int32_t count = branch_ct_8x8p[j][0] + branch_ct_8x8p[j][1];
                vp9_prob prob = get_binary_prob(branch_ct_8x8p[j][0], branch_ct_8x8p[j][1]);
                count = count > MODE_COUNT_SAT ? MODE_COUNT_SAT : count;
                factor = (MODE_MAX_UPDATE_FACTOR * count / MODE_COUNT_SAT);
                pProbSetup->pProbTab->a.tx8x8_prob[i][j] = weighted_prob(m_PrevCtx.tx8x8_prob[i][j], prob, factor);
            }
        }
        for (i = 0; i < TX_SIZE_CONTEXTS; ++i)
        {
            tx_counts_to_branch_counts_16x16(pProbSetup->pCtxCounters->tx16x16_count[i], branch_ct_16x16p);
            for (j = 0; j < TX_SIZE_MAX_SB - 2; ++j)
            {
                int32_t factor;
                int32_t count = branch_ct_16x16p[j][0] + branch_ct_16x16p[j][1];
                vp9_prob prob = get_binary_prob(branch_ct_16x16p[j][0], branch_ct_16x16p[j][1]);
                count = count > MODE_COUNT_SAT ? MODE_COUNT_SAT : count;
                factor = (MODE_MAX_UPDATE_FACTOR * count / MODE_COUNT_SAT);
                pProbSetup->pProbTab->a.tx16x16_prob[i][j] = weighted_prob(m_PrevCtx.tx16x16_prob[i][j], prob, factor);
            }
        }
        for (i = 0; i < TX_SIZE_CONTEXTS; ++i)
        {
            tx_counts_to_branch_counts_32x32(pProbSetup->pCtxCounters->tx32x32_count[i], branch_ct_32x32p);
            for (j = 0; j < TX_SIZE_MAX_SB - 1; ++j)
            {
                int32_t factor;
                int32_t count = branch_ct_32x32p[j][0] + branch_ct_32x32p[j][1];
                vp9_prob prob = get_binary_prob(branch_ct_32x32p[j][0], branch_ct_32x32p[j][1]);
                count = count > MODE_COUNT_SAT ? MODE_COUNT_SAT : count;
                factor = (MODE_MAX_UPDATE_FACTOR * count / MODE_COUNT_SAT);
                pProbSetup->pProbTab->a.tx32x32_prob[i][j] = weighted_prob(m_PrevCtx.tx32x32_prob[i][j], prob, factor);
            }
        }
    }
    for (i = 0; i < MBSKIP_CONTEXTS; ++i)
        pProbSetup->pProbTab->a.mbskip_probs[i] = update_mode_ct2(m_PrevCtx.mbskip_probs[i],pProbSetup->pCtxCounters->mbskip_count[i]);
}

void VulkanVP9Decoder::adaptModeContext(vp9_prob_update_s *pProbSetup)
{
    uint32_t i, j;
    uint32_t (*mode_ct)[VP9_INTER_MODES - 1][2] = pProbSetup->pCtxCounters->inter_mode_counts;

    for (j = 0; j < INTER_MODE_CONTEXTS; j++)
    {
        for (i = 0; i < VP9_INTER_MODES - 1; i++)
        {
            int32_t count = mode_ct[j][i][0] + mode_ct[j][i][1], factor;
            count = count > MVREF_COUNT_SAT ? MVREF_COUNT_SAT : count;
            factor = (MVREF_MAX_UPDATE_FACTOR * count / MVREF_COUNT_SAT);
            pProbSetup->pProbTab->a.inter_mode_prob[j][i] = weighted_prob(m_PrevCtx.inter_mode_prob[j][i],
                                                                        get_binary_prob(mode_ct[j][i][0], mode_ct[j][i][1]),
                                                                        factor);
        }
    }
}

uint32_t VulkanVP9Decoder::adapt_probs(uint32_t i,
                            const signed char* tree,
                            vp9_prob this_probs[],
                            const vp9_prob last_probs[],
                            const uint32_t num_events[])
{
    vp9_prob this_prob;
    uint32_t weight;

    const uint32_t left = tree[i] <= 0 ? num_events[-tree[i]] : adapt_probs(tree[i], tree, this_probs, last_probs, num_events);
    const uint32_t right = tree[i + 1] <= 0 ? num_events[-tree[i + 1]] : adapt_probs(tree[i + 1], tree, this_probs, last_probs, num_events);
    weight = left + right;
    if (weight)
    {
        this_prob = get_binary_prob(left, right);
        weight = weight > MV_COUNT_SAT ? MV_COUNT_SAT : weight;
        this_prob = weighted_prob(last_probs[i >> 1], this_prob, MV_MAX_UPDATE_FACTOR * weight / MV_COUNT_SAT);
    }
    else
    {
        this_prob = last_probs[i >> 1];
    }
    this_probs[i >> 1] = this_prob;
    return left + right;
}

void VulkanVP9Decoder::adapt_prob(vp9_prob *dest, vp9_prob prep, uint32_t ct[2])
{
    const int32_t count = std::min<int32_t>(ct[0] + ct[1], MV_COUNT_SAT);
    if (count)
    {
        const vp9_prob newp = get_binary_prob(ct[0], ct[1]);
        const int32_t factor = MV_MAX_UPDATE_FACTOR * count / MV_COUNT_SAT;
        *dest = weighted_prob(prep, newp, factor);
    }
    else
        *dest = prep;
}

void VulkanVP9Decoder::adaptNmvProbs(vp9_prob_update_s *pProbSetup)
{
    uint32_t usehp = pProbSetup->allow_high_precision_mv;
    uint32_t i, j;

    adapt_probs(0, vp9_mv_joint_tree,
                pProbSetup->pProbTab->a.nmvc.joints,
                m_PrevCtx.nmvc.joints,
                pProbSetup->pCtxCounters->nmvcount.joints);
    for (i = 0; i < 2; ++i)
    {
        adapt_prob(&pProbSetup->pProbTab->a.nmvc.sign[i],
                    m_PrevCtx.nmvc.sign[i],
                    pProbSetup->pCtxCounters->nmvcount.sign[i]);
        adapt_probs(0, vp9_mv_class_tree,
                    pProbSetup->pProbTab->a.nmvc.classes[i],
                    m_PrevCtx.nmvc.classes[i],
                    pProbSetup->pCtxCounters->nmvcount.classes[i]);
        adapt_probs(0, vp9_mv_class0_tree,
                    pProbSetup->pProbTab->a.nmvc.class0[i],
                    m_PrevCtx.nmvc.class0[i],
                    pProbSetup->pCtxCounters->nmvcount.class0[i]);
        for (j = 0; j < MV_OFFSET_BITS; ++j)
        {
            adapt_prob(&pProbSetup->pProbTab->a.nmvc.bits[i][j],
            m_PrevCtx.nmvc.bits[i][j],
            pProbSetup->pCtxCounters->nmvcount.bits[i][j]);
        }
        for (j = 0; j < CLASS0_SIZE; ++j)
        {
        adapt_probs(0, vp9_mv_fp_tree,
                    pProbSetup->pProbTab->a.nmvc.class0_fp[i][j],
                    m_PrevCtx.nmvc.class0_fp[i][j],
                    pProbSetup->pCtxCounters->nmvcount.class0_fp[i][j]);
        }
        adapt_probs(0, vp9_mv_fp_tree,
                    pProbSetup->pProbTab->a.nmvc.fp[i],
                    m_PrevCtx.nmvc.fp[i],
                    pProbSetup->pCtxCounters->nmvcount.fp[i]);
    }
    if (usehp)
    {
        for (i = 0; i < 2; ++i)
        {
            adapt_prob(&pProbSetup->pProbTab->a.nmvc.class0_hp[i],
                        m_PrevCtx.nmvc.class0_hp[i],
                        pProbSetup->pCtxCounters->nmvcount.class0_hp[i]);
            adapt_prob(&pProbSetup->pProbTab->a.nmvc.hp[i],
                        m_PrevCtx.nmvc.hp[i],
                        pProbSetup->pCtxCounters->nmvcount.hp[i]);
        }
    }
}

void VulkanVP9Decoder::UpdateBackwardProbability(vp9_prob_update_s *pProbSetup)
{
    if (!pProbSetup->errorResilient && !pProbSetup->FrameParallelDecoding)
    {
        adaptCoefProbs(pProbSetup); //vp9_adapt_coef_probs
        if(!pProbSetup->keyFrame && !pProbSetup->intraOnly)
        {
            adaptModeProbs(pProbSetup); //vp9_adapt_mode_probs
            adaptModeContext(pProbSetup);
            adaptNmvProbs(pProbSetup); //vp9_adapt_mv_probs
        }
    }
    //vp9hwdStoreProbs
    if (pProbSetup->RefreshEntropyProbs)
    {
        memcpy(&m_EntropyLast[pProbSetup->frameContextIdx], pProbSetup->pProbTab, sizeof(m_EntropyLast[pProbSetup->frameContextIdx]));
    }
    //VP9HwdUpdateRefs
}
