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

#ifndef _VP9_PROBMANAGER_H_
#define _VP9_PROBMANAGER_H_

#include <stdint.h>
#include <climits>

#include "VulkanVideoDecoder.h"

typedef enum {
  EIGHTTAP_SMOOTH,
  EIGHTTAP,
  EIGHTTAP_SHARP,
  BILINEAR,
  SWITCHABLE  /* should be the last one */
} INTERPOLATIONFILTERTYPE;

typedef enum {
  //NONE = -1,
  INTRA_FRAME = 0,
  LAST_FRAME = 1,
  GOLDEN_FRAME = 2,
  ALTREF_FRAME = 3,
  VP9_MAX_REF_FRAMES = 4
}MV_REFERENCE_FRAME;

typedef enum {
  ONLY_4X4            = 0,
  ALLOW_8X8           = 1,
  ALLOW_16X16         = 2,
  ALLOW_32X32         = 3,
  TX_MODE_SELECT      = 4,
  NB_TXFM_MODES       = 5,
} TXFM_MODE;

// Segment level features.
typedef enum {
  SEG_LVL_ALT_Q = 0,               // Use alternate Quantizer ....
  SEG_LVL_ALT_LF = 1,              // Use alternate loop filter value...
  SEG_LVL_REF_FRAME = 2,           // Optional Segment reference frame
  SEG_LVL_SKIP = 3,                // Optional Segment (0,0) + skip mode
  SEG_LVL_MAX = 4                  // Number of MB level features supported
} SEG_LVL_FEATURES;

typedef enum {
  SINGLE_PREDICTION_ONLY = 0,
  COMP_PREDICTION_ONLY   = 1,
  HYBRID_PREDICTION      = 2,
  NB_PREDICTION_TYPES    = 3,
} COMPPREDMODE_TYPE;

/* Symbols for coding which components are zero jointly */
typedef enum {
  MV_JOINT_ZERO = 0,             /* Zero vector */
  MV_JOINT_HNZVZ = 1,            /* Vert zero, hor nonzero */
  MV_JOINT_HZVNZ = 2,            /* Hor zero, vert nonzero */
  MV_JOINT_HNZVNZ = 3,           /* Both components nonzero */
} MV_JOINT_TYPE;

/* Symbols for coding magnitude class of nonzero components */
typedef enum {
  MV_CLASS_0 = 0,      /* (0, 2]     integer pel */
  MV_CLASS_1 = 1,      /* (2, 4]     integer pel */
  MV_CLASS_2 = 2,      /* (4, 8]     integer pel */
  MV_CLASS_3 = 3,      /* (8, 16]    integer pel */
  MV_CLASS_4 = 4,      /* (16, 32]   integer pel */
  MV_CLASS_5 = 5,      /* (32, 64]   integer pel */
  MV_CLASS_6 = 6,      /* (64, 128]  integer pel */
  MV_CLASS_7 = 7,      /* (128, 256] integer pel */
  MV_CLASS_8 = 8,      /* (256, 512] integer pel */
  MV_CLASS_9 = 9,      /* (512, 1024] integer pel */
  MV_CLASS_10 = 10,    /* (1024,2048] integer pel */
} MV_CLASS_TYPE;

typedef enum PARTITION_TYPE {
  PARTITION_NONE,
  PARTITION_HORZ,
  PARTITION_VERT,
  PARTITION_SPLIT,
  PARTITION_TYPES
} PARTITION_TYPE;


typedef enum
{
  DC_PRED,            /* average of above and left pixels */
  V_PRED,             /* vertical prediction */
  H_PRED,             /* horizontal prediction */
  D45_PRED,           /* Directional 45 deg prediction  [anti-clockwise from 0 deg hor] */
  D135_PRED,          /* Directional 135 deg prediction [anti-clockwise from 0 deg hor] */
  D117_PRED,          /* Directional 112 deg prediction [anti-clockwise from 0 deg hor] */
  D153_PRED,          /* Directional 157 deg prediction [anti-clockwise from 0 deg hor] */
  D27_PRED,           /* Directional 22 deg prediction  [anti-clockwise from 0 deg hor] */
  D63_PRED,           /* Directional 67 deg prediction  [anti-clockwise from 0 deg hor] */
  TM_PRED,            /* Truemotion prediction */
  NEARESTMV,
  NEARMV,
  ZEROMV,
  NEWMV,
  SPLITMV,
  MB_MODE_COUNT
} MB_PREDICTION_MODE;

typedef enum {
  KEY_FRAME = 0,
  INTER_FRAME = 1,
  NUM_FRAME_TYPES,
} FRAME_TYPE;

// Segment level features.
typedef enum {
  TX_4X4 = 0,                      // 4x4 dct transform
  TX_8X8 = 1,                      // 8x8 dct transform
  TX_16X16 = 2,                    // 16x16 dct transform
  TX_32X32 = 3,                    // 32x32 dct transform
  TX_SIZE_MAX_SB,                  // Number of transforms available to SBs
} TX_SIZE;

#define ROUND_POWER_OF_TWO(value, n) (((value) + (1 << ((n) - 1))) >> (n))

#define BIG_NUM 0xffff
#define MIN_TILE_WIDTH_B64 4
#define MAX_TILE_WIDTH_B64 64
#define MI_SIZE_LOG2 3
#define MI_BLOCK_SIZE_LOG2 (6 - MI_SIZE_LOG2)
#define ALIGN_POWER_OF_TWO(value, n) \
    (((value) + ((1 << (n)) - 1)) & ~((1 << (n)) - 1))
#define VP9_MB_LVL_MAX              2
#define VP9_MAX_MB_SEGMENTS         4
#define VP9_MB_FEATURE_TREE_PROBS   3
#define MAX_REF_LF_DELTAS       4
#define MAX_MODE_LF_DELTAS      2  //for vp8 its 4
#define ALLOWED_REFS_PER_FRAME  3
#define NUM_REF_FRAMES 8
#define NUM_REF_FRAMES_LG2 3
#define NUM_FRAME_CONTEXTS_LG2 2
#define MIN_TILE_WIDTH_SBS (MIN_TILE_WIDTH >> 6)
#define MIN_TILE_WIDTH 256
#define MAX_TILE_WIDTH_SBS (MAX_TILE_WIDTH >> 6)
//#define MAX_TILE_WIDTH 4096
#define MAX_MB_SEGMENTS 8
#define MB_SEG_TREE_PROBS  (MAX_MB_SEGMENTS-1)
#define MAX_PROB 255
#define PREDICTION_PROBS 3
#define TX_SIZE_CONTEXTS 2
#define PARTITION_PLOFFSET   4  // number of probability models per block size
#define NUM_PARTITION_CONTEXTS (4 * PARTITION_PLOFFSET)
#define BLOCK_SIZE_GROUPS   4
#define VP9_INTRA_MODES  10/* (TM_PRED + 1) */
#define COMP_PRED_CONTEXTS   2
/* Entropy nodes above is divided in two parts, first three probs in part1
 * and the modeled probs in part2. Part1 is padded so that tables align with
 *  32 byte addresses, so there is four bytes for each table. */
#define ENTROPY_NODES_PART1 4
#define ENTROPY_NODES_PART2 8
#define INTER_MODE_CONTEXTS     7
#define VP9_SWITCHABLE_FILTERS 3 /* number of switchable filters */
#define COMP_PRED_CONTEXTS   2
#define INTRA_INTER_CONTEXTS 4
#define COMP_INTER_CONTEXTS 5
#define REF_CONTEXTS 5
#define VP9_BLOCK_TYPES 2
#define VP9_REF_TYPES 2  // intra=0, inter=1
#define VP9_COEF_BANDS 6
#define VP9_PREV_COEF_CONTEXTS       6
#define MBSKIP_CONTEXTS 3
#define COEF_UPDATE_PROB 252
#define VP9_PROB_HALF 128
#define VP9_NMV_UPDATE_PROB  252
#define VP9_MV_UPDATE_PRECISION  7
#define MV_JOINTS     4
#define MV_CLASSES     11
#define CLASS0_BITS    1
#define CLASS0_SIZE    (1 << CLASS0_BITS)
#define MV_OFFSET_BITS (MV_CLASSES + CLASS0_BITS - 2)
/* The first nodes of the entropy probs are unconstrained, the rest are
 * modeled with statistic distribution. */
