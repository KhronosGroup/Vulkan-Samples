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

#ifndef YCBCR_UTILS_H_
#define YCBCR_UTILS_H_

#include <string.h>
#include <cassert>
#include <math.h>
#include <sstream>

typedef enum YCBCR_PLANES_LAYOUT {
    YCBCR_SINGLE_PLANE_UNNORMALIZED       = 0, // Single unnormalized plane;
    YCBCR_SINGLE_PLANE_INTERLEAVED        = 1, // Interleaved YUV format (1 plane) ex. YUY2, AYUV, UYVY;
    YCBCR_SEMI_PLANAR_CBCR_INTERLEAVED    = 2, // Y plane and interleaved UV plane (2 planes) ex. NV12;
    YCBCR_PLANAR_CBCR_STRIDE_INTERLEAVED  = 3, // Y plane and separate, side by side U and V planes (3 planes) ex. IMC2/4;
    YCBCR_PLANAR_STRIDE_PADDED            = 4, // Y plane and separate, top U and bottom V planes with Y/U/V planes stride padded (3 planes) ex. IMC1/3;
    YCBCR_PLANAR_CBCR_BLOCK_JOINED        = 5, // Y plane and separate, top U and bottom V planes with y/2 stride each (3 planes) ex. YV12;
} YCBCR_PLANES_LAYOUT;

typedef enum CC_MAP {
    CC_MAP_R  =  0,          // R color channel
    CC_MAP_G  =  1,          // G color channel
    CC_MAP_B  =  2,          // B color channel
    CC_MAP_A  =  3,          // A color channel
    CC_MAP_YL =  0,          // Y Luma channel
    CC_MAP_CB =  1,          // Cb Chroma channel
    CC_MAP_CR =  2,          // Cr Chroma channel
} CC_MAP;

typedef enum YCBCR_COLOR_CHANNEL {
    CC_UN   =  0x0,                         // unused color channel
    CC_Y    =  (1 << 0),                    // Luma Y color channel
    CC_Y0   =  (1 << 0),                    // Luma Y0 color channel for interleaved formats.
    CC_Y1   =  (CC_Y0 | (1 << 3)),          // Luma Y1 color channel for interleaved formats.
    CC_CB   =  (1 << 1),                    // Chroma Cb (U) color channel
    CC_CR   =  (1 << 2),                    // Chroma Cr (V) color channel
    CC_A    =  (1 << 3),                    // alpha color channel
    CC_CBCR =  (CC_CB | CC_CR),             // Chroma CbCr (U) color channel for dual color planes.
    CC_CRCB =  (CC_CR | CC_CB | (1 << 3)),  // Chroma CrCb (U) color channel for dual color planes.
} Ycbcr_COLOR_CHANNEL;

typedef enum Ycbcr_BPP {
    YCBCRA_8BPP =  0x0,
    YCBCRA_10BPP = 0x1,
    YCBCRA_12BPP = 0x2,
    YCBCRA_14BPP = 0x3,
    YCBCRA_16BPP = 0x4,
} Ycbcr_BPP;

typedef enum YCBCR_COLOR_RANGE {
    YCBCR_COLOR_RANGE_ITU_FULL   = 0,
    YCBCR_COLOR_RANGE_ITU_NARROW = 1,
    YCBCR_COLOR_RANGE_NATURAL    = ~0L,
} YCBCR_COLOR_RANGE;

typedef enum YcbcrLevelsRange {
    YcbcrLevelsDigital      = 0,
    YcbcrLevelsAnalog       = 1,
    YcbcrLevelsNvidiaCompat = 2,
} YcbcrLevelsRange;

typedef enum YcbcrBtStandard {
    YcbcrBtStandardUnknown    = -1,
    YcbcrBtStandardBt709      = 0,
    YcbcrBtStandardBt601Ebu   = 1,
    YcbcrBtStandardBt601Smtpe = 2,
    YcbcrBtStandardBt2020     = 3,
} YcbcrBtStandard;