#define UNCONSTRAINED_NODES 3
#define MODEL_NODES                 (VP9_ENTROPY_NODES - UNCONSTRAINED_NODES)
#define PIVOT_NODE                  2   // which node is pivot
#define COEFPROB_MODELS             128
#define END_OF_STREAM 0xFFFFFFFFU
#define VP9_DEF_UPDATE_PROB 252
#define MODULUS_PARAM               13
#define OK   0 //HANTRO_OK
#define NOK  1 //HANTRO_NOK
#define CHECK_END_OF_STREAM(s) if((s)==END_OF_STREAM) return (s)
#define VP9_INTER_MODES (1 + NEWMV - NEARESTMV)
#define VP9_REF_LIST_SIZE   8
#define SEGMENT_DELTADATA 0
#define SEGMENT_ABSDATA 1
#define MAXQ 255
#define LOTS_OF_BITS 0x40000000
#define BD_VALUE_SIZE ((int32_t)sizeof(VP9_BD_VALUE)*CHAR_BIT)

#define VP9_ENTROPY_NODES 11
#define COEF_COUNT_SAT 24
#define COEF_MAX_UPDATE_FACTOR 112
#define COEF_COUNT_SAT_KEY 24
#define COEF_MAX_UPDATE_FACTOR_KEY 112
#define COEF_COUNT_SAT_AFTER_KEY 24
#define COEF_MAX_UPDATE_FACTOR_AFTER_KEY 128
#define MODE_COUNT_SAT 20
#define MODE_MAX_UPDATE_FACTOR 128
#define MAX_PROBS 32
#define MVREF_COUNT_SAT 20
#define MVREF_MAX_UPDATE_FACTOR 128
#define MV_COUNT_SAT 20
#define MV_MAX_UPDATE_FACTOR 128

/* Coefficient token alphabet */

#define ZERO_TOKEN              0       /* 0         Extra Bits 0+0 */
#define ONE_TOKEN               1       /* 1         Extra Bits 0+1 */
#define TWO_TOKEN               2       /* 2         Extra Bits 0+1 */
#define THREE_TOKEN             3       /* 3         Extra Bits 0+1 */
#define FOUR_TOKEN              4       /* 4         Extra Bits 0+1 */
#define DCT_VAL_CATEGORY1       5       /* 5-6       Extra Bits 1+1 */
#define DCT_VAL_CATEGORY2       6       /* 7-10      Extra Bits 2+1 */
#define DCT_VAL_CATEGORY3       7       /* 11-18     Extra Bits 3+1 */
#define DCT_VAL_CATEGORY4       8       /* 19-34     Extra Bits 4+1 */
#define DCT_VAL_CATEGORY5       9       /* 35-66     Extra Bits 5+1 */
#define DCT_VAL_CATEGORY6       10      /* 67+       Extra Bits 13+1 */
#define DCT_EOB_TOKEN           11      /* EOB       Extra Bits 0+0 */
#define MAX_ENTROPY_TOKENS      12
#define FRAME_CONTEXTS_LOG2     2
#define FRAME_CONTEXTS          (1 << FRAME_CONTEXTS_LOG2)

#define DCT_EOB_MODEL_TOKEN 3 /* EOB Extra Bits 0+0 */

typedef signed char vp9_tree_index;

static const int32_t seg_feature_data_signed[SEG_LVL_MAX] = {1, 1, 0, 0};
static const int32_t seg_feature_data_max[SEG_LVL_MAX] = {MAXQ, 63, 3, 0};

#define NVDEC_VP9HWPAD(x, y) unsigned char x[y]

typedef struct {
    /* last bytes of address 41 */
    unsigned char joints[3];
    unsigned char sign[2];
    /* address 42 */
    unsigned char class0[2][1];
    unsigned char fp[2][3];
    unsigned char class0_hp[2];
    unsigned char hp[2];
    unsigned char classes[2][10];
    /* address 43 */
    unsigned char class0_fp[2][2][3];
    unsigned char bits[2][10];

} nvdec_nmv_context;

/* Adaptive entropy contexts, padding elements are added to have
 * 256 bit aligned tables for HW access.
 * Compile with TRACE_PROB_TABLES to print bases for each table. */
typedef struct nvdec_vp9AdaptiveEntropyProbs_s
{
    /* address 32 */
    unsigned char inter_mode_prob[7][4];
    unsigned char intra_inter_prob[4];

    /* address 33 */
    unsigned char uv_mode_prob[10][8];
    unsigned char tx8x8_prob[2][1];
    unsigned char tx16x16_prob[2][2];
    unsigned char tx32x32_prob[2][3];
    unsigned char sb_ymode_probB[4][1];
    unsigned char sb_ymode_prob[4][8];

    /* address 37 */
    unsigned char partition_prob[2][16][4];

    /* address 41 */
    unsigned char uv_mode_probB[10][1];
    unsigned char switchable_interp_prob[4][2];
    unsigned char comp_inter_prob[5];
    unsigned char mbskip_probs[3];
    NVDEC_VP9HWPAD(pad1, 1);

    nvdec_nmv_context nmvc;

    /* address 44 */
    unsigned char single_ref_prob[5][2];
    unsigned char comp_ref_prob[5];
    NVDEC_VP9HWPAD(pad2, 17);

    /* address 45 */
    unsigned char probCoeffs[2][2][6][6][4];
    unsigned char probCoeffs8x8[2][2][6][6][4];
    unsigned char probCoeffs16x16[2][2][6][6][4];
    unsigned char probCoeffs32x32[2][2][6][6][4];

} nvdec_vp9AdaptiveEntropyProbs_t;

typedef struct nvdec_vp9EntropyProbs_s
{
    /* Default keyframe probs */
    /* Table formatted for 256b memory, probs 0to7 for all tables followed by
     * probs 8toN for all tables.
     * Compile with TRACE_PROB_TABLES to print bases for each table. */

    unsigned char kf_bmode_prob[10][10][8];

    /* Address 25 */
    unsigned char kf_bmode_probB[10][10][1];
    unsigned char ref_pred_probs[3];
    unsigned char mb_segment_tree_probs[7];
    unsigned char segment_pred_probs[3];
    unsigned char ref_scores[4];
    unsigned char prob_comppred[2];
    NVDEC_VP9HWPAD(pad1, 9);

    /* Address 29 */
    unsigned char kf_uv_mode_prob[10][8];
    unsigned char kf_uv_mode_probB[10][1];
    NVDEC_VP9HWPAD(pad2, 6);

    nvdec_vp9AdaptiveEntropyProbs_t a;    /* Probs with backward adaptation */


} nvdec_vp9EntropyProbs_t;

typedef struct {
    unsigned int joints[4];
    unsigned int sign[2][2];
    unsigned int classes[2][11];
    unsigned int class0[2][2];
    unsigned int bits[2][10][2];
    unsigned int class0_fp[2][2][4];
    unsigned int fp[2][4];
    unsigned int class0_hp[2][2];
    unsigned int hp[2][2];

} nvdec_nmv_context_counts;

typedef struct nvdec_vp9EntropyCounts_s
{
    unsigned int inter_mode_counts[7][3][2];
    unsigned int sb_ymode_counts[4][10];
    unsigned int uv_mode_counts[10][10];
    unsigned int partition_counts[16][4];
    unsigned int switchable_interp_counts[4][3];
    unsigned int intra_inter_count[4][2];
    unsigned int comp_inter_count[5][2];
    unsigned int single_ref_count[5][2][2];
    unsigned int comp_ref_count[5][2];
    unsigned int tx32x32_count[2][4];
    unsigned int tx16x16_count[2][3];
    unsigned int tx8x8_count[2][2];
    unsigned int mbskip_count[3][2];

    nvdec_nmv_context_counts nmvcount;

    unsigned int countCoeffs[2][2][6][6][4];
    unsigned int countCoeffs8x8[2][2][6][6][4];
    unsigned int countCoeffs16x16[2][2][6][6][4];
    unsigned int countCoeffs32x32[2][2][6][6][4];

    unsigned int countEobs[4][2][2][6][6];

} nvdec_vp9EntropyCounts_t;

// Structure required to update Forward and Backward probabilities
typedef struct _vp9_prob_update_s
{
    nvdec_vp9EntropyProbs_t  *pProbTab;
    nvdec_vp9EntropyCounts_t *pCtxCounters;
    unsigned char   keyFrame : 1;
    unsigned char   prevIsKeyFrame : 1;
    unsigned char   resolutionChange : 1;
    unsigned char   errorResilient : 1;
    unsigned char   prevShowFrame : 1;
    unsigned char   intraOnly : 1;
    unsigned char   reserved2 : 2;
    char            lossless;
    char            transform_mode;
    char            allow_high_precision_mv;
    char            mcomp_filter_type;
    char            comp_pred_mode;
    unsigned char   FrameParallelDecoding;
    unsigned char   RefreshEntropyProbs;
    uint32_t            resetFrameContext;
    uint32_t            frameContextIdx;
    uint32_t            offsetToDctParts;
    uint32_t            allow_comp_inter_inter;
    uint32_t            probsDecoded;
} vp9_prob_update_s;