typedef struct YcbcrPlanesLayoutInfo {
    union {
        struct {
            unsigned int layout:4;                    // Planes memory layout: One of the YCBCR_PLANES_LAYOUT;
            unsigned int disjoint:1;                  // disjoint planes (separately bound to memory) 0: no 1: yes;
            unsigned int bpp:3;                       // bits per channel YCBCRA_BPP: 8-bit:YCBCRA_8BPP; 10-bit:YCBCRA_10BPP; 12-bit:YCBCRA_12BPP; 16-bit: YCBCRA_16BPP;
            unsigned int secondaryPlaneSubsampledX:1; // Sub-sample UV factor on X 0: no 1: yes;
            unsigned int secondaryPlaneSubsampledY:1; // Sub-sample UV factor on Y 0: no 1: yes;
            unsigned int numberOfExtraPlanes:2;       // The number of additional, planes for this format 0 - 3;
            unsigned int channel0:4;                  // Fist channel:   One of the Ycbcr_COLOR_CHANNEL;
            unsigned int channel1:4;                  // Second channel: One of the Ycbcr_COLOR_CHANNEL;
            unsigned int channel2:4;                  // Third channel:  One of the Ycbcr_COLOR_CHANNEL;
            unsigned int channel3:4;                  // Fourth channel: One of the Ycbcr_COLOR_CHANNEL;
        };
        unsigned int     planesInfo;
    };
    uint8_t              byteAlign;                   // Device specific memory alignment.(dev->physDevice->textureAlignmentBytes).
    uint8_t              bytePitchAlign;              // Device specific stride alignment (dev->physDevice->texturePitchAlignmentBytes).
    uint8_t              bytePlaneAlign;              // Device specific plane alignment. (dev->physDevice->textureAlignmentBytes).
    uint8_t              reserved;                    // reserved for structure alignment.
} YcbcrPlanesLayoutInfo;

static inline size_t YcbcrAlign(size_t toAlign, size_t alignment)
{
    return ((toAlign + (alignment - 1)) & ~(alignment -1));
}

static inline size_t YcbcrAlignPitch(const YcbcrPlanesLayoutInfo* planeInfo, size_t pitch)
{
    return YcbcrAlign(pitch, planeInfo->bytePitchAlign);
}

static inline size_t YcbcrMemoryAlign(const YcbcrPlanesLayoutInfo* planeInfo, size_t pitch)
{
    return YcbcrAlign(pitch, planeInfo->byteAlign);
}

static inline size_t YcbcrPlaneAlign(const YcbcrPlanesLayoutInfo* planeInfo, size_t pitch)
{
    return YcbcrAlign(pitch, planeInfo->bytePlaneAlign);
}

typedef struct YcbcrPrimariesConstants {
    float kb;
    float kr;
} YcbcrPrimariesConstants;

typedef struct YcbcrRangeConstants {
    float cbMax;
    float crMax;
} YcbcrRangeConstants;

typedef struct GammaCoefficients {
    float alpha;              // alpha coefficient
    float beta;               // beta coefficient
    float gamma;              // gamma exponent value
    float kCoeff;             // k-coefficient
    bool  reGamma;             // Use gamma reciprocal.
} GammaCoefficients;

//                                                  cbMax   crMax
#define COLOR_YCBCR_LEVELS_RANGE_DIGITAL()       {   0.5f,  0.5f   }
#define COLOR_YCBCR_LEVELS_RANGE_ANALOG()        { 0.436f,  0.615f }
#define COLOR_YCBCR_LEVELS_RANGE_NVIDIA_COMPAT() {  1.0f,  1.0f   }

static inline YcbcrRangeConstants GetYcbcrRangeConstants(YcbcrLevelsRange levelsRange)
{
    static const YcbcrRangeConstants YcbcrLevelsDigitalConstants = COLOR_YCBCR_LEVELS_RANGE_DIGITAL();
    static const YcbcrRangeConstants YcbcrLevelsAnalogConstants = COLOR_YCBCR_LEVELS_RANGE_ANALOG();
    static const YcbcrRangeConstants YcbcrLevelsNvidiaConstants = COLOR_YCBCR_LEVELS_RANGE_NVIDIA_COMPAT();
    static const YcbcrRangeConstants YcbcrLevelsUnsupported = COLOR_YCBCR_LEVELS_RANGE_NVIDIA_COMPAT();

    switch (levelsRange) {
    case YcbcrLevelsDigital:
        return YcbcrLevelsDigitalConstants;
    case YcbcrLevelsAnalog:
        return YcbcrLevelsAnalogConstants;
    case YcbcrLevelsNvidiaCompat:
        return YcbcrLevelsNvidiaConstants;
    }

    return YcbcrLevelsUnsupported;
}

//                                                   cb        cr
#define COLOR_PRIMARIES_BT709()                  { 0.0722f, 0.2126f }
#define COLOR_PRIMARIES_BT601_EBU()              {  0.114f,  0.299f }
#define COLOR_PRIMARIES_BT601_SMTPE()            {  0.087f,  0.212f }
#define COLOR_PRIMARIES_BT2020()                 { 0.0593f, 0.2627f }

static inline YcbcrPrimariesConstants GetYcbcrPrimariesConstants(YcbcrBtStandard primariesStandards)
{
    static const YcbcrPrimariesConstants YcbcrBtStandardBt709Constants = COLOR_PRIMARIES_BT709();
    static const YcbcrPrimariesConstants YcbcrBtStandardBt601EbuConstants = COLOR_PRIMARIES_BT601_EBU();
    static const YcbcrPrimariesConstants YcbcrBtStandardBt601SmtpeConstants = COLOR_PRIMARIES_BT601_SMTPE();
    static const YcbcrPrimariesConstants YcbcrBtStandardBt2020Constants = COLOR_PRIMARIES_BT2020();
    static const YcbcrPrimariesConstants YcbcrBtStandardBtUnsupported =  {0.0, 0.0};

    switch (primariesStandards) {
    case YcbcrBtStandardBt709:
        return YcbcrBtStandardBt709Constants;
    case YcbcrBtStandardBt601Ebu:
        return YcbcrBtStandardBt601EbuConstants;
    case YcbcrBtStandardBt601Smtpe:
        return YcbcrBtStandardBt601SmtpeConstants;
    case YcbcrBtStandardBt2020:
        return YcbcrBtStandardBt2020Constants;
    case YcbcrBtStandardUnknown:
    default:
        break;
    }

    return YcbcrBtStandardBtUnsupported;
}

//                                                  alpha         beta        gamma     kCoef    reGamma
#define ITU_BT_GAMMA_COEFFICIENTS()              { 1.0993f,      0.0181f,     0.45f,     4.5f,    true  }
#define SMPTE170M_GAMMA_COEFFICIENTS()           { 1.0993f,      0.0181f,      2.2f,     4.5f,    false }
#define SRGB_GAMMA_COEFFICIENTS()                {  1.055f,   0.0031308f,      2.4f,   12.92f,    false }
#define INVALID_GAMMA_COEFFICIENTS()             {    0.0f,         0.0f,      0.0f,     0.0f,    false }

class YcbcrGamma {

public:
    YcbcrGamma(float alpha, float beta, float gamma, float k, bool reGama) :
        m_alpha(alpha),
        m_beta(beta),
        m_gamma(reGama ? 1.0/gamma : gamma),
        m_kCoeff(k),
        m_reGamma(reGama ? m_gamma : 1.0 / m_gamma),
        m_alphaMinusOne(m_alpha - 1.0),
        m_reBeta(m_alpha * pow(m_beta, m_reGamma) - m_alphaMinusOne)
    {}

    float applyGamma(float in)
    {
        float out;

        if (in < (float)(m_beta)) {
            out = (float)(in  * m_kCoeff);
        } else {

            if (m_gamma != 2.0) {
                out = (float)(pow(in, (float)m_reGamma));
            } else {
                out = (float)(sqrt(in));
            }

            if (m_alpha > 1.0) {
               out = (float)(m_alpha * out - m_alphaMinusOne);
            }
        }

        return out;
    }

    float reverseGamma(float in)
    {
        float out;

        if (in < (float)(m_reBeta)) {
            out = (float)(in / m_kCoeff);
        } else {

            if (m_alpha > 1.0) {
                out =  (float)((in + m_alphaMinusOne)/m_alpha);
            } else {
                out =  in;
            }

            if (m_gamma != 2.0) {
                out =  (float)(pow(out, (float)m_gamma));
            } else {
                out *=  out;
            }
        }

        return out;
    }

private:
    double m_alpha;              // alpha coefficient
    double m_beta;               // beta coefficient
    double m_gamma;              // gamma exponent value
    double m_kCoeff;             // k-coefficient
    double m_reGamma;            // the reciprocal of gamma.
    double m_alphaMinusOne;
    double m_reBeta;             // help beta-coefficient for delinearization
};

class YcbcrNormalizeColorRange {

public:

    typedef enum YcbcrNames {
        cY  = 0,
        cCb = 1,
        cCr = 2,
        cA =  3,
    } YcbcrNames;

    template <class colorType>
    colorType clamp(colorType val, colorType min, colorType max) const
    {
        if (val > max) {
            return max;
        } else if (val < min) {
            return min;
        }
        return val;
    }