typedef uint32_t VP9_BD_VALUE;

typedef struct {
    uint32_t buffer_end;
    uint32_t buffer;
    int32_t value;
    int32_t count;
    uint32_t range;
    uint32_t pos;
} vp9_reader;

const vp9_tree_index vp9_coef_tree[ 22] =     /* corresponding _CONTEXT_NODEs */
{
  -DCT_EOB_TOKEN, 2,                             /* 0 = EOB */
  -ZERO_TOKEN, 4,                               /* 1 = ZERO */
  -ONE_TOKEN, 6,                               /* 2 = ONE */
  8, 12,                                      /* 3 = LOW_VAL */
  -TWO_TOKEN, 10,                            /* 4 = TWO */
  -THREE_TOKEN, -FOUR_TOKEN,                /* 5 = THREE */
  14, 16,                                    /* 6 = HIGH_LOW */
  -DCT_VAL_CATEGORY1, -DCT_VAL_CATEGORY2,   /* 7 = CAT_ONE */
  18, 20,                                   /* 8 = CAT_THREEFOUR */
  -DCT_VAL_CATEGORY3, -DCT_VAL_CATEGORY4,  /* 9 = CAT_THREE */
  -DCT_VAL_CATEGORY5, -DCT_VAL_CATEGORY6   /* 10 = CAT_FIVE */
};

const vp9_tree_index vp9_coefmodel_tree[6] = {
  -DCT_EOB_MODEL_TOKEN, 2,                      /* 0 = EOB */
  -ZERO_TOKEN, 4,                               /* 1 = ZERO */
  -ONE_TOKEN, -TWO_TOKEN,                       /* 2 = ONE */
};

const vp9_tree_index vp9_switchable_interp_tree[VP9_SWITCHABLE_FILTERS*2-2] = {
  -0, 2,
  -1, -2
};

const vp9_tree_index vp9_mv_joint_tree[2 * MV_JOINTS - 2] = {
  -MV_JOINT_ZERO, 2,
  -MV_JOINT_HNZVZ, 4,
  -MV_JOINT_HZVNZ, -MV_JOINT_HNZVNZ
};

const vp9_tree_index vp9_mv_class0_tree [2 * CLASS0_SIZE - 2] = {
  -0, -1,
};

const vp9_tree_index vp9_mv_class_tree[2 * MV_CLASSES - 2] = {
  -MV_CLASS_0, 2,
  -MV_CLASS_1, 4,
  6, 8,
  -MV_CLASS_2, -MV_CLASS_3,
  10, 12,
  -MV_CLASS_4, -MV_CLASS_5,
  -MV_CLASS_6, 14,
  16, 18,
  -MV_CLASS_7, -MV_CLASS_8,
  -MV_CLASS_9, -MV_CLASS_10,
};

const vp9_tree_index vp9_mv_fp_tree [2 * 4 - 2] = {
  -0, 2,
  -1, 4,
  -2, -3
};