    // Our HW normalizes 10 and 12-bit formats using 16-bit depth, since 8/16-bit are the only formats that the HW does natively support.
    // In addition, the 10/12-bit values are packed at the MSB side of the word and we can, easily treat the
    // 10/12-bit as 16-bit values and work with that. However, an error is accumulated because of this approximation.
    // Therefore, we need to correct the normalization of the values when using 10/12-bit depth. I.e. we would need to re-normalize with the
    // proper bit coefficient.
    // The correct UNORM normalization of all the formats, according to the Khronos spec. should follow:
    //     f = c / (2^b - 1)
    // When it comes to the HW texture unit, this formula is consistent with 8/16-bit formats, however for 10/12-bit formats we need to:
    // 1. Shift (divide) values first, (because the value is packed in the MSB, part of the word) and then;
    // 2. Normalize with the appropriate bit depth coefficients.
    // So the formula we would need to follow for UNORM normalization is:
    //     f = (c >> (16 - b)) / (2^b - 1) OR f = (c / (2 ^ (16 - b))) / (2^b - 1)
    // The hardware is currently handling the formats in the following (incorrect) way, since it is configured to work for 16-bit UNORM format:
    //     f = c / (2^16 - 1)
    // Therefore, when normalizing UNORM 10/12-bit values, we would need to re-normalize/compensate for this:
    //     Hardware produced value       |   Reverse the normalization | apply the shift  |  normalize with the correct bit coefficient
    //     f = (c / (2^16 - 1))          *        (2^16 - 1)           / (2 ^ (16 - b))   /               (2^b - 1)
    //     The processing at the shader side will do:
    //     f = (c / (2^16 - 1))          * (normColor * scaleColor);
    // Furthermore, the YCBCR_RANGE_KHRONOS, YCBCR_RANGE_ITU_FULL, YCBCR_RANGE_ITU_NARROW formulas re-normalize values based on the assumption,
    // that the normalization has been done against the regular Khronos UNORM normalization rules. We would need to tweak the formulas to reflect,
    // our own HW normalization.

    YcbcrNormalizeColorRange(unsigned int bpp = 8, YCBCR_COLOR_RANGE colorRange = YCBCR_COLOR_RANGE_ITU_NARROW,
            bool renormalizeWith16Bit = false,
            bool halfCbCr = false,
            int yMin    = 16, int yMax    = 235, int yOffset = 16,
            int cbCrMin = 16, int cbCrMax = 240, int cbCrOffset = 128)
        : m_bppShift(bpp - 8) // 8 bit is the base.
        , m_bpp16BitShift(m_bppShift ? (16 - bpp) : 0)
        , m_deNormalizeScale{1.0, 1.0, 1.0, 1.0}
        , m_deNormalizeShift{0, 0, 0, 0}
        , m_normalizeScale{1.0, 1.0, 1.0, 1.0}
        , m_normalizeShift{0.0, 0.0, 0.0, 0.0}

    {
        const int twoToBppPower = (1 << bpp);
        const int bppMaxValue = (twoToBppPower - 1);
        const unsigned int renormalizeMsbDivValue = (1 << (16 - bpp));
        const double renormalizeScaleValue = renormalizeWith16Bit ? ((1.0*((1 << 16) - 1))/renormalizeMsbDivValue) : (double)bppMaxValue;
        int yBitAdjOffset     = yOffset << m_bppShift;
        int cbCrBitAdjOffset  = cbCrOffset << m_bppShift;

        if (YCBCR_COLOR_RANGE_ITU_NARROW != colorRange) {
            for (unsigned int i = 0; i < 4; i++) {
                m_min[i] = 0;
                m_max[i] = bppMaxValue;
            }
            m_deNormalizeScale[cY] = m_deNormalizeScale[cCr] = m_deNormalizeScale[cCb] = twoToBppPower;
        }
        m_normalizeShift[cA] = m_normalizeShift[cY] = 0.0;
        m_normalizeScale[cA] = renormalizeWith16Bit ? renormalizeScaleValue/bppMaxValue : 1.0;
        m_deNormalizeShift[cA] = m_deNormalizeShift[cY] = 0;
        m_deNormalizeShift[cCr] = m_deNormalizeShift[cCb] =  cbCrBitAdjOffset;

        switch (colorRange) {
        case YCBCR_COLOR_RANGE_NATURAL:
        {
            m_normalizeScale[cCr] = m_normalizeScale[cCb] = m_normalizeScale[cY]  = renormalizeWith16Bit ? renormalizeScaleValue/bppMaxValue : 1.0;
            m_normalizeShift[cCr] = m_normalizeShift[cCb] = 0.0;
        }
        break;
        case YCBCR_COLOR_RANGE_ITU_FULL:
        {
            m_normalizeScale[cCr] = m_normalizeScale[cCb] = m_normalizeScale[cY] = renormalizeWith16Bit ? renormalizeScaleValue/bppMaxValue : 1.0;
            m_normalizeShift[cCr] = m_normalizeShift[cCb] = -((double)(twoToBppPower >> 1)) / bppMaxValue;
        }
        break;
        case YCBCR_COLOR_RANGE_ITU_NARROW:
        {
            // Y parameters.
            const int   narrowYdiv = (yMax - yMin) << m_bppShift;

            m_min[cY] =    (yMin << m_bppShift);
            m_max[cY] =    (yMax << m_bppShift);

            m_deNormalizeScale[cY] = narrowYdiv;
            m_deNormalizeShift[cY] = yBitAdjOffset;

            m_normalizeScale[cY]   = ((double)renormalizeScaleValue)/narrowYdiv;
            m_normalizeShift[cY]   = -(((double)yBitAdjOffset)/renormalizeScaleValue);

            // CbCr parameters.

            m_min[cCr] = m_min[cCb] = (cbCrMin << m_bppShift);
            m_max[cCr] = m_max[cCb] = (cbCrMax << m_bppShift);

            int  narrowCbCrdiv = (cbCrMax - cbCrMin);

            if (halfCbCr) {
                narrowCbCrdiv /= 2;
            }

            narrowCbCrdiv = narrowCbCrdiv << m_bppShift;

            m_deNormalizeScale[cCr] = m_deNormalizeScale[cCb] = narrowCbCrdiv;
            // m_deNormalizeShift[cCb] = m_deNormalizeShift[cCb] = cbCrBitAdjOffset;

            m_normalizeScale[cCr] = m_normalizeScale[cCb] = ((double)renormalizeScaleValue)/narrowCbCrdiv;
            m_normalizeShift[cCr] = m_normalizeShift[cCb] = -(((double)(cbCrMin))/renormalizeScaleValue);
        }
        break;
        default:
            ;
        }
    }

    void getNormalizeScaleShiftValues(double* scaleColor, double* shiftColor, size_t numValues) const {
        size_t maxNormalizeScale =  sizeof(m_normalizeScale)/sizeof(m_normalizeScale[0]);
        if (numValues > maxNormalizeScale) {
            numValues = maxNormalizeScale;
        }
        memcpy(scaleColor, m_normalizeScale, numValues * sizeof(m_normalizeScale[0]));
        memcpy(shiftColor, m_normalizeShift, numValues * sizeof(m_normalizeShift[0]));
    }

    void getDenormalizeScaleShiftValues(double* scaleColor, int* shiftColor, size_t numValues) const {
        size_t maxDeNormalizeScale =  sizeof(m_deNormalizeScale)/sizeof(m_deNormalizeScale[0]);
        if (numValues > maxDeNormalizeScale) {
            numValues = maxDeNormalizeScale;
        }
        memcpy(scaleColor, m_deNormalizeScale, numValues * sizeof(m_deNormalizeScale[0]));
        memcpy(shiftColor, m_deNormalizeShift, numValues * sizeof(m_deNormalizeShift[0]));
    }

    template <class colorType>
    void clampIntValues(colorType intColor[3]) const {
        for (int i = 0; i < 3; i++) {
            intColor[i] = clamp<colorType>(intColor[i],  m_min[i], m_max[i]);
        }
    }

    template <class colorType>
    void getIntValues(const float normColor[3], colorType intColor[3]) const {
        for (int i = 0; i < 3; i++) {
            // Add .5 to round-up when converting to int.
            uint32_t intYuv = (uint32_t)(normColor[i] * m_deNormalizeScale[i] + m_deNormalizeShift[i]);
            intYuv = clamp<uint32_t>(intYuv,  m_min[i], m_max[i]);
            intColor[i] = (colorType)(intYuv << m_bpp16BitShift); // TODO add this shift to the m_bppShift
        }
    }

    // Clamp color
    void clampNormalizedValues(const float normColor[3], float clampedColor[3]) const {
        const float yMin = 0.0f;
        const float yMax = 1.0f;
        const float cbCrMin = -0.5f;
        const float cbCrMax =  0.5f;
        clampedColor[cY]  =  clamp(normColor[cY],     yMin,    yMax);
        clampedColor[cCb] =  clamp(normColor[cCb], cbCrMin, cbCrMax);
        clampedColor[cCr] =  clamp(normColor[cCr], cbCrMin, cbCrMax);
    }

    template <class colorType>
    void getNormalizedValues(const colorType intColor[3], float normColor[3], bool clamp = true) const {
        for (int i = 0; i < 3; i++) {
            colorType intShiftedColor = intColor[i] >> m_bpp16BitShift; // TODO add this shift to the m_bppShift
            normColor[i] = (colorType)(intShiftedColor * m_normalizeScale[i] + m_normalizeShift[i]);
        }

        if (clamp) {
            clampNormalizedValues(normColor, normColor);
        }
    }

    void NormalizeYCbCrString(std::stringstream& outStr, const char* prependLine = "    ") const
    {
        // TODO:  No gamma correction yet.
        outStr << prependLine << "const vec3 normalizeShiftYCbCr  = vec3(" <<
                                                                  (float)m_normalizeShift[cY]  << ", " <<
                                                                  (float)m_normalizeShift[cCb] << ", " <<
                                                                  (float)m_normalizeShift[cCr] << ");\n";
        outStr << prependLine << "const vec3 m_normalizeScaleYCbCr  = vec3(" <<
                                                                  (float)m_normalizeScale[cY]  << ", " <<
                                                                  (float)m_normalizeScale[cCb] << ", " <<
                                                                  (float)m_normalizeScale[cCr] << ");\n";

        outStr << prependLine << "yuvNorm = ((yuv + normalizeShiftYCbCr) * m_normalizeScaleYCbCr);\n";
    }

private:
    int           m_min[4];
    int           m_max[4];
    unsigned int  m_bppShift;
    unsigned int  m_bpp16BitShift;
    double        m_deNormalizeScale[4];
    int           m_deNormalizeShift[4];
    double        m_normalizeScale[4];
    double        m_normalizeShift[4];
};