static const uint32_t vp9dx_bitreader_norm[256] =
{
    0, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

//*****************************************************************
//vp9_entropymode.c
typedef uint8_t vp9_prob;
//typedef uint8_t vp9_tree_index; // typedef i8 vp9_tree_index
static const vp9_prob default_kf_uv_probs[VP9_INTRA_MODES]
                                         [VP9_INTRA_MODES - 1] = {
  { 144,  11,  54, 157, 195, 130,  46,  58, 108 } /* y = dc */,
  { 118,  15, 123, 148, 131, 101,  44,  93, 131 } /* y = v */,
  { 113,  12,  23, 188, 226, 142,  26,  32, 125 } /* y = h */,
  { 120,  11,  50, 123, 163, 135,  64,  77, 103 } /* y = d45 */,
  { 113,   9,  36, 155, 111, 157,  32,  44, 161 } /* y = d135 */,
  { 116,   9,  55, 176,  76,  96,  37,  61, 149 } /* y = d117 */,
  { 115,   9,  28, 141, 161, 167,  21,  25, 193 } /* y = d153 */,
  { 120,  12,  32, 145, 195, 142,  32,  38,  86 } /* y = d27 */,
  { 116,  12,  64, 120, 140, 125,  49, 115, 121 } /* y = d63 */,
  { 102,  19,  66, 162, 182, 122,  35,  59, 128 } /* y = tm */
};

static const vp9_prob default_if_y_probs[BLOCK_SIZE_GROUPS]
                                        [VP9_INTRA_MODES - 1] = {
  {  65,  32,  18, 144, 162, 194,  41,  51,  98 } /* block_size < 8x8 */,
  { 132,  68,  18, 165, 217, 196,  45,  40,  78 } /* block_size < 16x16 */,
  { 173,  80,  19, 176, 240, 193,  64,  35,  46 } /* block_size < 32x32 */,
  { 221, 135,  38, 194, 248, 121,  96,  85,  29 } /* block_size >= 32x32 */
};

static const vp9_prob default_if_uv_probs[VP9_INTRA_MODES]
                                         [VP9_INTRA_MODES - 1] = {
  { 120,   7,  76, 176, 208, 126,  28,  54, 103 } /* y = dc */,
  {  48,  12, 154, 155, 139,  90,  34, 117, 119 } /* y = v */,
  {  67,   6,  25, 204, 243, 158,  13,  21,  96 } /* y = h */,
  {  97,   5,  44, 131, 176, 139,  48,  68,  97 } /* y = d45 */,
  {  83,   5,  42, 156, 111, 152,  26,  49, 152 } /* y = d135 */,
  {  80,   5,  58, 178,  74,  83,  33,  62, 145 } /* y = d117 */,
  {  86,   5,  32, 154, 192, 168,  14,  22, 163 } /* y = d153 */,
  {  85,   5,  32, 156, 216, 148,  19,  29,  73 } /* y = d27 */,
  {  77,   7,  64, 116, 132, 122,  37, 126, 120 } /* y = d63 */,
  { 101,  21, 107, 181, 192, 103,  19,  67, 125 } /* y = tm */
};

static const uint8_t vp9_default_inter_mode_prob[INTER_MODE_CONTEXTS][4] = {
  {2,       173,   34,   0},  // 0 = both zero mv
  {7,       145,   85,   0},  // 1 = one zero mv + one a predicted mv
  {7,       166,   63,   0},  // 2 = two predicted mvs
  {7,       94,    66,   0},  // 3 = one predicted/zero and one new mv
  {8,       64,    46,   0},  // 4 = two new mvs
  {17,      81,    31,   0},  // 5 = one intra neighbour + x
  {25,      29,    30,   0},  // 6 = two intra neighbours
};
static const vp9_prob vp9_partition_probs[NUM_FRAME_TYPES][NUM_PARTITION_CONTEXTS]
                                  [PARTITION_TYPES] = { /* 1 byte padding */
  { /* frame_type = keyframe */
    /* 8x8 -> 4x4 */
    { 158,  97,  94, 0 } /* a/l both not split */,
    {  93,  24,  99, 0 } /* a split, l not split */,
    {  85, 119,  44, 0 } /* l split, a not split */,
    {  62,  59,  67, 0 } /* a/l both split */,
    /* 16x16 -> 8x8 */
    { 149,  53,  53, 0 } /* a/l both not split */,
    {  94,  20,  48, 0 } /* a split, l not split */,
    {  83,  53,  24, 0 } /* l split, a not split */,
    {  52,  18,  18, 0 } /* a/l both split */,
    /* 32x32 -> 16x16 */
    { 150,  40,  39, 0 } /* a/l both not split */,
    {  78,  12,  26, 0 } /* a split, l not split */,
    {  67,  33,  11, 0 } /* l split, a not split */,
    {  24,   7,   5, 0 } /* a/l both split */,
    /* 64x64 -> 32x32 */
    { 174,  35,  49, 0 } /* a/l both not split */,
    {  68,  11,  27, 0 } /* a split, l not split */,
    {  57,  15,   9, 0 } /* l split, a not split */,
    {  12,   3,   3, 0 } /* a/l both split */
  }, { /* frame_type = interframe */
    /* 8x8 -> 4x4 */
    { 199, 122, 141, 0 } /* a/l both not split */,
    { 147,  63, 159, 0 } /* a split, l not split */,
    { 148, 133, 118, 0 } /* l split, a not split */,
    { 121, 104, 114, 0 } /* a/l both split */,
    /* 16x16 -> 8x8 */
    { 174,  73,  87, 0 } /* a/l both not split */,
    {  92,  41,  83, 0 } /* a split, l not split */,
    {  82,  99,  50, 0 } /* l split, a not split */,
    {  53,  39,  39, 0 } /* a/l both split */,
    /* 32x32 -> 16x16 */
    { 177,  58,  59, 0 } /* a/l both not split */,
    {  68,  26,  63, 0 } /* a split, l not split */,
    {  52,  79,  25, 0 } /* l split, a not split */,
    {  17,  14,  12, 0 } /* a/l both split */,
    /* 64x64 -> 32x32 */
    { 222,  34,  30, 0 } /* a/l both not split */,
    {  72,  16,  44, 0 } /* a split, l not split */,
    {  58,  32,  12, 0 } /* l split, a not split */,
    {  10,   7,   6, 0 } /* a/l both split */
  }
};
static const vp9_tree_index vp9_intra_mode_tree[VP9_INTRA_MODES * 2 - 2] = {
  -DC_PRED, 2,                      // 0 = DC_NODE
  -TM_PRED, 4,                      // 1 = TM_NODE
  -V_PRED, 6,                       // 2 = V_NODE
  8, 12,                            // 3 = COM_NODE
  -H_PRED, 10,                      // 4 = H_NODE
  -D135_PRED, -D117_PRED,           // 5 = D135_NODE
  -D45_PRED, 14,                    // 6 = D45_NODE
  -D63_PRED, 16,                    // 7 = D63_NODE
  -D153_PRED, -D27_PRED             // 8 = D153_NODE
};

static const vp9_tree_index vp9_partition_tree[6] = {
  -PARTITION_NONE, 2,
  -PARTITION_HORZ, 4,
  -PARTITION_VERT, -PARTITION_SPLIT
};

static const vp9_prob default_intra_inter_p[INTRA_INTER_CONTEXTS] = {
  9, 102, 187, 225
};

static const vp9_prob default_comp_inter_p[COMP_INTER_CONTEXTS] = {
  239, 183, 119,  96,  41
};

static const vp9_prob default_comp_ref_p[REF_CONTEXTS] = {
  50, 126, 123, 221, 226
};

static const vp9_prob default_single_ref_p[REF_CONTEXTS][2] = {
  {  33,  16 },
  {  77,  74 },
  { 142, 142 },
  { 172, 170 },
  { 238, 247 }
};

static const vp9_prob vp9_switchable_interp_prob [VP9_SWITCHABLE_FILTERS+1]
                                          [VP9_SWITCHABLE_FILTERS-1] = {
  { 235, 162, },
  { 36, 255, },
  { 34, 3, },
  { 149, 144, },
};
static const vp9_prob vp9_default_tx_probs_32x32p[TX_SIZE_CONTEXTS]
                                          [TX_SIZE_MAX_SB - 1] = {
  { 3, 136, 37, },
  { 5, 52, 13, },
};
static const vp9_prob vp9_default_tx_probs_16x16p[TX_SIZE_CONTEXTS]
                                          [TX_SIZE_MAX_SB - 2] = {
  { 20, 152, },
  { 15, 101, },
};
static const vp9_prob vp9_default_tx_probs_8x8p[TX_SIZE_CONTEXTS]
                                        [TX_SIZE_MAX_SB - 3] = {
  { 100, },
  { 66, },
};
static const vp9_prob vp9_default_mbskip_probs[MBSKIP_CONTEXTS] = {  //its C0..shud be f8??
  192, 128, 64
};

static const nvdec_nmv_context vp9_default_nmv_context = {
  {32, 64, 96}, /* joints */
  {128, 128},   /* sign */
  {{216},{208}},                                            /* class0 */
  {{64, 96, 64},{64, 96, 64}},                              /* fp */
  {160,160},                                                /* class0_hp bit */
  {128,128},                                                /* hp */
  {{224, 144, 192, 168, 192, 176, 192, 198, 198, 245},
   {216, 128, 176, 160, 176, 176, 192, 198, 198, 208}},     /* class */
  {{{128, 128, 64}, {96, 112, 64}},
   {{128, 128, 64}, {96, 112, 64}}},                        /* class0_fp */
  {{136, 140, 148, 160, 176, 192, 224, 234, 234, 240},
   {136, 140, 148, 160, 176, 192, 224, 234, 234, 240}},     /* bits */
};

static const int32_t vp9_seg_feature_data_signed[SEG_LVL_MAX] = { 1, 1, 0, 0 };
static const int32_t vp9_seg_feature_data_max[SEG_LVL_MAX] = { 255, 63, 3, 0 };
typedef uint8_t vp9_coeff_probs[VP9_REF_TYPES][VP9_COEF_BANDS][VP9_PREV_COEF_CONTEXTS][UNCONSTRAINED_NODES];

static const vp9_coeff_probs default_coef_probs_4x4[VP9_BLOCK_TYPES] = {
  { /* block Type 0 */
    { /* Intra */
      { /* Coeff Band 0 */
        { 195,  29, 183 },
        {  84,  49, 136 },
        {   8,  42,  71 }
      }, { /* Coeff Band 1 */
        {  31, 107, 169 },
        {  35,  99, 159 },
        {  17,  82, 140 },
        {   8,  66, 114 },
        {   2,  44,  76 },
        {   1,  19,  32 }
      }, { /* Coeff Band 2 */
        {  40, 132, 201 },
        {  29, 114, 187 },
        {  13,  91, 157 },
        {   7,  75, 127 },
        {   3,  58,  95 },
        {   1,  28,  47 }
      }, { /* Coeff Band 3 */
        {  69, 142, 221 },
        {  42, 122, 201 },
        {  15,  91, 159 },
        {   6,  67, 121 },
        {   1,  42,  77 },
        {   1,  17,  31 }
      }, { /* Coeff Band 4 */
        { 102, 148, 228 },
        {  67, 117, 204 },
        {  17,  82, 154 },
        {   6,  59, 114 },
        {   2,  39,  75 },
        {   1,  15,  29 }
      }, { /* Coeff Band 5 */
        { 156,  57, 233 },
        { 119,  57, 212 },
        {  58,  48, 163 },
        {  29,  40, 124 },
        {  12,  30,  81 },
        {   3,  12,  31 }
      }
    }, { /* Inter */
      { /* Coeff Band 0 */
        { 191, 107, 226 },
        { 124, 117, 204 },
        {  25,  99, 155 }
      }, { /* Coeff Band 1 */
        {  29, 148, 210 },
        {  37, 126, 194 },
        {   8,  93, 157 },
        {   2,  68, 118 },
        {   1,  39,  69 },
        {   1,  17,  33 }
      }, { /* Coeff Band 2 */
        {  41, 151, 213 },
        {  27, 123, 193 },
        {   3,  82, 144 },
        {   1,  58, 105 },
        {   1,  32,  60 },
        {   1,  13,  26 }
      }, { /* Coeff Band 3 */
        {  59, 159, 220 },
        {  23, 126, 198 },
        {   4,  88, 151 },
        {   1,  66, 114 },
        {   1,  38,  71 },
        {   1,  18,  34 }
      }, { /* Coeff Band 4 */
        { 114, 136, 232 },
        {  51, 114, 207 },
        {  11,  83, 155 },
        {   3,  56, 105 },
        {   1,  33,  65 },
        {   1,  17,  34 }
      }, { /* Coeff Band 5 */
        { 149,  65, 234 },
        { 121,  57, 215 },
        {  61,  49, 166 },
        {  28,  36, 114 },
        {  12,  25,  76 },
        {   3,  16,  42 }
      }
    }
  }, { /* block Type 1 */
    { /* Intra */
      { /* Coeff Band 0 */
        { 214,  49, 220 },
        { 132,  63, 188 },
        {  42,  65, 137 }
      }, { /* Coeff Band 1 */
        {  85, 137, 221 },
        { 104, 131, 216 },
        {  49, 111, 192 },
        {  21,  87, 155 },
        {   2,  49,  87 },
        {   1,  16,  28 }
      }, { /* Coeff Band 2 */
        {  89, 163, 230 },
        {  90, 137, 220 },
        {  29, 100, 183 },
        {  10,  70, 135 },
        {   2,  42,  81 },
        {   1,  17,  33 }
      }, { /* Coeff Band 3 */
        { 108, 167, 237 },
        {  55, 133, 222 },
        {  15,  97, 179 },
        {   4,  72, 135 },
        {   1,  45,  85 },
        {   1,  19,  38 }
      }, { /* Coeff Band 4 */
        { 124, 146, 240 },
        {  66, 124, 224 },
        {  17,  88, 175 },
        {   4,  58, 122 },
        {   1,  36,  75 },
        {   1,  18,  37 }
      }, { /* Coeff Band 5 */
        { 141,  79, 241 },
        { 126,  70, 227 },
        {  66,  58, 182 },
        {  30,  44, 136 },
        {  12,  34,  96 },
        {   2,  20,  47 }
      }
    }, { /* Inter */
      { /* Coeff Band 0 */
        { 229,  99, 249 },
        { 143, 111, 235 },
        {  46, 109, 192 }
      }, { /* Coeff Band 1 */
        {  82, 158, 236 },
        {  94, 146, 224 },
        {  25, 117, 191 },
        {   9,  87, 149 },
        {   3,  56,  99 },
        {   1,  33,  57 }
      }, { /* Coeff Band 2 */
        {  83, 167, 237 },
        {  68, 145, 222 },
        {  10, 103, 177 },
        {   2,  72, 131 },
        {   1,  41,  79 },
        {   1,  20,  39 }
      }, { /* Coeff Band 3 */
        {  99, 167, 239 },
        {  47, 141, 224 },
        {  10, 104, 178 },
        {   2,  73, 133 },
        {   1,  44,  85 },
        {   1,  22,  47 }
      }, { /* Coeff Band 4 */
        { 127, 145, 243 },
        {  71, 129, 228 },
        {  17,  93, 177 },
        {   3,  61, 124 },
        {   1,  41,  84 },
        {   1,  21,  52 }
      }, { /* Coeff Band 5 */
        { 157,  78, 244 },
        { 140,  72, 231 },
        {  69,  58, 184 },
        {  31,  44, 137 },
        {  14,  38, 105 },
        {   8,  23,  61 }
      }
    }
  }
};
static const vp9_coeff_probs default_coef_probs_8x8[VP9_BLOCK_TYPES] = {
  { /* block Type 0 */
    { /* Intra */
      { /* Coeff Band 0 */
        { 125,  34, 187 },
        {  52,  41, 133 },
        {   6,  31,  56 }
      }, { /* Coeff Band 1 */
        {  37, 109, 153 },
        {  51, 102, 147 },
        {  23,  87, 128 },
        {   8,  67, 101 },
        {   1,  41,  63 },
        {   1,  19,  29 }
      }, { /* Coeff Band 2 */
        {  31, 154, 185 },
        {  17, 127, 175 },
        {   6,  96, 145 },
        {   2,  73, 114 },
        {   1,  51,  82 },
        {   1,  28,  45 }
      }, { /* Coeff Band 3 */
        {  23, 163, 200 },
        {  10, 131, 185 },
        {   2,  93, 148 },
        {   1,  67, 111 },
        {   1,  41,  69 },
        {   1,  14,  24 }
      }, { /* Coeff Band 4 */
        {  29, 176, 217 },
        {  12, 145, 201 },
        {   3, 101, 156 },
        {   1,  69, 111 },
        {   1,  39,  63 },
        {   1,  14,  23 }
      }, { /* Coeff Band 5 */
        {  57, 192, 233 },
        {  25, 154, 215 },
        {   6, 109, 167 },
        {   3,  78, 118 },
        {   1,  48,  69 },
        {   1,  21,  29 }
      }
    }, { /* Inter */
      { /* Coeff Band 0 */
        { 202, 105, 245 },
        { 108, 106, 216 },
        {  18,  90, 144 }
      }, { /* Coeff Band 1 */
        {  33, 172, 219 },
        {  64, 149, 206 },
        {  14, 117, 177 },
        {   5,  90, 141 },
        {   2,  61,  95 },
        {   1,  37,  57 }
      }, { /* Coeff Band 2 */
        {  33, 179, 220 },
        {  11, 140, 198 },
        {   1,  89, 148 },
        {   1,  60, 104 },
        {   1,  33,  57 },
        {   1,  12,  21 }
      }, { /* Coeff Band 3 */
        {  30, 181, 221 },
        {   8, 141, 198 },
        {   1,  87, 145 },
        {   1,  58, 100 },
        {   1,  31,  55 },
        {   1,  12,  20 }
      }, { /* Coeff Band 4 */
        {  32, 186, 224 },
        {   7, 142, 198 },
        {   1,  86, 143 },
        {   1,  58, 100 },
        {   1,  31,  55 },
        {   1,  12,  22 }
      }, { /* Coeff Band 5 */
        {  57, 192, 227 },
        {  20, 143, 204 },
        {   3,  96, 154 },
        {   1,  68, 112 },
        {   1,  42,  69 },
        {   1,  19,  32 }
      }
    }
  }, { /* block Type 1 */
    { /* Intra */
      { /* Coeff Band 0 */
        { 212,  35, 215 },
        { 113,  47, 169 },
        {  29,  48, 105 }
      }, { /* Coeff Band 1 */
        {  74, 129, 203 },
        { 106, 120, 203 },
        {  49, 107, 178 },
        {  19,  84, 144 },
        {   4,  50,  84 },
        {   1,  15,  25 }
      }, { /* Coeff Band 2 */
        {  71, 172, 217 },
        {  44, 141, 209 },
        {  15, 102, 173 },
        {   6,  76, 133 },
        {   2,  51,  89 },
        {   1,  24,  42 }
      }, { /* Coeff Band 3 */
        {  64, 185, 231 },
        {  31, 148, 216 },
        {   8, 103, 175 },
        {   3,  74, 131 },
        {   1,  46,  81 },
        {   1,  18,  30 }
      }, { /* Coeff Band 4 */
        {  65, 196, 235 },
        {  25, 157, 221 },
        {   5, 105, 174 },
        {   1,  67, 120 },
        {   1,  38,  69 },
        {   1,  15,  30 }
      }, { /* Coeff Band 5 */
        {  65, 204, 238 },
        {  30, 156, 224 },
        {   7, 107, 177 },
        {   2,  70, 124 },
        {   1,  42,  73 },
        {   1,  18,  34 }
      }
    }, { /* Inter */
      { /* Coeff Band 0 */
        { 225,  86, 251 },
        { 144, 104, 235 },
        {  42,  99, 181 }
      }, { /* Coeff Band 1 */
        {  85, 175, 239 },
        { 112, 165, 229 },
        {  29, 136, 200 },
        {  12, 103, 162 },
        {   6,  77, 123 },
        {   2,  53,  84 }
      }, { /* Coeff Band 2 */
        {  75, 183, 239 },
        {  30, 155, 221 },
        {   3, 106, 171 },
        {   1,  74, 128 },
        {   1,  44,  76 },
        {   1,  17,  28 }
      }, { /* Coeff Band 3 */
        {  73, 185, 240 },
        {  27, 159, 222 },
        {   2, 107, 172 },
        {   1,  75, 127 },
        {   1,  42,  73 },
        {   1,  17,  29 }
      }, { /* Coeff Band 4 */
        {  62, 190, 238 },
        {  21, 159, 222 },
        {   2, 107, 172 },
        {   1,  72, 122 },
        {   1,  40,  71 },
        {   1,  18,  32 }
      }, { /* Coeff Band 5 */
        {  61, 199, 240 },
        {  27, 161, 226 },
        {   4, 113, 180 },
        {   1,  76, 129 },
        {   1,  46,  80 },
        {   1,  23,  41 }
      }
    }
  }
};
static const vp9_coeff_probs default_coef_probs_16x16[VP9_BLOCK_TYPES] = {
  { /* block Type 0 */
    { /* Intra */
      { /* Coeff Band 0 */
        {   7,  27, 153 },
        {   5,  30,  95 },
        {   1,  16,  30 }
      }, { /* Coeff Band 1 */
        {  50,  75, 127 },
        {  57,  75, 124 },
        {  27,  67, 108 },
        {  10,  54,  86 },
        {   1,  33,  52 },
        {   1,  12,  18 }
      }, { /* Coeff Band 2 */
        {  43, 125, 151 },
        {  26, 108, 148 },
        {   7,  83, 122 },
        {   2,  59,  89 },
        {   1,  38,  60 },
        {   1,  17,  27 }
      }, { /* Coeff Band 3 */
        {  23, 144, 163 },
        {  13, 112, 154 },
        {   2,  75, 117 },
        {   1,  50,  81 },
        {   1,  31,  51 },
        {   1,  14,  23 }
      }, { /* Coeff Band 4 */
        {  18, 162, 185 },
        {   6, 123, 171 },
        {   1,  78, 125 },
        {   1,  51,  86 },
        {   1,  31,  54 },
        {   1,  14,  23 }
      }, { /* Coeff Band 5 */
        {  15, 199, 227 },
        {   3, 150, 204 },
        {   1,  91, 146 },
        {   1,  55,  95 },
        {   1,  30,  53 },
        {   1,  11,  20 }
      }
    }, { /* Inter */
      { /* Coeff Band 0 */
        {  19,  55, 240 },
        {  19,  59, 196 },
        {   3,  52, 105 }
      }, { /* Coeff Band 1 */
        {  41, 166, 207 },
        { 104, 153, 199 },
        {  31, 123, 181 },
        {  14, 101, 152 },
        {   5,  72, 106 },
        {   1,  36,  52 }
      }, { /* Coeff Band 2 */
        {  35, 176, 211 },
        {  12, 131, 190 },
        {   2,  88, 144 },
        {   1,  60, 101 },
        {   1,  36,  60 },
        {   1,  16,  28 }
      }, { /* Coeff Band 3 */
        {  28, 183, 213 },
        {   8, 134, 191 },
        {   1,  86, 142 },
        {   1,  56,  96 },
        {   1,  30,  53 },
        {   1,  12,  20 }
      }, { /* Coeff Band 4 */
        {  20, 190, 215 },
        {   4, 135, 192 },
        {   1,  84, 139 },
        {   1,  53,  91 },
        {   1,  28,  49 },
        {   1,  11,  20 }
      }, { /* Coeff Band 5 */
        {  13, 196, 216 },
        {   2, 137, 192 },
        {   1,  86, 143 },
        {   1,  57,  99 },
        {   1,  32,  56 },
        {   1,  13,  24 }
      }
    }
  }, { /* block Type 1 */
    { /* Intra */
      { /* Coeff Band 0 */
        { 211,  29, 217 },
        {  96,  47, 156 },
        {  22,  43,  87 }
      }, { /* Coeff Band 1 */
        {  78, 120, 193 },
        { 111, 116, 186 },
        {  46, 102, 164 },
        {  15,  80, 128 },
        {   2,  49,  76 },
        {   1,  18,  28 }
      }, { /* Coeff Band 2 */
        {  71, 161, 203 },
        {  42, 132, 192 },
        {  10,  98, 150 },
        {   3,  69, 109 },
        {   1,  44,  70 },
        {   1,  18,  29 }
      }, { /* Coeff Band 3 */
        {  57, 186, 211 },
        {  30, 140, 196 },
        {   4,  93, 146 },
        {   1,  62, 102 },
        {   1,  38,  65 },
        {   1,  16,  27 }
      }, { /* Coeff Band 4 */
        {  47, 199, 217 },
        {  14, 145, 196 },
        {   1,  88, 142 },
        {   1,  57,  98 },
        {   1,  36,  62 },
        {   1,  15,  26 }
      }, { /* Coeff Band 5 */
        {  26, 219, 229 },
        {   5, 155, 207 },
        {   1,  94, 151 },
        {   1,  60, 104 },
        {   1,  36,  62 },
        {   1,  16,  28 }
      }
    }, { /* Inter */
      { /* Coeff Band 0 */
        { 233,  29, 248 },
        { 146,  47, 220 },
        {  43,  52, 140 }
      }, { /* Coeff Band 1 */
        { 100, 163, 232 },
        { 179, 161, 222 },
        {  63, 142, 204 },
        {  37, 113, 174 },
        {  26,  89, 137 },
        {  18,  68,  97 }
      }, { /* Coeff Band 2 */
        {  85, 181, 230 },
        {  32, 146, 209 },
        {   7, 100, 164 },
        {   3,  71, 121 },
        {   1,  45,  77 },
        {   1,  18,  30 }
      }, { /* Coeff Band 3 */
        {  65, 187, 230 },
        {  20, 148, 207 },
        {   2,  97, 159 },
        {   1,  68, 116 },
        {   1,  40,  70 },
        {   1,  14,  29 }
      }, { /* Coeff Band 4 */
        {  40, 194, 227 },
        {   8, 147, 204 },
        {   1,  94, 155 },
        {   1,  65, 112 },
        {   1,  39,  66 },
        {   1,  14,  26 }
      }, { /* Coeff Band 5 */
        {  16, 208, 228 },
        {   3, 151, 207 },
        {   1,  98, 160 },
        {   1,  67, 117 },
        {   1,  41,  74 },
        {   1,  17,  31 }
      }
    }
  }
};
static const vp9_coeff_probs default_coef_probs_32x32[VP9_BLOCK_TYPES] = {
  { /* block Type 0 */
    { /* Intra */
      { /* Coeff Band 0 */
        {  17,  38, 140 },
        {   7,  34,  80 },
        {   1,  17,  29 }
      }, { /* Coeff Band 1 */
        {  37,  75, 128 },
        {  41,  76, 128 },
        {  26,  66, 116 },
        {  12,  52,  94 },
        {   2,  32,  55 },
        {   1,  10,  16 }
      }, { /* Coeff Band 2 */
        {  50, 127, 154 },
        {  37, 109, 152 },
        {  16,  82, 121 },
        {   5,  59,  85 },
        {   1,  35,  54 },
        {   1,  13,  20 }
      }, { /* Coeff Band 3 */
        {  40, 142, 167 },
        {  17, 110, 157 },
        {   2,  71, 112 },
        {   1,  44,  72 },
        {   1,  27,  45 },
        {   1,  11,  17 }
      }, { /* Coeff Band 4 */
        {  30, 175, 188 },
        {   9, 124, 169 },
        {   1,  74, 116 },
        {   1,  48,  78 },
        {   1,  30,  49 },
        {   1,  11,  18 }
      }, { /* Coeff Band 5 */
        {  10, 222, 223 },
        {   2, 150, 194 },
        {   1,  83, 128 },
        {   1,  48,  79 },
        {   1,  27,  45 },
        {   1,  11,  17 }
      }
    }, { /* Inter */
      { /* Coeff Band 0 */
        {  36,  41, 235 },
        {  29,  36, 193 },
        {  10,  27, 111 }
      }, { /* Coeff Band 1 */
        {  85, 165, 222 },
        { 177, 162, 215 },
        { 110, 135, 195 },
        {  57, 113, 168 },
        {  23,  83, 120 },
        {  10,  49,  61 }
      }, { /* Coeff Band 2 */
        {  85, 190, 223 },
        {  36, 139, 200 },
        {   5,  90, 146 },
        {   1,  60, 103 },
        {   1,  38,  65 },
        {   1,  18,  30 }
      }, { /* Coeff Band 3 */
        {  72, 202, 223 },
        {  23, 141, 199 },
        {   2,  86, 140 },
        {   1,  56,  97 },
        {   1,  36,  61 },
        {   1,  16,  27 }
      }, { /* Coeff Band 4 */
        {  55, 218, 225 },
        {  13, 145, 200 },
        {   1,  86, 141 },
        {   1,  57,  99 },
        {   1,  35,  61 },
        {   1,  13,  22 }
      }, { /* Coeff Band 5 */
        {  15, 235, 212 },
        {   1, 132, 184 },
        {   1,  84, 139 },
        {   1,  57,  97 },
        {   1,  34,  56 },
        {   1,  14,  23 }
      }
    }
  }, { /* block Type 1 */
    { /* Intra */
      { /* Coeff Band 0 */
        { 181,  21, 201 },
        {  61,  37, 123 },
        {  10,  38,  71 }
      }, { /* Coeff Band 1 */
        {  47, 106, 172 },
        {  95, 104, 173 },
        {  42,  93, 159 },
        {  18,  77, 131 },
        {   4,  50,  81 },
        {   1,  17,  23 }
      }, { /* Coeff Band 2 */
        {  62, 147, 199 },
        {  44, 130, 189 },
        {  28, 102, 154 },
        {  18,  75, 115 },
        {   2,  44,  65 },
        {   1,  12,  19 }
      }, { /* Coeff Band 3 */
        {  55, 153, 210 },
        {  24, 130, 194 },
        {   3,  93, 146 },
        {   1,  61,  97 },
        {   1,  31,  50 },
        {   1,  10,  16 }
      }, { /* Coeff Band 4 */
        {  49, 186, 223 },
        {  17, 148, 204 },
        {   1,  96, 142 },
        {   1,  53,  83 },
        {   1,  26,  44 },
        {   1,  11,  17 }
      }, { /* Coeff Band 5 */
        {  13, 217, 212 },
        {   2, 136, 180 },
        {   1,  78, 124 },
        {   1,  50,  83 },
        {   1,  29,  49 },
        {   1,  14,  23 }
      }
    }, { /* Inter */
      { /* Coeff Band 0 */
        { 197,  13, 247 },
        {  82,  17, 222 },
        {  25,  17, 162 }
      }, { /* Coeff Band 1 */
        { 126, 186, 247 },
        { 234, 191, 243 },
        { 176, 177, 234 },
        { 104, 158, 220 },
        {  66, 128, 186 },
        {  55,  90, 137 }
      }, { /* Coeff Band 2 */
        { 111, 197, 242 },
        {  46, 158, 219 },
        {   9, 104, 171 },
        {   2,  65, 125 },
        {   1,  44,  80 },
        {   1,  17,  91 }
      }, { /* Coeff Band 3 */
        { 104, 208, 245 },
        {  39, 168, 224 },
        {   3, 109, 162 },
        {   1,  79, 124 },
        {   1,  50, 102 },
        {   1,  43, 102 }
      }, { /* Coeff Band 4 */
        {  84, 220, 246 },
        {  31, 177, 231 },
        {   2, 115, 180 },
        {   1,  79, 134 },
        {   1,  55,  77 },
        {   1,  60,  79 }
      }, { /* Coeff Band 5 */
        {  43, 243, 240 },
        {   8, 180, 217 },
        {   1, 115, 166 },
        {   1,  84, 121 },
        {   1,  51,  67 },
        {   1,  16,   6 }
      }
    }
  }
};

static const uint8_t vp9_kf_default_bmode_probs[VP9_INTRA_MODES]
                                   [VP9_INTRA_MODES]
                                   [VP9_INTRA_MODES-1] = {
  { /* above = dc */
    { 137,  30,  42, 148, 151, 207,  70,  52,  91 } /* left = dc */,
    {  92,  45, 102, 136, 116, 180,  74,  90, 100 } /* left = v */,
    {  73,  32,  19, 187, 222, 215,  46,  34, 100 } /* left = h */,
    {  91,  30,  32, 116, 121, 186,  93,  86,  94 } /* left = d45 */,
    {  72,  35,  36, 149,  68, 206,  68,  63, 105 } /* left = d135 */,
    {  73,  31,  28, 138,  57, 124,  55, 122, 151 } /* left = d117 */,
    {  67,  23,  21, 140, 126, 197,  40,  37, 171 } /* left = d153 */,
    {  86,  27,  28, 128, 154, 212,  45,  43,  53 } /* left = d27 */,
    {  74,  32,  27, 107,  86, 160,  63, 134, 102 } /* left = d63 */,
    {  59,  67,  44, 140, 161, 202,  78,  67, 119 } /* left = tm */
  }, { /* above = v */
    {  63,  36, 126, 146, 123, 158,  60,  90,  96 } /* left = dc */,
    {  43,  46, 168, 134, 107, 128,  69, 142,  92 } /* left = v */,
    {  44,  29,  68, 159, 201, 177,  50,  57,  77 } /* left = h */,
    {  58,  38,  76, 114,  97, 172,  78, 133,  92 } /* left = d45 */,
    {  46,  41,  76, 140,  63, 184,  69, 112,  57 } /* left = d135 */,
    {  38,  32,  85, 140,  46, 112,  54, 151, 133 } /* left = d117 */,
    {  39,  27,  61, 131, 110, 175,  44,  75, 136 } /* left = d153 */,
    {  52,  30,  74, 113, 130, 175,  51,  64,  58 } /* left = d27 */,
    {  47,  35,  80, 100,  74, 143,  64, 163,  74 } /* left = d63 */,
    {  36,  61, 116, 114, 128, 162,  80, 125,  82 } /* left = tm */
  }, { /* above = h */
    {  82,  26,  26, 171, 208, 204,  44,  32, 105 } /* left = dc */,
    {  55,  44,  68, 166, 179, 192,  57,  57, 108 } /* left = v */,
    {  42,  26,  11, 199, 241, 228,  23,  15,  85 } /* left = h */,
    {  68,  42,  19, 131, 160, 199,  55,  52,  83 } /* left = d45 */,
    {  58,  50,  25, 139, 115, 232,  39,  52, 118 } /* left = d135 */,
    {  50,  35,  33, 153, 104, 162,  64,  59, 131 } /* left = d117 */,
    {  44,  24,  16, 150, 177, 202,  33,  19, 156 } /* left = d153 */,
    {  55,  27,  12, 153, 203, 218,  26,  27,  49 } /* left = d27 */,
    {  53,  49,  21, 110, 116, 168,  59,  80,  76 } /* left = d63 */,
    {  38,  72,  19, 168, 203, 212,  50,  50, 107 } /* left = tm */
  }, { /* above = d45 */
    { 103,  26,  36, 129, 132, 201,  83,  80,  93 } /* left = dc */,
    {  59,  38,  83, 112, 103, 162,  98, 136,  90 } /* left = v */,
    {  62,  30,  23, 158, 200, 207,  59,  57,  50 } /* left = h */,
    {  67,  30,  29,  84,  86, 191, 102,  91,  59 } /* left = d45 */,
    {  60,  32,  33, 112,  71, 220,  64,  89, 104 } /* left = d135 */,
    {  53,  26,  34, 130,  56, 149,  84, 120, 103 } /* left = d117 */,
    {  53,  21,  23, 133, 109, 210,  56,  77, 172 } /* left = d153 */,
    {  77,  19,  29, 112, 142, 228,  55,  66,  36 } /* left = d27 */,
    {  61,  29,  29,  93,  97, 165,  83, 175, 162 } /* left = d63 */,
    {  47,  47,  43, 114, 137, 181, 100,  99,  95 } /* left = tm */
  }, { /* above = d135 */
    {  69,  23,  29, 128,  83, 199,  46,  44, 101 } /* left = dc */,
    {  53,  40,  55, 139,  69, 183,  61,  80, 110 } /* left = v */,
    {  40,  29,  19, 161, 180, 207,  43,  24,  91 } /* left = h */,
    {  60,  34,  19, 105,  61, 198,  53,  64,  89 } /* left = d45 */,
    {  52,  31,  22, 158,  40, 209,  58,  62,  89 } /* left = d135 */,
    {  44,  31,  29, 147,  46, 158,  56, 102, 198 } /* left = d117 */,
    {  35,  19,  12, 135,  87, 209,  41,  45, 167 } /* left = d153 */,
    {  55,  25,  21, 118,  95, 215,  38,  39,  66 } /* left = d27 */,
    {  51,  38,  25, 113,  58, 164,  70,  93,  97 } /* left = d63 */,
    {  47,  54,  34, 146, 108, 203,  72, 103, 151 } /* left = tm */
  }, { /* above = d117 */
    {  64,  19,  37, 156,  66, 138,  49,  95, 133 } /* left = dc */,
    {  46,  27,  80, 150,  55, 124,  55, 121, 135 } /* left = v */,
    {  36,  23,  27, 165, 149, 166,  54,  64, 118 } /* left = h */,
    {  53,  21,  36, 131,  63, 163,  60, 109,  81 } /* left = d45 */,
    {  40,  26,  35, 154,  40, 185,  51,  97, 123 } /* left = d135 */,
    {  35,  19,  34, 179,  19,  97,  48, 129, 124 } /* left = d117 */,
    {  36,  20,  26, 136,  62, 164,  33,  77, 154 } /* left = d153 */,
    {  45,  18,  32, 130,  90, 157,  40,  79,  91 } /* left = d27 */,
    {  45,  26,  28, 129,  45, 129,  49, 147, 123 } /* left = d63 */,
    {  38,  44,  51, 136,  74, 162,  57,  97, 121 } /* left = tm */
  }, { /* above = d153 */
    {  75,  17,  22, 136, 138, 185,  32,  34, 166 } /* left = dc */,
    {  56,  39,  58, 133, 117, 173,  48,  53, 187 } /* left = v */,
    {  35,  21,  12, 161, 212, 207,  20,  23, 145 } /* left = h */,
    {  56,  29,  19, 117, 109, 181,  55,  68, 112 } /* left = d45 */,
    {  47,  29,  17, 153,  64, 220,  59,  51, 114 } /* left = d135 */,
    {  46,  16,  24, 136,  76, 147,  41,  64, 172 } /* left = d117 */,
    {  34,  17,  11, 108, 152, 187,  13,  15, 209 } /* left = d153 */,
    {  51,  24,  14, 115, 133, 209,  32,  26, 104 } /* left = d27 */,
    {  55,  30,  18, 122,  79, 179,  44,  88, 116 } /* left = d63 */,
    {  37,  49,  25, 129, 168, 164,  41,  54, 148 } /* left = tm */
  }, { /* above = d27 */
    {  82,  22,  32, 127, 143, 213,  39,  41,  70 } /* left = dc */,
    {  62,  44,  61, 123, 105, 189,  48,  57,  64 } /* left = v */,
    {  47,  25,  17, 175, 222, 220,  24,  30,  86 } /* left = h */,
    {  68,  36,  17, 106, 102, 206,  59,  74,  74 } /* left = d45 */,
    {  57,  39,  23, 151,  68, 216,  55,  63,  58 } /* left = d135 */,
    {  49,  30,  35, 141,  70, 168,  82,  40, 115 } /* left = d117 */,
    {  51,  25,  15, 136, 129, 202,  38,  35, 139 } /* left = d153 */,
    {  68,  26,  16, 111, 141, 215,  29,  28,  28 } /* left = d27 */,
    {  59,  39,  19, 114,  75, 180,  77, 104,  42 } /* left = d63 */,
    {  40,  61,  26, 126, 152, 206,  61,  59,  93 } /* left = tm */
  }, { /* above = d63 */
    {  78,  23,  39, 111, 117, 170,  74, 124,  94 } /* left = dc */,
    {  48,  34,  86, 101,  92, 146,  78, 179, 134 } /* left = v */,
    {  47,  22,  24, 138, 187, 178,  68,  69,  59 } /* left = h */,
    {  56,  25,  33, 105, 112, 187,  95, 177, 129 } /* left = d45 */,
    {  48,  31,  27, 114,  63, 183,  82, 116,  56 } /* left = d135 */,
    {  43,  28,  37, 121,  63, 123,  61, 192, 169 } /* left = d117 */,
    {  42,  17,  24, 109,  97, 177,  56,  76, 122 } /* left = d153 */,
    {  58,  18,  28, 105, 139, 182,  70,  92,  63 } /* left = d27 */,
    {  46,  23,  32,  74,  86, 150,  67, 183,  88 } /* left = d63 */,
    {  36,  38,  48,  92, 122, 165,  88, 137,  91 } /* left = tm */
  }, { /* above = tm */
    {  65,  70,  60, 155, 159, 199,  61,  60,  81 } /* left = dc */,
    {  44,  78, 115, 132, 119, 173,  71, 112,  93 } /* left = v */,
    {  39,  38,  21, 184, 227, 206,  42,  32,  64 } /* left = h */,
    {  58,  47,  36, 124, 137, 193,  80,  82,  78 } /* left = d45 */,
    {  49,  50,  35, 144,  95, 205,  63,  78,  59 } /* left = d135 */,
    {  41,  53,  52, 148,  71, 142,  65, 128,  51 } /* left = d117 */,
    {  40,  36,  28, 143, 143, 202,  40,  55, 137 } /* left = d153 */,
    {  52,  34,  29, 129, 183, 227,  42,  35,  43 } /* left = d27 */,
    {  42,  44,  44, 104, 105, 164,  64, 130,  80 } /* left = d63 */,
    {  43,  81,  53, 140, 169, 204,  68,  84,  72 } /* left = tm */
  }
};

class VulkanVP9Decoder : public VulkanVideoDecoder
{
protected:
    vp9_reader                      reader;
    nvdec_vp9EntropyProbs_t         m_EntropyLast[FRAME_CONTEXTS];
    nvdec_vp9AdaptiveEntropyProbs_t m_PrevCtx;
    const unsigned char*            m_pCompressedHeader;

    void vp9_init_mbmode_probs(vp9_prob_update_s *pProbSetup);
    vp9_prob weighted_prob(int32_t prob1, int32_t prob2, int32_t factor);
    vp9_prob clip_prob(uint32_t p);
    vp9_prob get_prob(uint32_t num, uint32_t den);
    vp9_prob get_binary_prob(uint32_t n0, uint32_t n1);
    uint32_t convert_distribution(uint32_t i,
                            const vp9_tree_index * tree,
                            uint8_t probs[],
                            uint32_t branch_ct[][2],
                            const uint32_t num_events[],
                            uint32_t tok0_offset);
    void vp9_tree_probs_from_distribution(const vp9_tree_index* tree,
                                        uint8_t probs          [ /* n-1 */ ],
                                        uint32_t branch_ct       [ /* n-1 */ ] [2],
                                        const uint32_t num_events[ /* n */ ],
                                        uint32_t tok0_offset);
    void update_coef_probs(uint8_t dst_coef_probs[VP9_BLOCK_TYPES][VP9_REF_TYPES][VP9_COEF_BANDS][VP9_PREV_COEF_CONTEXTS][ENTROPY_NODES_PART1],
                        uint8_t pre_coef_probs[VP9_BLOCK_TYPES][VP9_REF_TYPES][VP9_COEF_BANDS][VP9_PREV_COEF_CONTEXTS][ENTROPY_NODES_PART1],
                        uint32_t coef_counts[VP9_BLOCK_TYPES][VP9_REF_TYPES][VP9_COEF_BANDS][VP9_PREV_COEF_CONTEXTS][UNCONSTRAINED_NODES+1],
                        uint32_t (*eob_counts)[VP9_REF_TYPES][VP9_COEF_BANDS][VP9_PREV_COEF_CONTEXTS],
                        int32_t count_sat, int32_t update_factor);
    void adaptCoefProbs(vp9_prob_update_s *pProbSetup);
    int32_t update_mode_ct(vp9_prob pre_prob, vp9_prob prob, uint32_t branch_ct[2]);
    int32_t update_mode_ct2(vp9_prob pre_prob, uint32_t branch_ct[2]);
    void update_mode_probs(int32_t n_modes,
                            const vp9_tree_index *tree, uint32_t *cnt,
                            vp9_prob *pre_probs, vp9_prob *pre_probsB,
                            vp9_prob *dst_probs, vp9_prob *dst_probsB,
                            uint32_t tok0_offset);
    void tx_counts_to_branch_counts_32x32(uint32_t *tx_count_32x32p, uint32_t (*ct_32x32p)[2]);
    void tx_counts_to_branch_counts_16x16(uint32_t *tx_count_16x16p, uint32_t (*ct_16x16p)[2]);
    void tx_counts_to_branch_counts_8x8(uint32_t *tx_count_8x8p, uint32_t (*ct_8x8p)[2]);
    void adaptModeProbs(vp9_prob_update_s *pProbSetup);
    void adaptModeContext(vp9_prob_update_s *pProbSetup);
    uint32_t adapt_probs(uint32_t i,
                         const signed char* tree,
                         vp9_prob this_probs[],
                         const vp9_prob last_probs[],
                         const uint32_t num_events[]);
    void adapt_prob(vp9_prob *dest, vp9_prob prep, uint32_t ct[2]);
    void adaptNmvProbs(vp9_prob_update_s *pProbSetup);

protected:
    void vp9_reader_fill();
    int32_t vp9_reader_init (uint32_t size);
    int32_t vp9_read_bit();
    int32_t vp9_read(int32_t probability);
    int32_t vp9_read_literal(int32_t bits);
    uint32_t ParseCompressedVP9();
    int32_t get_unsigned_bits(uint32_t num_values);
    uint32_t swGetBitsUnsignedMax( uint32_t maxValue);
    vp9_prob vp9hwdReadProbDiffUpdate(uint8_t oldp);
    int32_t vp9_inv_recenter_nonneg(int32_t v, int32_t m);
    int32_t inv_remap_prob(int32_t v, int32_t m);
    int32_t merge_index(int32_t v, int32_t n, int32_t modulus);
    uint32_t BoolDecodeUniform(uint32_t n);
    uint32_t vp9hwdDecodeSubExp(uint32_t k, uint32_t num_syms);
    uint32_t vp9hwdDecodeCoeffUpdate(uint8_t probCoeffs[VP9_BLOCK_TYPES][VP9_REF_TYPES][VP9_COEF_BANDS][VP9_PREV_COEF_CONTEXTS][ENTROPY_NODES_PART1]);
    uint32_t vp9hwdDecodeMvUpdate(vp9_prob_update_s *pProbSetup);
    void update_nmv(vp9_prob *const p, const vp9_prob upd_p);

public:
    VulkanVP9Decoder(VkVideoCodecOperationFlagBitsKHR std);
    void ResetProbs(vp9_prob_update_s *pProbSetup);
    void GetProbs(vp9_prob_update_s *pProbSetup);
    uint32_t UpdateForwardProbability(vp9_prob_update_s *pProbSetup, const unsigned char* pCompressed_Header);
    void UpdateBackwardProbability(vp9_prob_update_s *pProbSetup);

    // TODO: Need to implement these functions.
    bool                    IsPictureBoundary(int32_t) { return true; };
    int32_t                 ParseNalUnit() { return NALU_UNKNOWN; };
    bool                    DecodePicture(VkParserPictureData *) { return false; };
    void                    InitParser() {}
    bool                    BeginPicture(VkParserPictureData *) { return false; }
    void                    CreatePrivateContext() {}
    void                    FreeContext() {}
};

#endif // _VP9_PROBMANAGER_H_