class YcbcrBtMatrix {
    typedef enum CCHAN_NAMES {
        cR = 0,
        cY = 0,
        cG = 1,
        cCb = 1,
        cU = 1,
        cB = 2,
        cCr = 2,
        cV = 2,
        cA = 3,
        cInv = 0xF,
    } CCHAN_NAMES;

    struct YcbcrColorMap {
        YcbcrColorMap() :
            mcY(cY),
            mcCb(cCb),
            mcCr(cCr),
            mcA(cInv)
        { }
        CCHAN_NAMES mcY;
        CCHAN_NAMES mcCb;
        CCHAN_NAMES mcCr;
        CCHAN_NAMES mcA;
    };

    struct RgbColorMap {
        RgbColorMap() :
            mcR(cR),
            mcG(cG),
            mcB(cB),
            mcA(cA)
        {}
        CCHAN_NAMES mcR;
        CCHAN_NAMES mcG;
        CCHAN_NAMES mcB;
        CCHAN_NAMES mcA;
    };

public:
    YcbcrBtMatrix(float kb, float kr, float cbMax, float crMax, YcbcrGamma* gammaFunc = NULL) :
        m_Kb(kb),
        m_Kr(kr),
        m_Kg(1.0 - (kr + kb)),
        m_CbMax(cbMax),
        m_CrMax(crMax),
        m_KCb(cbMax/ (1.0 - kb)),
        m_KCr(crMax/ (1.0 - kr)),
        m_BCbK((1.0 - kb)/m_CbMax),
        m_RCrK((1.0 - kr)/m_CrMax),
        m_GCbK((kb * (1.0 - kb))/(cbMax * m_Kg)),
        m_GCrK((kr * (1.0 - kr))/(crMax * m_Kg)),
        m_gammaFunc(gammaFunc)
    {

    }

    int32_t copyMatrix(float *destMatrix, const float *srcMatrix, uint32_t matrixSize) const {
        if (matrixSize == 9) {

            memcpy(destMatrix, srcMatrix, 9 *sizeof(destMatrix[0]));

            return 9;

        } else if (matrixSize == 16) {
            uint32_t s = 0;
            for (uint32_t d = 0; s < 9; d += 4, s += 3) {
                memcpy(&destMatrix[d], &srcMatrix[s], 3 * sizeof(destMatrix[0]));
            }
            destMatrix[3] =  destMatrix[7] =  destMatrix[11] =  0;
            destMatrix[12] = destMatrix[13] = destMatrix[14] = 0;
            destMatrix[15] = 1.0;
            return 16;
        }

        return -1;
    }

    int32_t GetRgbToYcbcrMatrix(float *transformMatrix, uint32_t matrixSize) const
    {
        const float RGB_TO_Ycbcr[] = {
             (float)m_Kr,           (float)m_Kg,           (float)m_Kb,
             (float)(-m_Kr*m_KCb),  (float)(-m_Kg*m_KCb),  (float)m_CbMax,
             (float)m_CrMax,        (float)(-m_Kg*m_KCr),  (float)(-m_Kb*m_KCr),
        };

        return copyMatrix(transformMatrix, RGB_TO_Ycbcr, matrixSize);
    }

    int32_t GetYcbcrToRgbMatrix(float *transformMatrix, uint32_t matrixSize) const
    {
        const float Ycbcr_TO_RGB[] = {
             1.0f,           0.0f,               (float)(1.0/m_KCr),
             1.0f,          (float)-m_GCbK,      (float)-m_GCrK,
             1.0f,          (float)(1.0/m_KCb),  0.0f,
        };

        return copyMatrix(transformMatrix, Ycbcr_TO_RGB, matrixSize);
    }

    int32_t ConvertRgbToYcbcr(float yuv[3], const float inRgb[3],
            YcbcrColorMap* yuvMap = NULL, RgbColorMap* rgbMap = NULL) const
    {
        float nlRgb[3];
        const float* rgb = inRgb;
        if (m_gammaFunc) {
            for (uint32_t i = 0; i < 3; i++) {
                nlRgb[i] = m_gammaFunc->applyGamma(rgb[i]);
            }
            rgb = nlRgb;
        }

        if (yuvMap && rgbMap) {
            yuv[yuvMap->mcY]  = (float)m_Kr *   rgb[rgbMap->mcR] + (float)m_Kg * rgb[rgbMap->mcG] + (float)m_Kb * rgb[rgbMap->mcB];
            yuv[yuvMap->mcCb] = (float)m_KCb * (rgb[rgbMap->mcB] - yuv[yuvMap->mcY]);
            yuv[yuvMap->mcCr] = (float)m_KCr * (rgb[rgbMap->mcR] - yuv[yuvMap->mcY]);
        } else {
            yuv[cY]  = (float)m_Kr *   rgb[cR] + (float)m_Kg * rgb[cG] + (float)m_Kb * rgb[cB];
            yuv[cCb] = (float)m_KCb * (rgb[cB] - yuv[cY]);
            yuv[cCr] = (float)m_KCr * (rgb[cR] - yuv[cY]);
        }
        return 0;
    }

    std::stringstream&  ConvertRgbToYCbCrString(std::stringstream& outStr, const char* prependLine = "    ",
                                                YcbcrColorMap* yuvMap = NULL,
                                                RgbColorMap* rgbMap = NULL) const
    {
        // TODO:  No gamma correction yet.
        if (yuvMap && rgbMap) {
            outStr << prependLine << "yuv[" << yuvMap->mcY << "]  = " << (float)m_Kr  << " * rgb[" << rgbMap->mcR << "] + " <<
                                                          (float)m_Kg  << " * rgb[" << rgbMap->mcG << "] + " <<
                                                          (float)m_Kb  << " * rgb[" << rgbMap->mcB << ";\n";
            outStr << prependLine << "yuv[" << yuvMap->mcCb << "] = " << (float)m_KCb << " * (rgb[" << rgbMap->mcB << "] - yuv[" << yuvMap->mcY << "]);\n";
            outStr << prependLine << "yuv[" << yuvMap->mcCr << "] = " << (float)m_KCr << " * (rgb[" << rgbMap->mcR << "] - yuv[" << yuvMap->mcY << "]);\n";
        } else {
            outStr << prependLine << "yuv[" << cY << "]  = " << (float)m_Kr << " * rgb[" << cR << "] + " <<
                                                 (float)m_Kg << " * rgb[" << cG << "] + " <<
                                                 (float)m_Kb << " * rgb[" << cB << "];\n";
            outStr << prependLine << "yuv[" << cCb << "] = " << (float)m_KCb << " * (rgb[" << cB << "] - yuv[" << cY << "]);\n";
            outStr << prependLine << "yuv[" << cCr << "] = " << (float)m_KCr << " * (rgb[" << cR << "] - yuv[" << cY << "]);\n";
        }
        return outStr;
    }

    std::stringstream&  ConvertRgbToYCbCrDiscreteChString(std::stringstream& outStr, const char* prependLine = "    ") const
    {
        // TODO:  No gamma correction yet.
        outStr << prependLine << "y  = " << (float)m_Kr << " * r + " <<
                             (float)m_Kg << " * g + " <<
                             (float)m_Kb << " * b;\n";
        outStr << prependLine << "cb = " << (float)m_KCb << " * (b - y);\n";
        outStr << prependLine << "cr = " << (float)m_KCr << " * (r - y);\n";
        return outStr;
    }

    int32_t ConvertRgbToYcbcr2(float yuv[3], const float rgb[3]) const
    {

        yuv[cY] = ((float)m_Kr * rgb[cR]) + ((float)m_Kg * rgb[cG]) + ((float)m_Kb * rgb[cB]);
        yuv[cCb] = -rgb[cR] * (float)(m_Kr / (1.0 - m_Kb)) - (rgb[cG] * (float)(m_Kg / (1.0 - m_Kb))) + rgb[cB];
        yuv[cCr] = rgb[cR] - (rgb[cG] * (float)(m_Kg / (1.0 - m_Kr))) - (rgb[cB] * (float)(m_Kb / (1.0 - m_Kr)));

        return 0;
    }

    int32_t ConvertYcbcrToRgb(float rgb[3], const float yuv[3],
            YcbcrColorMap* yuvMap = NULL, RgbColorMap* rgbMap = NULL) const
    {
        if (yuvMap && rgbMap) {
            rgb[rgbMap->mcR] = yuv[yuvMap->mcY] + yuv[yuvMap->mcCr] * (float)m_RCrK;
            rgb[rgbMap->mcG] = yuv[yuvMap->mcY] - yuv[yuvMap->mcCb] * (float)m_GCbK - yuv[yuvMap->mcCr] * (float)m_GCrK;
            rgb[rgbMap->mcB] = yuv[yuvMap->mcY] + yuv[yuvMap->mcCb] * (float)m_BCbK;
        } else {
            rgb[cR] = yuv[cY] + yuv[cCr] * (float)m_RCrK;
            rgb[cG] = yuv[cY] - yuv[cCb] * (float)m_GCbK - yuv[cCr] * (float)m_GCrK;
            rgb[cB] = yuv[cY] + yuv[cCb] * (float)m_BCbK;
        }

        if (m_gammaFunc) {
            for (uint32_t i = 0; i < 3; i++) {
                rgb[i] = m_gammaFunc->reverseGamma(rgb[i]);
            }
        }

        return 0;
    }

    void ConvertYCbCrToRgbString(std::stringstream& outStr, const char* prependLine = "    ",
                                               YcbcrColorMap* yuvMap = NULL,
                                               RgbColorMap* rgbMap = NULL) const
    {
        if (yuvMap && rgbMap) {
            outStr << prependLine << "rgb[" << rgbMap->mcR << "] = yuv[" << yuvMap->mcY << "] + yuv[" << yuvMap->mcCr << "] * " << (float)m_RCrK << ";\n";
            outStr << prependLine << "rgb[" << rgbMap->mcG << "] = yuv[" << yuvMap->mcY << "] - yuv[" << yuvMap->mcCb << "] * " << (float)m_GCbK <<
                                                                             " - yuv[" << yuvMap->mcCr << "] * " << (float)m_GCrK << ";\n";
            outStr << prependLine << "rgb[" << rgbMap->mcB << "] = yuv[" << yuvMap->mcY << "] + yuv[" << yuvMap->mcCb << "] * " << (float)m_BCbK << ";\n";
        } else {
            outStr << prependLine << "rgb[" << cR << "] = yuv[" << cY << "] + yuv[" << cCr << "] * " << (float)m_RCrK << ";\n";
            outStr << prependLine << "rgb[" << cG << "] = yuv[" << cY << "] - yuv[" << cCb << "] * " << (float)m_GCbK <<
                                                           " - yuv[" << cCr << "] * " << (float)m_GCrK << ";\n";
            outStr << prependLine << "rgb[" << cB << "] = yuv[" << cY << "] + yuv[" << cCb << "] * " << (float)m_BCbK << ";\n";
        }

        // TODO:  No gamma correction yet.
    }

    std::stringstream&  ConvertYCbCrToRgbDiscreteChString(std::stringstream& outStr, const char* prependLine = "    ") const
    {
        outStr << prependLine << "r = y + cr * " << (float)m_RCrK << ";\n";
        outStr << prependLine << "g = y - cb * " << (float)m_GCbK <<
                       " - cr * " << (float)m_GCrK << ";\n";
        outStr << prependLine << "b = y + cb * " << (float)m_BCbK << ";\n";

        // TODO:  No gamma correction yet.
        return outStr;
    }

#ifdef ENABLE_YCBCR_DUMPS
    void dumpConvertRgbToYcbcr() {
        printf("\nRGB to Ycbcr\n");
        printf("yuv[cY]  = %f *   rgb[cR] + %f * rgb[cG] + %f * rgb[cB]\n", m_Kr, m_Kg, m_Kb);
        printf("yuv[cCb] = %f * (rgb[cB] - yuv[cY])\n", m_KCb);
        printf("yuv[cCr] = %f * (rgb[cR] - yuv[cY])\n", m_KCr);
    }

    void dumpConvertYcbcrToRgb() {
        printf("\nYcbcr to RGB\n");
        printf("rgb[cR] = yuv[cY] + yuv[cCr] * %f\n", m_RCrK);
        printf("rgb[cG] = yuv[cY] - yuv[cCb] * %f - yuv[cCr] * %f\n", m_GCbK, m_GCrK);
        printf("rgb[cB] = yuv[cY] + yuv[cCb] * %f\n", m_BCbK);
    }

    // Dump Coefficients
    void dumpCoefficients() {
        printf("\nCoefficients\n");
        printf("m_Kb %f, m_Kr %f, m_Kg %f, m_CbMax %f, m_CrMax %f,\n"
                "\tm_KCb %f, m_KCr %f, m_BCbK %f, m_RCrK %f, m_GCbK %f, m_GCrK %f\n",
                m_Kb, m_Kr, m_Kg, m_CbMax, m_CrMax, m_KCb, m_KCr, m_BCbK, m_RCrK, m_GCbK, m_GCrK);
    }
#endif // ENABLE_YCBCR_DUMPS

private:
    YcbcrBtMatrix();

    double m_Kb;
    double m_Kr;
    double m_Kg;
    double m_CbMax;
    double m_CrMax;
    double m_KCb;
    double m_KCr;
    double m_BCbK;
    double m_RCrK;
    double m_GCbK;
    double m_GCrK;

    YcbcrGamma* m_gammaFunc;
};

#endif /* YCBCR_UTILS_H_ */
