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

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <algorithm>
#include <atomic>
#include <iostream>
#include <vector>
#include <queue>
#include <bitset> // std::bitset

#include "vkvideo_parser/VulkanVideoParserIf.h"
#include "NvVideoParser/nvVulkanVideoParser.h"
#include "NvVideoParser/nvVulkanVideoUtils.h"
#include "vkvideo_parser/PictureBufferBase.h"
#include "VkVideoCore/VkVideoCoreProfile.h"
#include "vkvideo_parser/StdVideoPictureParametersSet.h"

#include "vkvideo_parser/VulkanVideoParser.h"

#undef min
#undef max

static const uint32_t topFieldShift = 0;
static const uint32_t topFieldMask = (1 << topFieldShift);
static const uint32_t bottomFieldShift = 1;
static const uint32_t bottomFieldMask = (1 << bottomFieldShift);
static const uint32_t fieldIsReferenceMask = (topFieldMask | bottomFieldMask);

static const uint32_t MAX_DPB_REF_SLOTS = 16;
static const uint32_t MAX_DPB_REF_AND_SETUP_SLOTS = MAX_DPB_REF_SLOTS + 1; // plus 1 for the current picture (h.264 only)

#define COPYFIELD(pout, pin, name) pout->name = pin->name

namespace NvVulkanDecoder
{

struct nvVideoDecodeH264DpbSlotInfo {
    VkVideoDecodeH264DpbSlotInfoKHR dpbSlotInfo;
    StdVideoDecodeH264ReferenceInfo stdReferenceInfo;

    nvVideoDecodeH264DpbSlotInfo()
        : dpbSlotInfo()
        , stdReferenceInfo()
    {
    }

    const VkVideoDecodeH264DpbSlotInfoKHR* Init(int8_t slotIndex)
    {
        assert((slotIndex >= 0) && (slotIndex < (int8_t)MAX_DPB_REF_AND_SETUP_SLOTS));
        dpbSlotInfo.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_DPB_SLOT_INFO_KHR;
        dpbSlotInfo.pNext = NULL;
        dpbSlotInfo.pStdReferenceInfo = &stdReferenceInfo;
        return &dpbSlotInfo;
    }

    bool IsReference() const
    {
        return (dpbSlotInfo.pStdReferenceInfo == &stdReferenceInfo);
    }

    operator bool() const { return IsReference(); }
    void Invalidate() { memset(this, 0x00, sizeof(*this)); }
};

struct nvVideoDecodeH265DpbSlotInfo {
    VkVideoDecodeH265DpbSlotInfoKHR dpbSlotInfo;
    StdVideoDecodeH265ReferenceInfo stdReferenceInfo;

    nvVideoDecodeH265DpbSlotInfo()
        : dpbSlotInfo()
        , stdReferenceInfo()
    {
    }

    const VkVideoDecodeH265DpbSlotInfoKHR* Init(int8_t slotIndex)
    {
        assert((slotIndex >= 0) && (slotIndex < (int8_t)MAX_DPB_REF_SLOTS));
        dpbSlotInfo.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_DPB_SLOT_INFO_KHR;
        dpbSlotInfo.pNext = NULL;
        dpbSlotInfo.pStdReferenceInfo = &stdReferenceInfo;
        return &dpbSlotInfo;
    }

    bool IsReference() const
    {
        return (dpbSlotInfo.pStdReferenceInfo == &stdReferenceInfo);
    }

    operator bool() const { return IsReference(); }

    void Invalidate() { memset(this, 0x00, sizeof(*this)); }
};
/******************************************************/
//! \struct nvVideoH264PicParameters
//! H.264 picture parameters
/******************************************************/
struct nvVideoH264PicParameters {
    enum { MAX_REF_PICTURES_LIST_ENTRIES = 16 };

    StdVideoDecodeH264PictureInfo stdPictureInfo;
    VkVideoDecodeH264PictureInfoKHR pictureInfo;
    VkVideoDecodeH264SessionParametersAddInfoKHR pictureParameters;
    nvVideoDecodeH264DpbSlotInfo currentDpbSlotInfo;
    nvVideoDecodeH264DpbSlotInfo dpbRefList[MAX_REF_PICTURES_LIST_ENTRIES];
};

/*******************************************************/
//! \struct nvVideoH265PicParameters
//! HEVC picture parameters
/*******************************************************/
struct nvVideoH265PicParameters {
    enum { MAX_REF_PICTURES_LIST_ENTRIES = 16 };

    StdVideoDecodeH265PictureInfo stdPictureInfo;
    VkVideoDecodeH265PictureInfoKHR pictureInfo;
    VkVideoDecodeH265SessionParametersAddInfoKHR pictureParameters;
    nvVideoDecodeH265DpbSlotInfo dpbRefList[MAX_REF_PICTURES_LIST_ENTRIES];
};

struct nvVideoDecodeAV1DpbSlotInfo
{
    enum {
        // Number of reference frame types (including intra type)
        TOTAL_REFS_PER_FRAME = 8,
    };
    VkVideoDecodeAV1DpbSlotInfoKHR dpbSlotInfo{};
    StdVideoDecodeAV1ReferenceInfo stdReferenceInfo;
    const VkVideoDecodeAV1DpbSlotInfoKHR* Init(int8_t slotIndex)
    {
        assert((slotIndex >= 0) && (slotIndex < (int8_t)TOTAL_REFS_PER_FRAME));
        dpbSlotInfo.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_AV1_DPB_SLOT_INFO_KHR;
        dpbSlotInfo.pNext = nullptr;
        dpbSlotInfo.pStdReferenceInfo = &stdReferenceInfo;
        return &dpbSlotInfo;
    }

    void Invalidate() { memset(this, 0x00, sizeof(*this)); }

    // Set the STD data here for AV1.

};

struct nvVideoAV1PicParameters {
    // Maximum number of tiles specified by any defined level
    uint32_t tileOffsets[256];
    uint32_t tileSizes[256];
    uint16_t MiColStarts[64];
    uint16_t MiRowStarts[64];
    uint16_t width_in_sbs_minus_1[64];
    uint16_t height_in_sbs_minus_1[64];
    StdVideoDecodeAV1PictureInfo stdPictureInfo; // memory for the pointer in pictureInfo
    VkVideoDecodeAV1PictureInfoKHR pictureInfo;
    VkVideoDecodeAV1SessionParametersCreateInfoKHR pictureParameters;
    nvVideoDecodeAV1DpbSlotInfo dpbRefList[nvVideoDecodeAV1DpbSlotInfo::TOTAL_REFS_PER_FRAME + 1];
};

static vkPicBuffBase* GetPic(VkPicIf* pPicBuf)
{
    return (vkPicBuffBase*)pPicBuf;
}

// Keeps track of data associated with active internal reference frames
class DpbSlot {
public:
    bool isInUse() { return (m_reserved || m_inUse); }

    bool isAvailable() { return !isInUse(); }

    bool Invalidate()
    {
        bool wasInUse = isInUse();
        if (m_picBuf) {
            m_picBuf->Release();
            m_picBuf = NULL;
        }

        m_reserved = m_inUse = false;

        return wasInUse;
    }

    vkPicBuffBase* getPictureResource() { return m_picBuf; }

    vkPicBuffBase* setPictureResource(vkPicBuffBase* picBuf, int32_t age = 0)
    {
        vkPicBuffBase* oldPic = m_picBuf;

        if (picBuf) {
            picBuf->AddRef();
        }
        m_picBuf = picBuf;

        if (oldPic) {
            oldPic->Release();
        }

        m_pictureId = age;
        return oldPic;
    }

    void Reserve() { m_reserved = true; }

    void MarkInUse(int32_t age = 0)
    {
        m_pictureId = age;
        m_inUse = true;
    }

    int32_t getAge() { return m_pictureId; }

private:
    int32_t m_pictureId; // PictureID at map time (age)
    vkPicBuffBase* m_picBuf; // Associated resource
    int32_t m_reserved : 1;
    int32_t m_inUse : 1;
};

class DpbSlots {
public:
    DpbSlots(uint32_t dpbMaxSize)
        : m_dpbMaxSize(0)
        , m_slotInUseMask(0)
        , m_dpb(m_dpbMaxSize)
        , m_dpbSlotsAvailable()
    {
        Init(dpbMaxSize, false);
    }

    int32_t Init(uint32_t newDpbMaxSize, bool reconfigure)
    {
        assert(newDpbMaxSize <= MAX_DPB_REF_AND_SETUP_SLOTS);

        if (!reconfigure) {
            Deinit();
        }

        if (reconfigure && (newDpbMaxSize < m_dpbMaxSize)) {
            return m_dpbMaxSize;
        }

        uint32_t oldDpbMaxSize = reconfigure ? m_dpbMaxSize : 0;
        m_dpbMaxSize = newDpbMaxSize;

        m_dpb.resize(m_dpbMaxSize);

        for (uint32_t ndx = oldDpbMaxSize; ndx < m_dpbMaxSize; ndx++) {
            m_dpb[ndx].Invalidate();
        }

        for (uint8_t dpbIndx = oldDpbMaxSize; dpbIndx < m_dpbMaxSize; dpbIndx++) {
            m_dpbSlotsAvailable.push(dpbIndx);
        }

        return m_dpbMaxSize;
    }

    void Deinit()
    {
        for (uint32_t ndx = 0; ndx < m_dpbMaxSize; ndx++) {
            m_dpb[ndx].Invalidate();
        }

        while (!m_dpbSlotsAvailable.empty()) {
            m_dpbSlotsAvailable.pop();
        }

        m_dpbMaxSize = 0;
        m_slotInUseMask = 0;
    }

    ~DpbSlots() { Deinit(); }

    int8_t AllocateSlot()
    {
        if (m_dpbSlotsAvailable.empty()) {
            assert(!"No more DPB slots are available");
            return -1;
        }
        int8_t slot = (int8_t)m_dpbSlotsAvailable.front();
        assert((slot >= 0) && ((uint8_t)slot < m_dpbMaxSize));
        m_slotInUseMask |= (1 << slot);
        m_dpbSlotsAvailable.pop();
        m_dpb[slot].Reserve();
        return slot;
    }

    void FreeSlot(int8_t slot)
    {
        assert((uint8_t)slot < m_dpbMaxSize);
        assert(m_dpb[slot].isInUse());
        assert(m_slotInUseMask & (1 << slot));

        m_dpb[slot].Invalidate();
        m_dpbSlotsAvailable.push(slot);
        m_slotInUseMask &= ~(1 << slot);
    }

    DpbSlot& operator[](uint32_t slot)
    {
        assert(slot < m_dpbMaxSize);
        return m_dpb[slot];
    }

    // Return the remapped index given an external decode render target index
    int8_t GetSlotOfPictureResource(vkPicBuffBase* pPic)
    {
        for (int8_t i = 0; i < (int8_t)m_dpbMaxSize; i++) {
            if ((m_slotInUseMask & (1 << i)) && m_dpb[i].isInUse() && (pPic == m_dpb[i].getPictureResource())) {
                return i;
            }
        }
        return -1; // not found
    }

    void MapPictureResource(vkPicBuffBase* pPic, int32_t dpbSlot,
        int32_t age = 0)
    {
        for (uint32_t slot = 0; slot < m_dpbMaxSize; slot++) {
            if ((uint8_t)slot == dpbSlot) {
                m_dpb[slot].setPictureResource(pPic, age);
            } else if (pPic) {
                if (m_dpb[slot].getPictureResource() == pPic) {
                    FreeSlot(slot);
                }
            }
        }
    }

    uint32_t getSlotInUseMask() { return m_slotInUseMask; }

    uint32_t getMaxSize() { return m_dpbMaxSize; }

private:
    uint32_t m_dpbMaxSize;
    uint32_t m_slotInUseMask;
    std::vector<DpbSlot> m_dpb;
    std::queue<uint8_t> m_dpbSlotsAvailable;
};

class VulkanVideoParser : public VkParserVideoDecodeClient,
                          public IVulkanVideoParser {
    friend class IVulkanVideoParser;

public:
    enum { MAX_FRM_CNT = 32 };
    enum { HEVC_MAX_DPB_SLOTS = 16 };
    enum { AVC_MAX_DPB_SLOTS = 17 };
    enum { MAX_REMAPPED_ENTRIES = 20 };

    // H.264 internal DPB structure
    typedef struct dpbH264Entry {
        int8_t dpbSlot;
        // bit0(used_for_reference)=1: top field used for reference,
        // bit1(used_for_reference)=1: bottom field used for reference
        uint32_t used_for_reference : 2;
        uint32_t is_long_term : 1; // 0 = short-term, 1 = long-term
        uint32_t is_non_existing : 1; // 1 = marked as non-existing
        uint32_t is_field_ref : 1; // set if unpaired field or complementary field pair
        union {
            int16_t FieldOrderCnt[2]; // h.264 : 2*32 [top/bottom].
            int32_t PicOrderCnt; // HEVC PicOrderCnt
        };
        union {
            int16_t FrameIdx; // : 16   short-term: FrameNum (16 bits), long-term:
                // LongTermFrameIdx (4 bits)
            int8_t originalDpbIndex; // Original Dpb source Index.
        };
        vkPicBuffBase* m_picBuff; // internal picture reference

        void setReferenceAndTopBottomField(
            bool isReference, bool nonExisting, bool isLongTerm, bool isFieldRef,
            bool topFieldIsReference, bool bottomFieldIsReference, int16_t frameIdx,
            const int16_t fieldOrderCntList[2], vkPicBuffBase* picBuff)
        {
            is_non_existing = nonExisting;
            is_long_term = isLongTerm;
            is_field_ref = isFieldRef;
            if (isReference && isFieldRef) {
                used_for_reference = (bottomFieldIsReference << bottomFieldShift) | (topFieldIsReference << topFieldShift);
            } else {
                used_for_reference = isReference ? 3 : 0;
            }

            FrameIdx = frameIdx;

            FieldOrderCnt[0] = fieldOrderCntList[used_for_reference == 2]; // 0: for progressive and top reference; 1: for
                // bottom reference only.
            FieldOrderCnt[1] = fieldOrderCntList[used_for_reference != 1]; // 0: for top reference only;  1: for bottom
                // reference and progressive.

            dpbSlot = -1;
            m_picBuff = picBuff;
        }

        void setReference(bool isLongTerm, int32_t picOrderCnt,
            vkPicBuffBase* picBuff)
        {
            is_non_existing = (picBuff == NULL);
            is_long_term = isLongTerm;
            is_field_ref = false;
            used_for_reference = (picBuff != NULL) ? 3 : 0;

            PicOrderCnt = picOrderCnt;

            dpbSlot = -1;
            m_picBuff = picBuff;
            originalDpbIndex = -1;
        }

        bool isRef() { return (used_for_reference != 0); }

        StdVideoDecodeH264ReferenceInfoFlags getPictureFlag(bool currentPictureIsProgressive)
        {
            StdVideoDecodeH264ReferenceInfoFlags picFlags = StdVideoDecodeH264ReferenceInfoFlags();
            if (m_dumpParserData)
                std::cout << "\t\t Flags: ";

            if (used_for_reference) {
                if (m_dumpParserData)
                    std::cout << "FRAME_IS_REFERENCE ";
                // picFlags.is_reference = true;
            }

            if (is_long_term) {
                if (m_dumpParserData)
                    std::cout << "IS_LONG_TERM ";
                picFlags.used_for_long_term_reference = true;
            }
            if (is_non_existing) {
                if (m_dumpParserData)
                    std::cout << "IS_NON_EXISTING ";
                picFlags.is_non_existing = true;
            }

            if (is_field_ref) {
                if (m_dumpParserData)
                    std::cout << "IS_FIELD ";
                // picFlags.field_pic_flag = true;
            }

            if (!currentPictureIsProgressive && (used_for_reference & topFieldMask)) {
                if (m_dumpParserData)
                    std::cout << "TOP_FIELD_IS_REF ";
                picFlags.top_field_flag = true;
            }
            if (!currentPictureIsProgressive && (used_for_reference & bottomFieldMask)) {
                if (m_dumpParserData)
                    std::cout << "BOTTOM_FIELD_IS_REF ";
                picFlags.bottom_field_flag = true;
            }

            return picFlags;
        }

        void setH264PictureData(nvVideoDecodeH264DpbSlotInfo* pDpbRefList,
            VkVideoReferenceSlotInfoKHR* pReferenceSlots,
            uint32_t dpbEntryIdx, uint32_t dpbSlotIndex,
            bool currentPictureIsProgressive)
        {
            assert(dpbEntryIdx < AVC_MAX_DPB_SLOTS);
            assert(dpbSlotIndex < AVC_MAX_DPB_SLOTS);

            assert((dpbSlotIndex == (uint32_t)dpbSlot) || is_non_existing);
            pReferenceSlots[dpbEntryIdx].sType = VK_STRUCTURE_TYPE_VIDEO_REFERENCE_SLOT_INFO_KHR;
            pReferenceSlots[dpbEntryIdx].slotIndex = dpbSlotIndex;
            pReferenceSlots[dpbEntryIdx].pNext = pDpbRefList[dpbEntryIdx].Init(dpbSlotIndex);

            StdVideoDecodeH264ReferenceInfo* pRefPicInfo = &pDpbRefList[dpbEntryIdx].stdReferenceInfo;
            pRefPicInfo->FrameNum = FrameIdx;
            if (m_dumpParserData) {
                std::cout << "\tdpbEntryIdx: " << dpbEntryIdx
                          << "dpbSlotIndex: " << dpbSlotIndex
                          << " FrameIdx: " << (int32_t)FrameIdx;
            }
            pRefPicInfo->flags = getPictureFlag(currentPictureIsProgressive);
            pRefPicInfo->PicOrderCnt[0] = FieldOrderCnt[0];
            pRefPicInfo->PicOrderCnt[1] = FieldOrderCnt[1];
            if (m_dumpParserData)
                std::cout << " fieldOrderCnt[0]: " << pRefPicInfo->PicOrderCnt[0]
                          << " fieldOrderCnt[1]: " << pRefPicInfo->PicOrderCnt[1]
                          << std::endl;
        }

        void setH265PictureData(nvVideoDecodeH265DpbSlotInfo* pDpbSlotInfo,
            VkVideoReferenceSlotInfoKHR* pReferenceSlots,
            uint32_t dpbEntryIdx, uint32_t dpbSlotIndex)
        {
            assert(dpbEntryIdx < HEVC_MAX_DPB_SLOTS);
            assert(dpbSlotIndex < HEVC_MAX_DPB_SLOTS);
            assert(isRef());

            assert((dpbSlotIndex == (uint32_t)dpbSlot) || is_non_existing);
            pReferenceSlots[dpbEntryIdx].sType = VK_STRUCTURE_TYPE_VIDEO_REFERENCE_SLOT_INFO_KHR;
            pReferenceSlots[dpbEntryIdx].slotIndex = dpbSlotIndex;
            pReferenceSlots[dpbEntryIdx].pNext = pDpbSlotInfo[dpbEntryIdx].Init(dpbSlotIndex);

            StdVideoDecodeH265ReferenceInfo* pRefPicInfo = &pDpbSlotInfo[dpbEntryIdx].stdReferenceInfo;
            pRefPicInfo->PicOrderCntVal = PicOrderCnt;
            pRefPicInfo->flags.used_for_long_term_reference = is_long_term;

            if (m_dumpParserData) {
                std::cout << "\tdpbIndex: " << dpbSlotIndex
                          << " picOrderCntValList: " << PicOrderCnt;

                std::cout << "\t\t Flags: ";
                std::cout << "FRAME IS REFERENCE ";
                if (pRefPicInfo->flags.used_for_long_term_reference) {
                    std::cout << "IS LONG TERM ";
                }
                std::cout << std::endl;
            }
        }

        void setAV1PictureData(nvVideoDecodeAV1DpbSlotInfo* pDpbSlotInfo,
            VkVideoReferenceSlotInfoKHR* pReferenceSlots,
            uint32_t dpbEntryIdx, uint32_t dpbSlotIndex)
        {
            assert(dpbEntryIdx < STD_VIDEO_AV1_NUM_REF_FRAMES);
            assert(dpbSlotIndex < STD_VIDEO_AV1_NUM_REF_FRAMES);
            assert(isRef());

            assert((dpbSlotIndex == (uint32_t)dpbSlot) || is_non_existing);
            pReferenceSlots[dpbEntryIdx].sType = VK_STRUCTURE_TYPE_VIDEO_REFERENCE_SLOT_INFO_KHR;
            pReferenceSlots[dpbEntryIdx].slotIndex = dpbSlotIndex;
            pReferenceSlots[dpbEntryIdx].pNext = pDpbSlotInfo[dpbEntryIdx].Init(dpbSlotIndex);

            StdVideoDecodeAV1ReferenceInfo* pRefPicInfo = &pDpbSlotInfo[dpbEntryIdx].stdReferenceInfo;
            (void)pRefPicInfo;
#if 0
            // Logical array - maintained by the parser.
            // Vulkan Video parser.cpp -- maintains its own indices.
            // We can use more indices in the parser than the spec. (Ther eis a max of 8 but we can use 16)
            // Reason for single structure for DPB -- the array is passed in the callback (in the proxy of the processor)
            // It checks which references are in use. 
            // 2nd Finds which DPB references were assigned before - and reuses indices.
            // The local array maintains the 
            pRefPicInfo->flags.disable_frame_end_update_cdf = ;
            pRefPicInfo->flags.segmentation_enabled = ;
            pRefPicInfo->base_q_idx = ;
            pRefPicInfo->frame_type = ;
            pRefPicInfo->primary_ref_frame_slot_index = primary_ref_frame_slot_index;
            pRefPicInfo->flags.used_for_long_term_reference = is_long_term;
#endif

            if (m_dumpParserData) {
                std::cout << "\tdpbIndex: " << dpbSlotIndex
                          << " picOrderCntValList: " << PicOrderCnt;

                std::cout << "\t\t Flags: ";
                std::cout << "FRAME IS REFERENCE ";
                //if (pRefPicInfo->flags.used_for_long_term_reference) {
                //    std::cout << "IS LONG TERM ";
                //}
                std::cout << std::endl;
            }
        }

    } dpbH264Entry;

    virtual int32_t AddRef();
    virtual int32_t Release();

    // INvVideoDecoderClient
    virtual VkResult ParseVideoData(VkParserSourceDataPacket* pPacket,
                                    size_t* pParsedBytes,
                                    bool doPartialParsing = false);

    // Interface to allow decoder to communicate with the client implementing
    // INvVideoDecoderClient

    virtual int32_t BeginSequence(
        const VkParserSequenceInfo* pnvsi); // Returns max number of reference frames (always
        // at least 2 for MPEG-2)
    virtual bool AllocPictureBuffer(
        VkPicIf** ppPicBuf); // Returns a new VkPicIf interface
    virtual bool DecodePicture(
        VkParserPictureData* pParserPictureData); // Called when a picture is ready to be decoded
    virtual bool DisplayPicture(
        VkPicIf* pPicBuf,
        int64_t llPTS); // Called when a picture is ready to be displayed
    virtual void UnhandledNALU(
        const uint8_t* /*pbData*/, size_t /*cbData*/) {}; // Called for custom NAL parsing (not required)

    virtual uint32_t GetDecodeCaps()
    {
        // FIXME: Add MVC / SVC support
        uint32_t decode_caps = 0; // NVD_CAPS_MVC | NVD_CAPS_SVC; // !!!
        return decode_caps;
    };

    virtual VkDeviceSize GetBitstreamBuffer(VkDeviceSize size,
                                      VkDeviceSize minBitstreamBufferOffsetAlignment,
                                      VkDeviceSize minBitstreamBufferSizeAlignment,
                                      const uint8_t* pInitializeBufferMemory,
                                      VkDeviceSize initializeBufferMemorySize,
                                      VkSharedBaseObj<VulkanBitstreamBuffer>& bitstreamBuffer);

    const IVulkanVideoDecoderHandler* GetDecoderHandler()
    {
        return m_decoderHandler;
    }

    IVulkanVideoFrameBufferParserCb* GetFrameBufferParserCb()
    {
        return m_videoFrameBufferCb;
    }

    uint32_t GetNumNumDecodeSurfaces() { return m_maxNumDecodeSurfaces; }

protected:
    void Deinitialize();
    VkResult Initialize(
        VkSharedBaseObj<IVulkanVideoDecoderHandler>& decoderHandler,
        VkSharedBaseObj<IVulkanVideoFrameBufferParserCb>& videoFrameBufferCb,
        uint32_t defaultMinBufferSize,
        uint32_t bufferOffsetAlignment,
        uint32_t bufferSizeAlignment,
        bool outOfBandPictureParameters,
        uint32_t errorThreshold);

    VulkanVideoParser(VkVideoCodecOperationFlagBitsKHR codecType,
        uint32_t maxNumDecodeSurfaces, uint32_t maxNumDpbSurfaces,
        uint64_t clockRate);

    virtual ~VulkanVideoParser() { Deinitialize(); }

    bool UpdatePictureParameters(VkSharedBaseObj<StdVideoPictureParametersSet>& pictureParametersObject,
                                 VkSharedBaseObj<VkVideoRefCountBase>& client);

    bool DecodePicture(VkParserPictureData* pParserPictureData,
        vkPicBuffBase* pVkPicBuff,
        VkParserDecodePictureInfo* pDecodePictureInfo);

    int8_t GetPicIdx(vkPicBuffBase*);
    int8_t GetPicIdx(VkPicIf* pPicBuf);
    int8_t GetPicDpbSlot(vkPicBuffBase*);
    int8_t GetPicDpbSlot(int8_t picIndex);
    int8_t SetPicDpbSlot(vkPicBuffBase*, int8_t dpbSlot);
    int8_t SetPicDpbSlot(int8_t picIndex, int8_t dpbSlot);
    uint32_t ResetPicDpbSlots(uint32_t picIndexSlotValidMask);
    bool GetFieldPicFlag(int8_t picIndex);
    bool SetFieldPicFlag(int8_t picIndex, bool fieldPicFlag);

    uint32_t FillDpbH264State(const VkParserPictureData* pd,
        const VkParserH264DpbEntry* dpbIn,
        uint32_t maxDpbInSlotsInUse,
        nvVideoDecodeH264DpbSlotInfo* pDpbRefList,
        uint32_t maxRefPictures,
        VkVideoReferenceSlotInfoKHR* pReferenceSlots,
        int8_t* pGopReferenceImagesIndexes,
        StdVideoDecodeH264PictureInfoFlags currPicFlags,
        int32_t* pCurrAllocatedSlotIndex);
    uint32_t FillDpbH265State(const VkParserPictureData* pd,
        const VkParserHevcPictureData* pin,
        nvVideoDecodeH265DpbSlotInfo* pDpbSlotInfo,
        StdVideoDecodeH265PictureInfo* pStdPictureInfo,
        uint32_t maxRefPictures,
        VkVideoReferenceSlotInfoKHR* pReferenceSlots,
        int8_t* pGopReferenceImagesIndexes,
        int32_t* pCurrAllocatedSlotIndex);
    uint32_t FillDpbAV1State(const VkParserPictureData* pd,
        VkParserAv1PictureData* pin,
        nvVideoDecodeAV1DpbSlotInfo* pDpbSlotInfo,
        StdVideoDecodeAV1PictureInfo* pStdPictureInfo,
        uint32_t maxRefPictures,
        VkVideoReferenceSlotInfoKHR* pReferenceSlots,
        int8_t* pGopReferenceImagesIndexes,
        int32_t* pCurrAllocatedSlotIndex);

    int8_t AllocateDpbSlotForCurrentH264(
        vkPicBuffBase* pPic, StdVideoDecodeH264PictureInfoFlags currPicFlags,
        int8_t presetDpbSlot);
    int8_t AllocateDpbSlotForCurrentH265(vkPicBuffBase* pPic, bool isReference,
                                         int8_t presetDpbSlot);
    int8_t AllocateDpbSlotForCurrentAV1(vkPicBuffBase* pPic, bool isReference,
                                         int8_t presetDpbSlot);
    

protected:
    VkSharedBaseObj<VulkanVideoDecodeParser>    m_vkParser;
    VkSharedBaseObj<IVulkanVideoDecoderHandler> m_decoderHandler;
    VkSharedBaseObj<IVulkanVideoFrameBufferParserCb> m_videoFrameBufferCb;
    std::atomic<int32_t> m_refCount;
    VkVideoCodecOperationFlagBitsKHR m_codecType;
    uint32_t m_maxNumDecodeSurfaces;
    uint32_t m_maxNumDpbSlots;
    uint64_t m_clockRate;
    VkParserSequenceInfo m_nvsi;
    int32_t m_nCurrentPictureID;
    uint32_t m_dpbSlotsMask;
    uint32_t m_fieldPicFlagMask;
    DpbSlots m_dpb;
    uint32_t m_outOfBandPictureParameters : 1;
    uint32_t m_inlinedPictureParametersUseBeginCoding : 1;
    int8_t m_pictureToDpbSlotMap[MAX_FRM_CNT];

public:
    static bool m_dumpParserData;
    static bool m_dumpDpbData;
};

bool VulkanVideoParser::m_dumpParserData = false;
bool VulkanVideoParser::m_dumpDpbData = false;

bool VulkanVideoParser::DecodePicture(VkParserPictureData* pd)
{
    bool result = false;

    if (!pd->pCurrPic) {
        return result;
    }

    vkPicBuffBase* pVkPicBuff = GetPic(pd->pCurrPic);
    const int32_t picIdx = pVkPicBuff ? pVkPicBuff->m_picIdx : -1;
    if (picIdx >= VulkanVideoParser::MAX_FRM_CNT) {
        assert(0);
        return result;
    }

    if (m_dumpParserData) {
        std::cout
            << "\t ==> VulkanVideoParser::DecodePicture " << picIdx << std::endl
            << "\t\t progressive: " << (bool)pd->progressive_frame
            << // Frame is progressive
            "\t\t field: " << (bool)pd->field_pic_flag << std::endl
            << // 0 = frame picture, 1 = field picture
            "\t\t\t bottom_field: " << (bool)pd->bottom_field_flag
            << // 0 = top field, 1 = bottom field (ignored if field_pic_flag=0)
            "\t\t\t second_field: " << (bool)pd->second_field
            << // Second field of a complementary field pair
            "\t\t\t top_field: " << (bool)pd->top_field_first << std::endl
            << // Frame pictures only
            "\t\t repeat_first: " << pd->repeat_first_field
            << // For 3:2 pulldown (number of additional fields, 2 = frame
            // doubling, 4 = frame tripling)
            "\t\t ref_pic: " << (bool)pd->ref_pic_flag
            << std::endl; // Frame is a reference frame
    }

    VkParserDecodePictureInfo decodePictureInfo = VkParserDecodePictureInfo();

    decodePictureInfo.pictureIndex = picIdx;
    decodePictureInfo.flags.progressiveFrame = pd->progressive_frame;
    decodePictureInfo.flags.fieldPic = pd->field_pic_flag; // 0 = frame picture, 1 = field picture
    decodePictureInfo.flags.repeatFirstField = pd->repeat_first_field; // For 3:2 pulldown (number of additional fields,
        // 2 = frame doubling, 4 = frame tripling)
    decodePictureInfo.flags.refPic = pd->ref_pic_flag; // Frame is a reference frame

    // Mark the first field as unpaired Detect unpaired fields
    if (pd->field_pic_flag) {
        decodePictureInfo.flags.bottomField = pd->bottom_field_flag; // 0 = top field, 1 = bottom field (ignored if
            // field_pic_flag=0)
        decodePictureInfo.flags.secondField = pd->second_field; // Second field of a complementary field pair
        decodePictureInfo.flags.topFieldFirst = pd->top_field_first; // Frame pictures only

        if (!pd->second_field) {
            decodePictureInfo.flags.unpairedField = true; // Incomplete (half) frame.
        } else {
            if (decodePictureInfo.flags.unpairedField) {
                decodePictureInfo.flags.syncToFirstField = true;
                decodePictureInfo.flags.unpairedField = false;
            }
        }
    }

    decodePictureInfo.frameSyncinfo.unpairedField = decodePictureInfo.flags.unpairedField;
    decodePictureInfo.frameSyncinfo.syncToFirstField = decodePictureInfo.flags.syncToFirstField;

    return DecodePicture(pd, pVkPicBuff, &decodePictureInfo);
}

bool VulkanVideoParser::DisplayPicture(VkPicIf* pPicBuff, int64_t timestamp)
{
    bool result = false;

    vkPicBuffBase* pVkPicBuff = GetPic(pPicBuff);
    assert(pVkPicBuff);

    int32_t picIdx = pVkPicBuff ? pVkPicBuff->m_picIdx : -1;

    if (m_dumpParserData) {
        std::cout << "\t ======================< " << picIdx
                  << " >============================" << std::endl;
        std::cout << "\t ==> VulkanVideoParser::DisplayPicture " << picIdx
                  << std::endl;
    }
    assert(picIdx != -1);

    assert(m_videoFrameBufferCb);
    if (m_videoFrameBufferCb && (picIdx != -1)) {
        VulkanVideoDisplayPictureInfo dispInfo = VulkanVideoDisplayPictureInfo();
        dispInfo.timestamp = (VkVideotimestamp)timestamp;

        int32_t retVal = m_videoFrameBufferCb->QueueDecodedPictureForDisplay(
            (int8_t)picIdx, &dispInfo);
        if (picIdx == retVal) {
            result = true;
        } else {
            assert(!"QueueDecodedPictureForDisplay failed");
        }
    }

    if (m_dumpParserData) {
        std::cout << "\t <== VulkanVideoParser::DisplayPicture " << picIdx
                  << std::endl;
        std::cout << "\t ======================< " << picIdx
                  << " >============================" << std::endl;
    }
    return result;
}

bool VulkanVideoParser::AllocPictureBuffer(VkPicIf** ppPicBuff)
{
    bool result = false;

    assert(m_videoFrameBufferCb);
    if (m_videoFrameBufferCb) {
        *ppPicBuff = m_videoFrameBufferCb->ReservePictureBuffer();
        if (*ppPicBuff) {
            result = true;
        }
    }

    if (!result) {
        *ppPicBuff = (VkPicIf*)NULL;
    }

    return result;
}

VkDeviceSize VulkanVideoParser::GetBitstreamBuffer(VkDeviceSize size,
                                             VkDeviceSize minBitstreamBufferOffsetAlignment,
                                             VkDeviceSize minBitstreamBufferSizeAlignment,
                                             const uint8_t* pInitializeBufferMemory,
                                             VkDeviceSize initializeBufferMemorySize,
                                             VkSharedBaseObj<VulkanBitstreamBuffer>& bitstreamBuffer)
{
    // Forward the request to the Vulkan decoder handler
    return m_decoderHandler->GetBitstreamBuffer(size,
                                                minBitstreamBufferOffsetAlignment,
                                                minBitstreamBufferSizeAlignment,
                                                pInitializeBufferMemory,
                                                initializeBufferMemorySize,
                                                bitstreamBuffer);
}

int32_t VulkanVideoParser::AddRef() { return ++m_refCount; }

int32_t VulkanVideoParser::Release()
{
    uint32_t ret;
    ret = --m_refCount;
    // Destroy the device if refcount reaches zero
    if (ret == 0) {
        delete this;
    }
    return ret;
}

VulkanVideoParser::VulkanVideoParser(VkVideoCodecOperationFlagBitsKHR codecType,
    uint32_t maxNumDecodeSurfaces,
    uint32_t maxNumDpbSurfaces,
    uint64_t clockRate)
    : m_vkParser()
    , m_decoderHandler()
    , m_videoFrameBufferCb()
    , m_refCount(0)
    , m_codecType(codecType)
    , m_maxNumDecodeSurfaces(maxNumDecodeSurfaces)
    , m_maxNumDpbSlots(maxNumDpbSurfaces)
    , m_clockRate(clockRate)
    , m_nCurrentPictureID(0)
    , m_dpbSlotsMask(0)
    , m_fieldPicFlagMask(0)
    , m_dpb(3)
    , m_outOfBandPictureParameters(true)
    , m_inlinedPictureParametersUseBeginCoding(false)
{
    memset(&m_nvsi, 0, sizeof(m_nvsi));
    for (uint32_t picId = 0; picId < MAX_FRM_CNT; picId++) {
        m_pictureToDpbSlotMap[picId] = -1;
    }
}

static void nvParserLog(const char* format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    printf(format, argptr);
    va_end(argptr);
}

VkResult VulkanVideoParser::Initialize(
    VkSharedBaseObj<IVulkanVideoDecoderHandler>& decoderHandler,
    VkSharedBaseObj<IVulkanVideoFrameBufferParserCb>& videoFrameBufferCb,
    uint32_t defaultMinBufferSize,
    uint32_t bufferOffsetAlignment,
    uint32_t bufferSizeAlignment,
    bool outOfBandPictureParameters,
    uint32_t errorThreshold)
{
    Deinitialize();

    m_outOfBandPictureParameters = outOfBandPictureParameters;
    m_decoderHandler = decoderHandler;
    m_videoFrameBufferCb = videoFrameBufferCb;

    memset(&m_nvsi, 0, sizeof(m_nvsi));

    VkParserInitDecodeParameters nvdp;

    memset(&nvdp, 0, sizeof(nvdp));
    nvdp.interfaceVersion = NV_VULKAN_VIDEO_PARSER_API_VERSION;
    nvdp.pClient = reinterpret_cast<VkParserVideoDecodeClient*>(this);

    nvdp.defaultMinBufferSize = defaultMinBufferSize;
    nvdp.bufferOffsetAlignment = bufferOffsetAlignment;
    nvdp.bufferSizeAlignment = bufferSizeAlignment;

    nvdp.referenceClockRate = m_clockRate;
    nvdp.errorThreshold = errorThreshold;
    nvdp.outOfBandPictureParameters = outOfBandPictureParameters;

    static const VkExtensionProperties h264StdExtensionVersion = { VK_STD_VULKAN_VIDEO_CODEC_H264_DECODE_EXTENSION_NAME, VK_STD_VULKAN_VIDEO_CODEC_H264_DECODE_SPEC_VERSION };
    static const VkExtensionProperties h265StdExtensionVersion = { VK_STD_VULKAN_VIDEO_CODEC_H265_DECODE_EXTENSION_NAME, VK_STD_VULKAN_VIDEO_CODEC_H265_DECODE_SPEC_VERSION };
    static const VkExtensionProperties av1StdExtensionVersion = { VK_STD_VULKAN_VIDEO_CODEC_AV1_DECODE_EXTENSION_NAME, VK_STD_VULKAN_VIDEO_CODEC_AV1_DECODE_SPEC_VERSION };

    const VkExtensionProperties* pStdExtensionVersion = NULL;
    if (m_codecType == VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR) {
        pStdExtensionVersion = &h264StdExtensionVersion;
    } else if (m_codecType == VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR) {
        pStdExtensionVersion = &h265StdExtensionVersion;
    } else if (m_codecType == VK_VIDEO_CODEC_OPERATION_DECODE_AV1_BIT_KHR) {
        pStdExtensionVersion = &av1StdExtensionVersion;
    } else {
        assert(!"Unsupported codec type");
        return VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR;
    }

    return CreateVulkanVideoDecodeParser(m_codecType, pStdExtensionVersion, &nvParserLog, 0, &nvdp, m_vkParser);
}

void VulkanVideoParser::Deinitialize()
{
    m_vkParser = nullptr;
    m_decoderHandler = nullptr;
    m_videoFrameBufferCb = nullptr;
}

VkResult VulkanVideoParser::ParseVideoData(VkParserSourceDataPacket* pPacket,
                                           size_t *pParsedBytes,
                                           bool doPartialParsing)
{
    VkParserBitstreamPacket pkt;
    VkResult result;

    memset(&pkt, 0, sizeof(pkt));
    if (pPacket->flags & VK_PARSER_PKT_DISCONTINUITY) {
        // Handle discontinuity separately, in order to flush before any new
        // content
        pkt.bDiscontinuity = true;
        m_vkParser->ParseByteStream(&pkt); // Send a NULL discontinuity packet
    }
    pkt.pByteStream = pPacket->payload;
    pkt.nDataLength = pPacket->payload_size;
    pkt.bEOS = !!(pPacket->flags & VK_PARSER_PKT_ENDOFSTREAM);
    pkt.bEOP = !!(pPacket->flags & VK_PARSER_PKT_ENDOFPICTURE);
    pkt.bPTSValid = !!(pPacket->flags & VK_PARSER_PKT_TIMESTAMP);
    pkt.llPTS = pPacket->timestamp;
    pkt.bPartialParsing = doPartialParsing;
    if (m_vkParser->ParseByteStream(&pkt, pParsedBytes)) {
        result = VK_SUCCESS;
    } else {
        result = VK_ERROR_INITIALIZATION_FAILED;
    }

    if (pkt.bEOS) {
        // Flush any pending frames after EOS
    }
    return result;
}

int8_t VulkanVideoParser::GetPicIdx(vkPicBuffBase* pPicBuf)
{
    if (pPicBuf) {
        int32_t picIndex = pPicBuf->m_picIdx;
        if ((picIndex >= 0) && ((uint32_t)picIndex < m_maxNumDecodeSurfaces)) {
            return (int8_t)picIndex;
        }
    }
    return -1;
}

int8_t VulkanVideoParser::GetPicIdx(VkPicIf* pPicBuf)
{
    return GetPicIdx(GetPic(pPicBuf));
}

int8_t VulkanVideoParser::GetPicDpbSlot(int8_t picIndex)
{
    return m_pictureToDpbSlotMap[picIndex];
}

int8_t VulkanVideoParser::GetPicDpbSlot(vkPicBuffBase* pPicBuf)
{
    int8_t picIndex = GetPicIdx(pPicBuf);
    assert((picIndex >= 0) && ((uint32_t)picIndex < m_maxNumDecodeSurfaces));
    return GetPicDpbSlot(picIndex);
}

bool VulkanVideoParser::GetFieldPicFlag(int8_t picIndex)
{
    assert((picIndex >= 0) && ((uint32_t)picIndex < m_maxNumDecodeSurfaces));
    return (m_fieldPicFlagMask & (1 << (uint32_t)picIndex));
}

bool VulkanVideoParser::SetFieldPicFlag(int8_t picIndex, bool fieldPicFlag)
{
    assert((picIndex >= 0) && ((uint32_t)picIndex < m_maxNumDecodeSurfaces));
    bool oldFieldPicFlag = GetFieldPicFlag(picIndex);

    if (fieldPicFlag) {
        m_fieldPicFlagMask |= (1 << (uint32_t)picIndex);
    } else {
        m_fieldPicFlagMask &= ~(1 << (uint32_t)picIndex);
    }

    return oldFieldPicFlag;
}

int8_t VulkanVideoParser::SetPicDpbSlot(int8_t picIndex, int8_t dpbSlot)
{
    int8_t oldDpbSlot = m_pictureToDpbSlotMap[picIndex];
    m_pictureToDpbSlotMap[picIndex] = dpbSlot;
    if (dpbSlot >= 0) {
        m_dpbSlotsMask |= (1 << picIndex);
    } else {
        m_dpbSlotsMask &= ~(1 << picIndex);
        if (oldDpbSlot >= 0) {
            m_dpb.FreeSlot(oldDpbSlot);
        }
    }
    return oldDpbSlot;
}

int8_t VulkanVideoParser::SetPicDpbSlot(vkPicBuffBase* pPicBuf,
    int8_t dpbSlot)
{
    int8_t picIndex = GetPicIdx(pPicBuf);
    assert((picIndex >= 0) && ((uint32_t)picIndex < m_maxNumDecodeSurfaces));
    return SetPicDpbSlot(picIndex, dpbSlot);
}

uint32_t VulkanVideoParser::ResetPicDpbSlots(uint32_t picIndexSlotValidMask)
{
    uint32_t resetSlotsMask = ~(picIndexSlotValidMask | ~m_dpbSlotsMask);
    if (resetSlotsMask != 0) {
        for (uint32_t picIdx = 0;
             ((picIdx < m_maxNumDecodeSurfaces) && resetSlotsMask); picIdx++) {
            if (resetSlotsMask & (1 << picIdx)) {
                resetSlotsMask &= ~(1 << picIdx);
                if (m_dumpDpbData) {
                    printf(";;; Resetting picIdx %d, was using dpb slot %d\n", picIdx, m_pictureToDpbSlotMap[picIdx]);
                }
                SetPicDpbSlot(picIdx, -1);
            }
        }
    }
    return m_dpbSlotsMask;
}

int32_t VulkanVideoParser::BeginSequence(const VkParserSequenceInfo* pnvsi)
{
    bool sequenceUpdate = ((m_nvsi.nMaxWidth != 0) && (m_nvsi.nMaxHeight != 0)) ? true : false;

    uint32_t maxDpbSlots =  (pnvsi->eCodec == VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR) ?
        MAX_DPB_REF_AND_SETUP_SLOTS : MAX_DPB_REF_SLOTS;

    if (pnvsi->eCodec == VK_VIDEO_CODEC_OPERATION_DECODE_AV1_BIT_KHR) {
        maxDpbSlots = 9;
    }

    uint32_t configDpbSlots = (pnvsi->nMinNumDpbSlots > 0) ? pnvsi->nMinNumDpbSlots : maxDpbSlots;
    configDpbSlots = std::min<uint32_t>(configDpbSlots, maxDpbSlots);

    bool sequenceReconfigureFormat = false;
    bool sequenceReconfigureCodedExtent = false;
    if (sequenceUpdate) {
        if ((pnvsi->eCodec != m_nvsi.eCodec) ||
         (pnvsi->nChromaFormat != m_nvsi.nChromaFormat) || (pnvsi->uBitDepthLumaMinus8 != m_nvsi.uBitDepthLumaMinus8) ||
                                                           (pnvsi->uBitDepthChromaMinus8 != m_nvsi.uBitDepthChromaMinus8) ||
        (pnvsi->bProgSeq != m_nvsi.bProgSeq)) {
            sequenceReconfigureFormat = true;
        }

        if ((pnvsi->nCodedWidth != m_nvsi.nCodedWidth) || (pnvsi->nCodedHeight != m_nvsi.nCodedHeight)) {
            sequenceReconfigureCodedExtent = true;
        }

    }

    m_nvsi = *pnvsi;
    m_nvsi.nMaxWidth = pnvsi->nCodedWidth;
    m_nvsi.nMaxHeight = pnvsi->nCodedHeight;

    m_maxNumDecodeSurfaces = pnvsi->nMinNumDecodeSurfaces;

    if (m_decoderHandler) {
        VkParserDetectedVideoFormat detectedFormat;
        uint8_t raw_seqhdr_data[1024]; /* Output the sequence header data, currently
                                      not used */

        memset(&detectedFormat, 0, sizeof(detectedFormat));

        detectedFormat.sequenceUpdate = sequenceUpdate;
        detectedFormat.sequenceReconfigureFormat = sequenceReconfigureFormat;
        detectedFormat.sequenceReconfigureCodedExtent = sequenceReconfigureCodedExtent;

        detectedFormat.codec = pnvsi->eCodec;
        detectedFormat.frame_rate.numerator = NV_FRAME_RATE_NUM(pnvsi->frameRate);
        detectedFormat.frame_rate.denominator = NV_FRAME_RATE_DEN(pnvsi->frameRate);
        detectedFormat.progressive_sequence = pnvsi->bProgSeq;
        detectedFormat.coded_width = pnvsi->nCodedWidth;
        detectedFormat.coded_height = pnvsi->nCodedHeight;
        detectedFormat.display_area.right = pnvsi->nDisplayWidth;
        detectedFormat.display_area.bottom = pnvsi->nDisplayHeight;
        detectedFormat.filmGrainUsed = pnvsi->hasFilmGrain;

        if ((StdChromaFormatIdc)pnvsi->nChromaFormat == chroma_format_idc_420) {
            detectedFormat.chromaSubsampling = VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR;
        } else if ((StdChromaFormatIdc)pnvsi->nChromaFormat == chroma_format_idc_422) {
            detectedFormat.chromaSubsampling = VK_VIDEO_CHROMA_SUBSAMPLING_422_BIT_KHR;
        } else if ((StdChromaFormatIdc)pnvsi->nChromaFormat == chroma_format_idc_444) {
            detectedFormat.chromaSubsampling = VK_VIDEO_CHROMA_SUBSAMPLING_444_BIT_KHR;
        } else {
            assert(!"Invalid chroma sub-sampling format");
        }

        switch (pnvsi->uBitDepthLumaMinus8) {
        case 0:
            detectedFormat.lumaBitDepth = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR;
            break;
        case 2:
            detectedFormat.lumaBitDepth = VK_VIDEO_COMPONENT_BIT_DEPTH_10_BIT_KHR;
            break;
        case 4:
            detectedFormat.lumaBitDepth = VK_VIDEO_COMPONENT_BIT_DEPTH_12_BIT_KHR;
            break;
        default:
            assert(false);
        }

        switch (pnvsi->uBitDepthChromaMinus8) {
        case 0:
            detectedFormat.chromaBitDepth = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR;
            break;
        case 2:
            detectedFormat.chromaBitDepth = VK_VIDEO_COMPONENT_BIT_DEPTH_10_BIT_KHR;
            break;
        case 4:
            detectedFormat.chromaBitDepth = VK_VIDEO_COMPONENT_BIT_DEPTH_12_BIT_KHR;
            break;
        default:
            assert(false);
        }

        detectedFormat.bit_depth_luma_minus8 = pnvsi->uBitDepthLumaMinus8;
        detectedFormat.bit_depth_chroma_minus8 = pnvsi->uBitDepthChromaMinus8;
        detectedFormat.bitrate = pnvsi->lBitrate;
        detectedFormat.display_aspect_ratio.x = pnvsi->lDARWidth;
        detectedFormat.display_aspect_ratio.y = pnvsi->lDARHeight;
        detectedFormat.video_signal_description.video_format = pnvsi->lVideoFormat;
        detectedFormat.video_signal_description.video_full_range_flag = pnvsi->uVideoFullRange;
        detectedFormat.video_signal_description.color_primaries = pnvsi->lColorPrimaries;
        detectedFormat.video_signal_description.transfer_characteristics = pnvsi->lTransferCharacteristics;
        detectedFormat.video_signal_description.matrix_coefficients = pnvsi->lMatrixCoefficients;
        detectedFormat.seqhdr_data_length = (uint32_t)std::min((size_t)pnvsi->cbSequenceHeader, sizeof(raw_seqhdr_data));
        detectedFormat.minNumDecodeSurfaces = pnvsi->nMinNumDecodeSurfaces;
        detectedFormat.maxNumDpbSlots = configDpbSlots;
        detectedFormat.codecProfile = pnvsi->codecProfile;

        if (detectedFormat.seqhdr_data_length > 0) {
            memcpy(raw_seqhdr_data, pnvsi->SequenceHeaderData,
                detectedFormat.seqhdr_data_length);
        }
        int32_t maxDecodeRTs = m_decoderHandler->StartVideoSequence(&detectedFormat);
        // nDecodeRTs <= 0 means SequenceCallback failed
        // nDecodeRTs  = 1 means SequenceCallback succeeded
        // nDecodeRTs  > 1 means we need to overwrite the MaxNumDecodeSurfaces
        if (maxDecodeRTs <= 0) {
            return 0;
        }
        // MaxNumDecodeSurface may not be correctly calculated by the client while
        // parser creation so overwrite it with NumDecodeSurface. (only if nDecodeRT
        // > 1)
        if (maxDecodeRTs > 1) {
            m_maxNumDecodeSurfaces = maxDecodeRTs;
        }
    } else {
        assert(!"m_pDecoderHandler is NULL");
    }

    m_maxNumDpbSlots = m_dpb.Init(configDpbSlots, sequenceUpdate /* reconfigure the DPB size if true */);

    return m_maxNumDecodeSurfaces;
}

// FillDpbState
uint32_t VulkanVideoParser::FillDpbH264State(
    const VkParserPictureData* pd, const VkParserH264DpbEntry* dpbIn,
    uint32_t maxDpbInSlotsInUse, nvVideoDecodeH264DpbSlotInfo* pDpbRefList,
    uint32_t /*maxRefPictures*/, VkVideoReferenceSlotInfoKHR* pReferenceSlots,
    int8_t* pGopReferenceImagesIndexes,
    StdVideoDecodeH264PictureInfoFlags currPicFlags,
    int32_t* pCurrAllocatedSlotIndex)
{
    // #### Update m_dpb based on dpb parameters ####
    // Create unordered DPB and generate a bitmask of all render targets present
    // in DPB
    uint32_t num_ref_frames = pd->CodecSpecific.h264.pStdSps->GetStdH264Sps()->max_num_ref_frames;
    assert(num_ref_frames <= HEVC_MAX_DPB_SLOTS);
    assert(num_ref_frames <= m_maxNumDpbSlots);
    dpbH264Entry refOnlyDpbIn[AVC_MAX_DPB_SLOTS]; // max number of Dpb
        // surfaces
    memset(&refOnlyDpbIn, 0, m_maxNumDpbSlots * sizeof(refOnlyDpbIn[0]));
    uint32_t refDpbUsedAndValidMask = 0;
    uint32_t numUsedRef = 0;
    for (int32_t inIdx = 0; (uint32_t)inIdx < maxDpbInSlotsInUse; inIdx++) {
        // used_for_reference: 0 = unused, 1 = top_field, 2 = bottom_field, 3 =
        // both_fields
        const uint32_t used_for_reference = dpbIn[inIdx].used_for_reference & fieldIsReferenceMask;
        if (used_for_reference) {
            int8_t picIdx = (!dpbIn[inIdx].not_existing && dpbIn[inIdx].pPicBuf)
                ? GetPicIdx(dpbIn[inIdx].pPicBuf)
                : -1;
            const bool isFieldRef = (picIdx >= 0) ? GetFieldPicFlag(picIdx)
                                                  : (used_for_reference && (used_for_reference != fieldIsReferenceMask));
            const int16_t fieldOrderCntList[2] = {
                (int16_t)dpbIn[inIdx].FieldOrderCnt[0],
                (int16_t)dpbIn[inIdx].FieldOrderCnt[1]
            };
            refOnlyDpbIn[numUsedRef].setReferenceAndTopBottomField(
                !!used_for_reference,
                (picIdx < 0), /* not_existing is frame inferred by the decoding
                           process for gaps in frame_num */
                !!dpbIn[inIdx].is_long_term, isFieldRef,
                !!(used_for_reference & topFieldMask),
                !!(used_for_reference & bottomFieldMask), dpbIn[inIdx].FrameIdx,
                fieldOrderCntList, GetPic(dpbIn[inIdx].pPicBuf));
            if (picIdx >= 0) {
                refDpbUsedAndValidMask |= (1 << picIdx);
            }
            numUsedRef++;
        }
        // Invalidate all slots.
        pReferenceSlots[inIdx].slotIndex = -1;
        pGopReferenceImagesIndexes[inIdx] = -1;
    }

    assert(numUsedRef <= HEVC_MAX_DPB_SLOTS);
    assert(numUsedRef <= m_maxNumDpbSlots);
    assert(numUsedRef <= num_ref_frames);

    if (m_dumpDpbData) {
        std::cout << " =>>> ********************* picIdx: "
                  << (int32_t)GetPicIdx(pd->pCurrPic)
                  << " *************************" << std::endl;
        std::cout << "\tRef frames data in for picIdx: "
                  << (int32_t)GetPicIdx(pd->pCurrPic) << std::endl
                  << "\tSlot Index:\t\t";
        for (uint32_t slot = 0; slot < numUsedRef; slot++) {
            if (!refOnlyDpbIn[slot].is_non_existing) {
                std::cout << slot << ",\t";
            } else {
                std::cout << 'X' << ",\t";
            }
        }
        std::cout << std::endl
                  << "\tPict Index:\t\t";
        for (uint32_t slot = 0; slot < numUsedRef; slot++) {
            if (!refOnlyDpbIn[slot].is_non_existing) {
                std::cout << refOnlyDpbIn[slot].m_picBuff->m_picIdx << ",\t";
            } else {
                std::cout << 'X' << ",\t";
            }
        }
        std::cout << "\n\tTotal Ref frames for picIdx: "
                  << (int32_t)GetPicIdx(pd->pCurrPic) << " : " << numUsedRef
                  << " out of " << num_ref_frames << " MAX(" << m_maxNumDpbSlots
                  << ")" << std::endl
                  << std::endl;

        std::cout << std::flush;
    }

    // Map all frames not present in DPB as non-reference, and generate a mask of
    // all used DPB entries
    /* uint32_t destUsedDpbMask = */ ResetPicDpbSlots(refDpbUsedAndValidMask);

    // Now, map DPB render target indices to internal frame buffer index,
    // assign each reference a unique DPB entry, and create the ordered DPB
    // This is an undocumented MV restriction: the position in the DPB is stored
    // along with the co-located data, so once a reference frame is assigned a DPB
    // entry, it can no longer change.

    // Find or allocate slots for existing dpb items.
    // Take into account the reference picture now.
    int8_t currPicIdx = GetPicIdx(pd->pCurrPic);
    assert(currPicIdx >= 0);
    int8_t bestNonExistingPicIdx = currPicIdx;
    if (refDpbUsedAndValidMask) {
        int32_t minFrameNumDiff = 0x10000;
        for (int32_t dpbIdx = 0; (uint32_t)dpbIdx < numUsedRef; dpbIdx++) {
            if (!refOnlyDpbIn[dpbIdx].is_non_existing) {
                vkPicBuffBase* picBuff = refOnlyDpbIn[dpbIdx].m_picBuff;
                int8_t picIdx = GetPicIdx(picBuff); // should always be valid at this point
                assert(picIdx >= 0);
                // We have up to 17 internal frame buffers, but only MAX_DPB_SIZE dpb
                // entries, so we need to re-map the index from the [0..MAX_DPB_SIZE]
                // range to [0..15]
                int8_t dpbSlot = GetPicDpbSlot(picIdx);
                if (dpbSlot < 0) {
                    dpbSlot = m_dpb.AllocateSlot();
                    assert((dpbSlot >= 0) && ((uint32_t)dpbSlot < m_maxNumDpbSlots));
                    SetPicDpbSlot(picIdx, dpbSlot);
                    m_dpb[dpbSlot].setPictureResource(picBuff, m_nCurrentPictureID);
                }
                m_dpb[dpbSlot].MarkInUse(m_nCurrentPictureID);
                assert(dpbSlot >= 0);

                if (dpbSlot >= 0) {
                    refOnlyDpbIn[dpbIdx].dpbSlot = dpbSlot;
                } else {
                    // This should never happen
                    printf("DPB mapping logic broken!\n");
                    assert(0);
                }

                int32_t frameNumDiff = ((int32_t)pd->CodecSpecific.h264.frame_num - refOnlyDpbIn[dpbIdx].FrameIdx);
                if (frameNumDiff <= 0) {
                    frameNumDiff = 0xffff;
                }
                if (frameNumDiff < minFrameNumDiff) {
                    bestNonExistingPicIdx = picIdx;
                    minFrameNumDiff = frameNumDiff;
                } else if (bestNonExistingPicIdx == currPicIdx) {
                    bestNonExistingPicIdx = picIdx;
                }
            }
        }
    }
    // In Vulkan, we always allocate a Dbp slot for the current picture,
    // regardless if it is going to become a reference or not. Non-reference slots
    // get freed right after usage. if (pd->ref_pic_flag) {
    int8_t currPicDpbSlot = AllocateDpbSlotForCurrentH264(GetPic(pd->pCurrPic),
            currPicFlags, pd->current_dpb_id);
    assert(currPicDpbSlot >= 0);
    *pCurrAllocatedSlotIndex = currPicDpbSlot;

    if (refDpbUsedAndValidMask) {
        // Find or allocate slots for non existing dpb items and populate the slots.
        uint32_t dpbInUseMask = m_dpb.getSlotInUseMask();
        int8_t firstNonExistingDpbSlot = 0;
        for (uint32_t dpbIdx = 0; dpbIdx < numUsedRef; dpbIdx++) {
            int8_t dpbSlot = -1;
            int8_t picIdx = -1;
            if (refOnlyDpbIn[dpbIdx].is_non_existing) {
                assert(refOnlyDpbIn[dpbIdx].m_picBuff == NULL);
                while (((uint32_t)firstNonExistingDpbSlot < m_maxNumDpbSlots) && (dpbSlot == -1)) {
                    if (!(dpbInUseMask & (1 << firstNonExistingDpbSlot))) {
                        dpbSlot = firstNonExistingDpbSlot;
                    }
                    firstNonExistingDpbSlot++;
                }
                assert((dpbSlot >= 0) && ((uint32_t)dpbSlot < m_maxNumDpbSlots));
                picIdx = bestNonExistingPicIdx;
                // Find the closest valid refpic already in the DPB
                uint32_t minDiffPOC = 0x7fff;
                for (uint32_t j = 0; j < numUsedRef; j++) {
                    if (!refOnlyDpbIn[j].is_non_existing && (refOnlyDpbIn[j].used_for_reference & refOnlyDpbIn[dpbIdx].used_for_reference) == refOnlyDpbIn[dpbIdx].used_for_reference) {
                        uint32_t diffPOC = abs((int32_t)(refOnlyDpbIn[j].FieldOrderCnt[0] - refOnlyDpbIn[dpbIdx].FieldOrderCnt[0]));
                        if (diffPOC <= minDiffPOC) {
                            minDiffPOC = diffPOC;
                            picIdx = GetPicIdx(refOnlyDpbIn[j].m_picBuff);
                        }
                    }
                }
            } else {
                assert(refOnlyDpbIn[dpbIdx].m_picBuff != NULL);
                dpbSlot = refOnlyDpbIn[dpbIdx].dpbSlot;
                picIdx = GetPicIdx(refOnlyDpbIn[dpbIdx].m_picBuff);
            }
            assert((dpbSlot >= 0) && ((uint32_t)dpbSlot < m_maxNumDpbSlots));
            refOnlyDpbIn[dpbIdx].setH264PictureData(pDpbRefList, pReferenceSlots,
                dpbIdx, dpbSlot, pd->progressive_frame);
            pGopReferenceImagesIndexes[dpbIdx] = picIdx;
        }
    }

    if (m_dumpDpbData) {
        uint32_t slotInUseMask = m_dpb.getSlotInUseMask();
        uint32_t slotsInUseCount = 0;
        std::cout << "\tAllocated Ref slot " << (int32_t)currPicDpbSlot << " for "
                  << (pd->ref_pic_flag ? "REFERENCE" : "NON-REFERENCE")
                  << " picIdx: " << (int32_t)currPicIdx << std::endl;
        std::cout << "\tRef frames map for picIdx: " << (int32_t)currPicIdx
                  << std::endl
                  << "\tSlot Index:\t\t";
        for (uint32_t slot = 0; slot < m_dpb.getMaxSize(); slot++) {
            if (slotInUseMask & (1 << slot)) {
                std::cout << slot << ",\t";
                slotsInUseCount++;
            } else {
                std::cout << 'X' << ",\t";
            }
        }
        std::cout << std::endl
                  << "\tPict Index:\t\t";
        for (uint32_t slot = 0; slot < m_dpb.getMaxSize(); slot++) {
            if (slotInUseMask & (1 << slot)) {
                if (m_dpb[slot].getPictureResource()) {
                    std::cout << m_dpb[slot].getPictureResource()->m_picIdx << ",\t";
                } else {
                    std::cout << "non existent"
                              << ",\t";
                }
            } else {
                std::cout << 'X' << ",\t";
            }
        }
        std::cout << "\n\tTotal slots in use for picIdx: " << (int32_t)currPicIdx
                  << " : " << slotsInUseCount << " out of " << m_dpb.getMaxSize()
                  << std::endl;
        std::cout << " <<<= ********************* picIdx: "
                  << (int32_t)GetPicIdx(pd->pCurrPic)
                  << " *************************" << std::endl
                  << std::endl;
        std::cout << std::flush;
    }
    return refDpbUsedAndValidMask ? numUsedRef : 0;
}

uint32_t VulkanVideoParser::FillDpbH265State(
    const VkParserPictureData* pd, const VkParserHevcPictureData* pin,
    nvVideoDecodeH265DpbSlotInfo* pDpbSlotInfo,
    StdVideoDecodeH265PictureInfo* pStdPictureInfo, uint32_t /*maxRefPictures*/,
    VkVideoReferenceSlotInfoKHR* pReferenceSlots,
    int8_t* pGopReferenceImagesIndexes,
    int32_t* pCurrAllocatedSlotIndex)
{
    // #### Update m_dpb based on dpb parameters ####
    // Create unordered DPB and generate a bitmask of all render targets present
    // in DPB
    dpbH264Entry refOnlyDpbIn[HEVC_MAX_DPB_SLOTS];
    assert(m_maxNumDpbSlots <= HEVC_MAX_DPB_SLOTS);
    memset(&refOnlyDpbIn, 0, m_maxNumDpbSlots * sizeof(refOnlyDpbIn[0]));
    uint32_t refDpbUsedAndValidMask = 0;
    uint32_t numUsedRef = 0;
    if (m_dumpParserData)
        std::cout << "Ref frames data: " << std::endl;
    for (int32_t inIdx = 0; inIdx < HEVC_MAX_DPB_SLOTS; inIdx++) {
        // used_for_reference: 0 = unused, 1 = top_field, 2 = bottom_field, 3 =
        // both_fields
        int8_t picIdx = GetPicIdx(pin->RefPics[inIdx]);
        if (picIdx >= 0) {
            assert(numUsedRef < HEVC_MAX_DPB_SLOTS);
            refOnlyDpbIn[numUsedRef].setReference((pin->IsLongTerm[inIdx] == 1),
                pin->PicOrderCntVal[inIdx],
                GetPic(pin->RefPics[inIdx]));
            if (picIdx >= 0) {
                refDpbUsedAndValidMask |= (1 << picIdx);
            }
            refOnlyDpbIn[numUsedRef].originalDpbIndex = inIdx;
            numUsedRef++;
        }
        // Invalidate all slots.
        pReferenceSlots[inIdx].slotIndex = -1;
        pGopReferenceImagesIndexes[inIdx] = -1;
    }

    if (m_dumpParserData)
        std::cout << "Total Ref frames: " << numUsedRef << std::endl;

    assert(numUsedRef <= m_maxNumDpbSlots);
    assert(numUsedRef <= HEVC_MAX_DPB_SLOTS);

    // Take into account the reference picture now.
    int8_t currPicIdx = GetPicIdx(pd->pCurrPic);
    assert(currPicIdx >= 0);
    if (currPicIdx >= 0) {
        refDpbUsedAndValidMask |= (1 << currPicIdx);
    }

    // Map all frames not present in DPB as non-reference, and generate a mask of
    // all used DPB entries
    /* uint32_t destUsedDpbMask = */ ResetPicDpbSlots(refDpbUsedAndValidMask);

    // Now, map DPB render target indices to internal frame buffer index,
    // assign each reference a unique DPB entry, and create the ordered DPB
    // This is an undocumented MV restriction: the position in the DPB is stored
    // along with the co-located data, so once a reference frame is assigned a DPB
    // entry, it can no longer change.

    int8_t frmListToDpb[HEVC_MAX_DPB_SLOTS];
    // TODO change to -1 for invalid indexes.
    memset(&frmListToDpb, 0, sizeof(frmListToDpb));
    // Find or allocate slots for existing dpb items.
    for (int32_t dpbIdx = 0; (uint32_t)dpbIdx < numUsedRef; dpbIdx++) {
        if (!refOnlyDpbIn[dpbIdx].is_non_existing) {
            vkPicBuffBase* picBuff = refOnlyDpbIn[dpbIdx].m_picBuff;
            int32_t picIdx = GetPicIdx(picBuff); // should always be valid at this point
            assert(picIdx >= 0);
            // We have up to 17 internal frame buffers, but only HEVC_MAX_DPB_SLOTS
            // dpb entries, so we need to re-map the index from the
            // [0..HEVC_MAX_DPB_SLOTS] range to [0..15]
            int8_t dpbSlot = GetPicDpbSlot(picIdx);
            if (dpbSlot < 0) {
                dpbSlot = m_dpb.AllocateSlot();
                assert(dpbSlot >= 0);
                SetPicDpbSlot(picIdx, dpbSlot);
                m_dpb[dpbSlot].setPictureResource(picBuff, m_nCurrentPictureID);
            }
            m_dpb[dpbSlot].MarkInUse(m_nCurrentPictureID);
            assert(dpbSlot >= 0);

            if (dpbSlot >= 0) {
                refOnlyDpbIn[dpbIdx].dpbSlot = dpbSlot;
                uint32_t originalDpbIndex = refOnlyDpbIn[dpbIdx].originalDpbIndex;
                assert(originalDpbIndex < HEVC_MAX_DPB_SLOTS);
                frmListToDpb[originalDpbIndex] = dpbSlot;
            } else {
                // This should never happen
                printf("DPB mapping logic broken!\n");
                assert(0);
            }
        }
    }

    // Find or allocate slots for non existing dpb items and populate the slots.
    uint32_t dpbInUseMask = m_dpb.getSlotInUseMask();
    int8_t firstNonExistingDpbSlot = 0;
    for (uint32_t dpbIdx = 0; dpbIdx < numUsedRef; dpbIdx++) {
        int8_t dpbSlot = -1;
        if (refOnlyDpbIn[dpbIdx].is_non_existing) {
            // There shouldn't be  not_existing in h.265
            assert(0);
            assert(refOnlyDpbIn[dpbIdx].m_picBuff == NULL);
            while (((uint32_t)firstNonExistingDpbSlot < m_maxNumDpbSlots) && (dpbSlot == -1)) {
                if (!(dpbInUseMask & (1 << firstNonExistingDpbSlot))) {
                    dpbSlot = firstNonExistingDpbSlot;
                }
                firstNonExistingDpbSlot++;
            }
            assert((dpbSlot >= 0) && ((uint32_t)dpbSlot < m_maxNumDpbSlots));
        } else {
            assert(refOnlyDpbIn[dpbIdx].m_picBuff != NULL);
            dpbSlot = refOnlyDpbIn[dpbIdx].dpbSlot;
        }
        assert((dpbSlot >= 0) && (dpbSlot < HEVC_MAX_DPB_SLOTS));
        refOnlyDpbIn[dpbIdx].setH265PictureData(pDpbSlotInfo, pReferenceSlots,
            dpbIdx, dpbSlot);
        pGopReferenceImagesIndexes[dpbIdx] = GetPicIdx(refOnlyDpbIn[dpbIdx].m_picBuff);
    }

    if (m_dumpParserData) {
        std::cout << "frmListToDpb:" << std::endl;
        for (int8_t dpbResIdx = 0; dpbResIdx < HEVC_MAX_DPB_SLOTS; dpbResIdx++) {
            std::cout << "\tfrmListToDpb[" << (int32_t)dpbResIdx << "] is "
                      << (int32_t)frmListToDpb[dpbResIdx] << std::endl;
        }
    }

    int32_t numPocTotalCurr = 0;
    int32_t numPocStCurrBefore = 0;
    const size_t maxNumPocStCurrBefore = ARRAYSIZE(pStdPictureInfo->RefPicSetStCurrBefore);
    assert((size_t)pin->NumPocStCurrBefore <= maxNumPocStCurrBefore);
    if ((size_t)pin->NumPocStCurrBefore > maxNumPocStCurrBefore) {
        fprintf(stderr, "\nERROR: FillDpbH265State() pin->NumPocStCurrBefore(%d) must be smaller than maxNumPocStCurrBefore(%zd)\n", pin->NumPocStCurrBefore, maxNumPocStCurrBefore);
    }
    for (int32_t i = 0; i < pin->NumPocStCurrBefore; i++) {
        uint8_t idx = (uint8_t)pin->RefPicSetStCurrBefore[i];
        if (idx < HEVC_MAX_DPB_SLOTS) {
            if (m_dumpParserData)
                std::cout << "\trefPicSetStCurrBefore[" << i << "] is " << (int32_t)idx
                          << " -> " << (int32_t)frmListToDpb[idx] << std::endl;
            pStdPictureInfo->RefPicSetStCurrBefore[numPocStCurrBefore++] = frmListToDpb[idx] & 0xf;
            numPocTotalCurr++;
        }
    }
    while (numPocStCurrBefore < 8) {
        pStdPictureInfo->RefPicSetStCurrBefore[numPocStCurrBefore++] = 0xff;
    }

    int32_t numPocStCurrAfter = 0;
    const size_t maxNumPocStCurrAfter = ARRAYSIZE(pStdPictureInfo->RefPicSetStCurrAfter);
    assert((size_t)pin->NumPocStCurrAfter <= maxNumPocStCurrAfter);
    if ((size_t)pin->NumPocStCurrAfter > maxNumPocStCurrAfter) {
        fprintf(stderr, "\nERROR: FillDpbH265State() pin->NumPocStCurrAfter(%d) must be smaller than maxNumPocStCurrAfter(%zd)\n", pin->NumPocStCurrAfter, maxNumPocStCurrAfter);
    }
    for (int32_t i = 0; i < pin->NumPocStCurrAfter; i++) {
        uint8_t idx = (uint8_t)pin->RefPicSetStCurrAfter[i];
        if (idx < HEVC_MAX_DPB_SLOTS) {
            if (m_dumpParserData)
                std::cout << "\trefPicSetStCurrAfter[" << i << "] is " << (int32_t)idx
                          << " -> " << (int32_t)frmListToDpb[idx] << std::endl;
            pStdPictureInfo->RefPicSetStCurrAfter[numPocStCurrAfter++] = frmListToDpb[idx] & 0xf;
            numPocTotalCurr++;
        }
    }
    while (numPocStCurrAfter < 8) {
        pStdPictureInfo->RefPicSetStCurrAfter[numPocStCurrAfter++] = 0xff;
    }

    int32_t numPocLtCurr = 0;
    const size_t maxNumPocLtCurr = ARRAYSIZE(pStdPictureInfo->RefPicSetLtCurr);
    assert((size_t)pin->NumPocLtCurr <= maxNumPocLtCurr);
    if ((size_t)pin->NumPocLtCurr > maxNumPocLtCurr) {
        fprintf(stderr, "\nERROR: FillDpbH265State() pin->NumPocLtCurr(%d) must be smaller than maxNumPocLtCurr(%zd)\n", pin->NumPocLtCurr, maxNumPocLtCurr);
    }
    for (int32_t i = 0; i < pin->NumPocLtCurr; i++) {
        uint8_t idx = (uint8_t)pin->RefPicSetLtCurr[i];
        if (idx < HEVC_MAX_DPB_SLOTS) {
            if (m_dumpParserData)
                std::cout << "\trefPicSetLtCurr[" << i << "] is " << (int32_t)idx
                          << " -> " << (int32_t)frmListToDpb[idx] << std::endl;
            pStdPictureInfo->RefPicSetLtCurr[numPocLtCurr++] = frmListToDpb[idx] & 0xf;
            numPocTotalCurr++;
        }
    }
    while (numPocLtCurr < 8) {
        pStdPictureInfo->RefPicSetLtCurr[numPocLtCurr++] = 0xff;
    }

    (void)numPocTotalCurr; // TODO: This variable is unused, remove or use.

    for (int32_t i = 0; i < 8; i++) {
        if (m_dumpParserData)
            std::cout << "\tlist indx " << i << ": "
                      << " refPicSetStCurrBefore: "
                      << (int32_t)pStdPictureInfo->RefPicSetStCurrBefore[i]
                      << " refPicSetStCurrAfter: "
                      << (int32_t)pStdPictureInfo->RefPicSetStCurrAfter[i]
                      << " refPicSetLtCurr: "
                      << (int32_t)pStdPictureInfo->RefPicSetLtCurr[i] << std::endl;
    }

    int8_t dpbSlot = AllocateDpbSlotForCurrentH265(GetPic(pd->pCurrPic),
        true /* isReference */, pd->current_dpb_id);
    *pCurrAllocatedSlotIndex = dpbSlot;
    assert(!(dpbSlot < 0));
    if (dpbSlot >= 0) {
        assert(pd->ref_pic_flag);
    }

    return numUsedRef;
}

uint32_t VulkanVideoParser::FillDpbAV1State(
        const VkParserPictureData* pd,
        VkParserAv1PictureData* pin,
        nvVideoDecodeAV1DpbSlotInfo* pDpbSlotInfo,
        StdVideoDecodeAV1PictureInfo*,
        uint32_t,
        VkVideoReferenceSlotInfoKHR* pReferenceSlots,
        int8_t* pGopReferenceImagesIndexes,
        int32_t* pCurrAllocatedSlotIndex)
{
    // TODO(charlie): This shouldn't really need to be + 1 (9), TBD.
    assert(m_maxNumDpbSlots <= STD_VIDEO_AV1_NUM_REF_FRAMES + 1);
    uint32_t refDpbUsedAndValidMask = 0;
    uint32_t referenceIndex = 0;

    if (m_dumpParserData)
        std::cout << "Ref frames data: " << std::endl;

    if (m_dumpDpbData) {
        printf(";;;; ======= AV1 DPB fill begin %d =======\n", m_nCurrentPictureID);
        printf("ref_frame_idx: ");
        for (int i = 0 ; i < 7; i++)
            printf("%02d ", i);
        printf("\nref_frame_idx: ");
        for (int i = 0 ; i < 7; i++)
            printf("%02d ", pin->ref_frame_idx[i]);
        printf("\n");

        printf("m_pictureToDpbSlotMap: ");
        for (int i = 0; i < MAX_FRM_CNT; i++) {
            printf("%02d ", i);
        }
        printf("\nm_pictureToDpbSlotMap: ");
        for (int i = 0; i < MAX_FRM_CNT; i++) {
            printf("%02d ", m_pictureToDpbSlotMap[i]);
        }
        printf("\n");

        printf("ref_frame_picture: ");
        for (int32_t inIdx = 0; inIdx < STD_VIDEO_AV1_NUM_REF_FRAMES; inIdx++) {
            printf("%02d ", inIdx);
        }
        printf("\nref_frame_picture: ");
        for (int32_t inIdx = 0; inIdx < STD_VIDEO_AV1_NUM_REF_FRAMES; inIdx++) {
            int8_t picIdx = pin->pic_idx[inIdx];
            printf("%02d ", picIdx);
        }
        printf("\n");
    }

    bool isKeyFrame = pin->std_info.frame_type == STD_VIDEO_AV1_FRAME_TYPE_KEY;

    // It doesn't look like this tracking is needed.
    int8_t activeReferences[32];
    memset(activeReferences, 0, sizeof(activeReferences));
    for (size_t refName = 0; refName < STD_VIDEO_AV1_REFS_PER_FRAME; refName++) {
        int8_t picIdx = isKeyFrame ? -1 : pin->pic_idx[pin->ref_frame_idx[refName]];
        if (picIdx < 0) {
            //pKhr->referenceNameSlotIndices[refName] = -1;
            continue;
        }
        int8_t dpbSlot = GetPicDpbSlot(picIdx);
        assert(dpbSlot >= 0);
        //pKhr->referenceNameSlotIndices[refName] = dpbSlot;
        activeReferences[dpbSlot]++;
        //hdr.delta_frame_id_minus_1[dpbSlot] = pin->delta_frame_id_minus_1[pin->ref_frame_idx[i]];
    }

    for (int32_t inIdx = 0; inIdx < STD_VIDEO_AV1_NUM_REF_FRAMES; inIdx++) {
        int8_t picIdx = isKeyFrame ? -1 : pin->pic_idx[inIdx];
        int8_t dpbSlot = -1;
        if ((picIdx >= 0) && !(refDpbUsedAndValidMask & (1 << picIdx))) {
            dpbSlot = GetPicDpbSlot(picIdx);

            assert(dpbSlot >= 0); // There is still content hitting this assert.
            if (dpbSlot < 0) {
                continue;
            }

            refDpbUsedAndValidMask |= (1 << picIdx);
            m_dpb[dpbSlot].MarkInUse(m_nCurrentPictureID);
            if (activeReferences[dpbSlot] == 0) {
                continue;
            }

            VkVideoDecodeAV1DpbSlotInfoKHR &dpbSlotInfo = pDpbSlotInfo[referenceIndex].dpbSlotInfo;
            dpbSlotInfo.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_AV1_DPB_SLOT_INFO_KHR;
            dpbSlotInfo.pStdReferenceInfo = &pin->dpbSlotInfos[inIdx];
            pReferenceSlots[referenceIndex].sType = VK_STRUCTURE_TYPE_VIDEO_REFERENCE_SLOT_INFO_KHR;
            pReferenceSlots[referenceIndex].pNext = &dpbSlotInfo;
            pReferenceSlots[referenceIndex].slotIndex = dpbSlot;
            pGopReferenceImagesIndexes[referenceIndex] = picIdx;

            referenceIndex++;
        }
    }

    if (m_dumpDpbData) {
        printf(";;; pReferenceSlots (%d): ", referenceIndex);
        for (size_t i =0 ;i < referenceIndex; i++) {
            printf("%02d ", pReferenceSlots[i].slotIndex);
        }
        printf("\n");
    }

    ResetPicDpbSlots(refDpbUsedAndValidMask);

    // Take into account the reference picture now.
    int8_t currPicIdx = GetPicIdx(pd->pCurrPic);
    assert(currPicIdx >= 0);
    if (currPicIdx >= 0) {
        refDpbUsedAndValidMask |= (1 << currPicIdx);
    }

    // NOTE(charlie): Most likely we can consider isReference = refresh_frame_flags != 0;
    // However, the AMD fw interface appears to always need a setup slot & a destination resource,
    // so it's not clear what to properly do in that case.
    int8_t dpbSlot = AllocateDpbSlotForCurrentAV1(GetPic(pd->pCurrPic),
        true /* isReference */, pd->current_dpb_id);

    assert(dpbSlot >= 0);

    *pCurrAllocatedSlotIndex = dpbSlot;
    assert(!(dpbSlot < 0));
    if (dpbSlot >= 0) {
        assert(pd->ref_pic_flag);
    }

    if (m_dumpDpbData) {
        printf("SlotsInUse: ");
        uint32_t slotsInUse = m_dpb.getSlotInUseMask();
        for (int i = 0; i < 9; i++) {
            printf("%02d ", i);
        }
        uint8_t greenSquare[] = { 0xf0, 0x9f,  0x9f, 0xa9, 0x00 };
        uint8_t redSquare[] = { 0xf0, 0x9f,  0x9f, 0xa5, 0x00 };
        uint8_t yellowSquare[] = { 0xf0, 0x9f,  0x9f, 0xa8, 0x00 };
        printf("\nSlotsInUse: ");
        for (int i = 0; i < 9; i++) {
            printf("%-2s ", (slotsInUse & (1<<i)) ? (i == dpbSlot ? (char*)yellowSquare : (char*)greenSquare) : (char*)redSquare); 
        }
        printf("\n");
    }

    return referenceIndex;
}

int8_t VulkanVideoParser::AllocateDpbSlotForCurrentH264(
    vkPicBuffBase* pPic, StdVideoDecodeH264PictureInfoFlags currPicFlags,
    int8_t /*presetDpbSlot*/)
{
    // Now, map the current render target
    int8_t dpbSlot = -1;
    int8_t currPicIdx = GetPicIdx(pPic);
    assert(currPicIdx >= 0);
    SetFieldPicFlag(currPicIdx, currPicFlags.field_pic_flag);
    // In Vulkan we always allocate reference slot for the current picture.
    if (true /* currPicFlags.is_reference */) {
        dpbSlot = GetPicDpbSlot(currPicIdx);
        if (dpbSlot < 0) {
            dpbSlot = m_dpb.AllocateSlot();
            assert(dpbSlot >= 0);
            SetPicDpbSlot(currPicIdx, dpbSlot);
            m_dpb[dpbSlot].setPictureResource(pPic, m_nCurrentPictureID);
        }
        assert(dpbSlot >= 0);
    }
    return dpbSlot;
}

int8_t VulkanVideoParser::AllocateDpbSlotForCurrentH265(vkPicBuffBase* pPic,
    bool isReference, int8_t /*presetDpbSlot*/)
{
    // Now, map the current render target
    int8_t dpbSlot = -1;
    int8_t currPicIdx = GetPicIdx(pPic);
    assert(currPicIdx >= 0);
    assert(isReference);
    if (isReference) {
        dpbSlot = GetPicDpbSlot(currPicIdx);
        if (dpbSlot < 0) {
            dpbSlot = m_dpb.AllocateSlot();
            assert(dpbSlot >= 0);
            SetPicDpbSlot(currPicIdx, dpbSlot);
            m_dpb[dpbSlot].setPictureResource(pPic, m_nCurrentPictureID);
        }
        assert(dpbSlot >= 0);
    }
    return dpbSlot;
}

int8_t VulkanVideoParser::AllocateDpbSlotForCurrentAV1(vkPicBuffBase* pPic,
    bool isReference, int8_t /*presetDpbSlot*/)
{
    int8_t dpbSlot = -1;
    int8_t currPicIdx = GetPicIdx(pPic);
    assert(currPicIdx >= 0);
    assert(isReference);
    if (isReference) {
        dpbSlot = GetPicDpbSlot(currPicIdx); // use the associated slot, if not allocate a new slot.
        if (dpbSlot < 0) {
            dpbSlot = m_dpb.AllocateSlot(); 
            assert(dpbSlot >= 0);
            SetPicDpbSlot(currPicIdx, dpbSlot); // Assign the dpbSlot to the current picture index.
            m_dpb[dpbSlot].setPictureResource(pPic, m_nCurrentPictureID); // m_nCurrentPictureID is our main index.
        }
        assert(dpbSlot >= 0);
    }
    return dpbSlot;
}

static const char* PictureParametersTypeToName(StdVideoPictureParametersSet::StdType updateType)
{
    switch (updateType)
    {
    case StdVideoPictureParametersSet::TYPE_H264_SPS:
        return "H264_SPS";
    case StdVideoPictureParametersSet::TYPE_H264_PPS:
        return "H264_PPS";
    case StdVideoPictureParametersSet::TYPE_H265_VPS:
        return "H265_VPS";
    case StdVideoPictureParametersSet::TYPE_H265_SPS:
        return "H265_SPS";
    case StdVideoPictureParametersSet::TYPE_H265_PPS:
        return "H265_PPS";
    case StdVideoPictureParametersSet::TYPE_AV1_SPS:
        return "AV1_SPS";
    }
    return "unknown";
}

bool VulkanVideoParser::UpdatePictureParameters(
        VkSharedBaseObj<StdVideoPictureParametersSet>& pictureParametersObject,
        VkSharedBaseObj<VkVideoRefCountBase>& client)
{
    if (false) {
        std::cout << "################################################# " << std::endl;
        std::cout << "Update Picture parameters "
                << PictureParametersTypeToName(pictureParametersObject->GetStdType()) << ": "
                << pictureParametersObject.Get()
                << ", count: " << (uint32_t)pictureParametersObject->GetUpdateSequenceCount()
                << std::endl << std::flush;
        std::cout << "################################################# " << std::endl;
    }

    if (m_decoderHandler == NULL) {
        assert(!"m_pDecoderHandler is NULL");
        return false;
    }

    if (pictureParametersObject) {
        return m_decoderHandler->UpdatePictureParameters(pictureParametersObject, client);
    }

    return false;
}

bool VulkanVideoParser::DecodePicture(
    VkParserPictureData* pd, vkPicBuffBase* /*pVkPicBuff*/,
    VkParserDecodePictureInfo* pDecodePictureInfo)
{
    bool bRet = false;

    // union {
    nvVideoH264PicParameters h264;
    nvVideoH265PicParameters hevc;
    nvVideoAV1PicParameters av1;
    // };

    if (m_decoderHandler == NULL) {
        assert(!"m_pDecoderHandler is NULL");
        return false;
    }

    if (!pd->pCurrPic) {
        return false;
    }

    const uint32_t PicIdx = GetPicIdx(pd->pCurrPic);
    if (PicIdx >= MAX_FRM_CNT) {
        assert(0);
        return false;
    }

    VkParserPerFrameDecodeParameters pictureParams = VkParserPerFrameDecodeParameters();
    VkParserPerFrameDecodeParameters* pCurrFrameDecParams = &pictureParams;
    pCurrFrameDecParams->currPicIdx = PicIdx;
    pCurrFrameDecParams->numSlices = pd->numSlices;
    pCurrFrameDecParams->firstSliceIndex = pd->firstSliceIndex;
    pCurrFrameDecParams->bitstreamDataOffset = pd->bitstreamDataOffset;
    pCurrFrameDecParams->bitstreamDataLen = pd->bitstreamDataLen;
    pCurrFrameDecParams->bitstreamData = pd->bitstreamData;

    VkVideoReferenceSlotInfoKHR
        referenceSlots[VkParserPerFrameDecodeParameters::MAX_DPB_REF_AND_SETUP_SLOTS];
    VkVideoReferenceSlotInfoKHR setupReferenceSlot = {
        VK_STRUCTURE_TYPE_VIDEO_REFERENCE_SLOT_INFO_KHR, NULL,
        -1, // slotIndex
        NULL // pPictureResource
    };

    pCurrFrameDecParams->decodeFrameInfo.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_INFO_KHR;
    pCurrFrameDecParams->decodeFrameInfo.dstPictureResource.sType = VK_STRUCTURE_TYPE_VIDEO_PICTURE_RESOURCE_INFO_KHR;
    pCurrFrameDecParams->dpbSetupPictureResource.sType = VK_STRUCTURE_TYPE_VIDEO_PICTURE_RESOURCE_INFO_KHR;

    if (m_codecType == VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR) {
        const VkParserH264PictureData* const pin = &pd->CodecSpecific.h264;

        h264 = nvVideoH264PicParameters();

        nvVideoH264PicParameters* const pout = &h264;
        VkVideoDecodeH264PictureInfoKHR* pPictureInfo = &h264.pictureInfo;
        nvVideoDecodeH264DpbSlotInfo* pDpbRefList = h264.dpbRefList;
        StdVideoDecodeH264PictureInfo* pStdPictureInfo = &h264.stdPictureInfo;

        pCurrFrameDecParams->pStdPps = pin->pStdPps;
        pCurrFrameDecParams->pStdSps = pin->pStdSps;
        pCurrFrameDecParams->pStdVps = nullptr;
        if (false) {
            std::cout << "\n\tCurrent h.264 Picture SPS update : "
                    << pin->pStdSps->GetUpdateSequenceCount() << std::endl;
            std::cout << "\tCurrent h.264 Picture PPS update : "
                    << pin->pStdPps->GetUpdateSequenceCount() << std::endl;
        }

        //pDecodePictureInfo->videoFrameType = 0; // pd->CodecSpecific.h264.slice_type;
        // FIXME: If mvcext is enabled.
        pDecodePictureInfo->viewId = pd->CodecSpecific.h264.mvcext.view_id;

        pPictureInfo->pStdPictureInfo = &h264.stdPictureInfo;

        pPictureInfo->sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PICTURE_INFO_KHR;

        if (!m_outOfBandPictureParameters) {
            // In-band h264 Picture Parameters for testing
            h264.pictureParameters.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_SESSION_PARAMETERS_ADD_INFO_KHR;
            h264.pictureParameters.stdSPSCount = 1;
            h264.pictureParameters.pStdSPSs = pin->pStdSps->GetStdH264Sps();
            h264.pictureParameters.stdPPSCount = 1;
            h264.pictureParameters.pStdPPSs = pin->pStdPps->GetStdH264Pps();
            if (m_inlinedPictureParametersUseBeginCoding) {
                pCurrFrameDecParams->beginCodingInfoPictureParametersExt = &h264.pictureParameters;
                pPictureInfo->pNext = nullptr;
            } else {
                pPictureInfo->pNext = &h264.pictureParameters;
            }
            pCurrFrameDecParams->useInlinedPictureParameters = true;
        } else {
            pPictureInfo->pNext = nullptr;
        }

        pCurrFrameDecParams->decodeFrameInfo.pNext = &h264.pictureInfo;

        pStdPictureInfo->pic_parameter_set_id = pin->pic_parameter_set_id; // PPS ID
        pStdPictureInfo->seq_parameter_set_id = pin->seq_parameter_set_id; // SPS ID;

        pStdPictureInfo->frame_num = (uint16_t)pin->frame_num;
        pPictureInfo->sliceCount = pd->numSlices;
        uint32_t maxSliceCount = 0;
        assert(pd->firstSliceIndex == 0); // No slice and MV modes are supported yet
        pPictureInfo->pSliceOffsets = pd->bitstreamData->GetStreamMarkersPtr(pd->firstSliceIndex, maxSliceCount);
        assert(maxSliceCount == pd->numSlices);

        StdVideoDecodeH264PictureInfoFlags currPicFlags = StdVideoDecodeH264PictureInfoFlags();
        currPicFlags.is_intra = (pd->intra_pic_flag != 0);
        // 0 = frame picture, 1 = field picture
        if (pd->field_pic_flag) {
            // 0 = top field, 1 = bottom field (ignored if field_pic_flag = 0)
            currPicFlags.field_pic_flag = true;
            if (pd->bottom_field_flag) {
                currPicFlags.bottom_field_flag = true;
            }
        }
        // Second field of a complementary field pair
        if (pd->second_field) {
            currPicFlags.complementary_field_pair = true;
        }
        // Frame is a reference frame
        if (pd->ref_pic_flag) {
            currPicFlags.is_reference = true;
        }
        pStdPictureInfo->flags = currPicFlags;
        if (!pd->field_pic_flag) {
            pStdPictureInfo->PicOrderCnt[0] = pin->CurrFieldOrderCnt[0];
            pStdPictureInfo->PicOrderCnt[1] = pin->CurrFieldOrderCnt[1];
        } else {
            pStdPictureInfo->PicOrderCnt[pd->bottom_field_flag] = pin->CurrFieldOrderCnt[pd->bottom_field_flag];
        }

        const uint32_t maxDpbInputSlots = ARRAYSIZE(pin->dpb);
        pCurrFrameDecParams->numGopReferenceSlots = FillDpbH264State(
            pd, pin->dpb, maxDpbInputSlots, pDpbRefList,
            VkParserPerFrameDecodeParameters::MAX_DPB_REF_SLOTS, // 16 reference pictures
            referenceSlots, pCurrFrameDecParams->pGopReferenceImagesIndexes,
            h264.stdPictureInfo.flags, &setupReferenceSlot.slotIndex);
        // TODO: Remove it is for debugging only. Reserved fields must be set to "0".
        pout->stdPictureInfo.reserved1 = pCurrFrameDecParams->numGopReferenceSlots;
        assert(!pd->ref_pic_flag || (setupReferenceSlot.slotIndex >= 0));
        if (setupReferenceSlot.slotIndex >= 0) {
            setupReferenceSlot.pPictureResource = &pCurrFrameDecParams->dpbSetupPictureResource;
            pCurrFrameDecParams->decodeFrameInfo.pSetupReferenceSlot = &setupReferenceSlot;
        }
        if (pCurrFrameDecParams->numGopReferenceSlots) {
            assert(pCurrFrameDecParams->numGopReferenceSlots <= (int32_t)MAX_DPB_REF_SLOTS);
            for (uint32_t dpbEntryIdx = 0; dpbEntryIdx < (uint32_t)pCurrFrameDecParams->numGopReferenceSlots;
                 dpbEntryIdx++) {
                pCurrFrameDecParams->pictureResources[dpbEntryIdx].sType = VK_STRUCTURE_TYPE_VIDEO_PICTURE_RESOURCE_INFO_KHR;
                referenceSlots[dpbEntryIdx].pPictureResource = &pCurrFrameDecParams->pictureResources[dpbEntryIdx];
                assert(pDpbRefList[dpbEntryIdx].IsReference());
            }

            pCurrFrameDecParams->decodeFrameInfo.pReferenceSlots = referenceSlots;
            pCurrFrameDecParams->decodeFrameInfo.referenceSlotCount = pCurrFrameDecParams->numGopReferenceSlots;
        } else {
            pCurrFrameDecParams->decodeFrameInfo.pReferenceSlots = NULL;
            pCurrFrameDecParams->decodeFrameInfo.referenceSlotCount = 0;
        }

    }
    else if (m_codecType == VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR) {
        const VkParserHevcPictureData* const pin = &pd->CodecSpecific.hevc;
        hevc = nvVideoH265PicParameters();
        VkVideoDecodeH265PictureInfoKHR* pPictureInfo = &hevc.pictureInfo;
        StdVideoDecodeH265PictureInfo* pStdPictureInfo = &hevc.stdPictureInfo;
        nvVideoDecodeH265DpbSlotInfo* pDpbRefList = hevc.dpbRefList;

        pCurrFrameDecParams->pStdPps = pin->pStdPps;
        pCurrFrameDecParams->pStdSps = pin->pStdSps;
        pCurrFrameDecParams->pStdVps = pin->pStdVps;
        if (false) {
            std::cout << "\n\tCurrent h.265 Picture VPS update : "
                    << pin->pStdVps->GetUpdateSequenceCount() << std::endl;
            std::cout << "\n\tCurrent h.265 Picture SPS update : "
                    << pin->pStdSps->GetUpdateSequenceCount() << std::endl;
            std::cout << "\tCurrent h.265 Picture PPS update : "
                    << pin->pStdPps->GetUpdateSequenceCount() << std::endl;
        }

        pPictureInfo->sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_PICTURE_INFO_KHR;

        if (!m_outOfBandPictureParameters) {
            // In-band h265 Picture Parameters for testing
            hevc.pictureParameters.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_SESSION_PARAMETERS_ADD_INFO_KHR;
            hevc.pictureParameters.stdVPSCount = 1;
            hevc.pictureParameters.pStdVPSs = pin->pStdVps->GetStdH265Vps();
            hevc.pictureParameters.stdSPSCount = 1;
            hevc.pictureParameters.pStdSPSs = pin->pStdSps->GetStdH265Sps();
            hevc.pictureParameters.stdPPSCount = 1;
            hevc.pictureParameters.pStdPPSs = pin->pStdPps->GetStdH265Pps();
            if (m_inlinedPictureParametersUseBeginCoding) {
                pCurrFrameDecParams->beginCodingInfoPictureParametersExt = &hevc.pictureParameters;
                pPictureInfo->pNext = nullptr;
            } else {
                pPictureInfo->pNext = &hevc.pictureParameters;
            }
            pCurrFrameDecParams->useInlinedPictureParameters = true;
        } else {
            pPictureInfo->pNext = nullptr;
        }

        pPictureInfo->pStdPictureInfo = &hevc.stdPictureInfo;
        pCurrFrameDecParams->decodeFrameInfo.pNext = &hevc.pictureInfo;

        //pDecodePictureInfo->videoFrameType = 0; // pd->CodecSpecific.hevc.SliceType;
        if (pd->CodecSpecific.hevc.mv_hevc_enable) {
            pDecodePictureInfo->viewId = pd->CodecSpecific.hevc.nuh_layer_id;
        } else {
            pDecodePictureInfo->viewId = 0;
        }

        pPictureInfo->sliceSegmentCount = pd->numSlices;
        uint32_t maxSliceCount = 0;
        assert(pd->firstSliceIndex == 0); // No slice and MV modes are supported yet
        pPictureInfo->pSliceSegmentOffsets = pd->bitstreamData->GetStreamMarkersPtr(pd->firstSliceIndex, maxSliceCount);
        assert(maxSliceCount == pd->numSlices);

        pStdPictureInfo->pps_pic_parameter_set_id   = pin->pic_parameter_set_id;       // PPS ID
        pStdPictureInfo->pps_seq_parameter_set_id   = pin->seq_parameter_set_id;       // SPS ID
        pStdPictureInfo->sps_video_parameter_set_id = pin->vps_video_parameter_set_id; // VPS ID

        // hevc->irapPicFlag = m_slh.nal_unit_type >= NUT_BLA_W_LP &&
        // m_slh.nal_unit_type <= NUT_CRA_NUT;
        pStdPictureInfo->flags.IrapPicFlag = pin->IrapPicFlag; // Intra Random Access Point for current picture.
        // hevc->idrPicFlag = m_slh.nal_unit_type == NUT_IDR_W_RADL ||
        // m_slh.nal_unit_type == NUT_IDR_N_LP;
        pStdPictureInfo->flags.IdrPicFlag = pin->IdrPicFlag; // Instantaneous Decoding Refresh for current picture.

        // NumBitsForShortTermRPSInSlice = s->sh.short_term_rps ?
        // s->sh.short_term_ref_pic_set_size : 0
        pStdPictureInfo->NumBitsForSTRefPicSetInSlice = pin->NumBitsForShortTermRPSInSlice;

        // NumDeltaPocsOfRefRpsIdx = s->sh.short_term_rps ?
        // s->sh.short_term_rps->rps_idx_num_delta_pocs : 0
        pStdPictureInfo->NumDeltaPocsOfRefRpsIdx = pin->NumDeltaPocsOfRefRpsIdx;
        pStdPictureInfo->PicOrderCntVal = pin->CurrPicOrderCntVal;

        if (m_dumpParserData)
            std::cout << "\tnumPocStCurrBefore: " << (int32_t)pin->NumPocStCurrBefore
                      << " numPocStCurrAfter: " << (int32_t)pin->NumPocStCurrAfter
                      << " numPocLtCurr: " << (int32_t)pin->NumPocLtCurr << std::endl;

        pCurrFrameDecParams->numGopReferenceSlots = FillDpbH265State(pd, pin, pDpbRefList, pStdPictureInfo,
                VkParserPerFrameDecodeParameters::MAX_DPB_REF_SLOTS, // max 16 reference pictures
            referenceSlots, pCurrFrameDecParams->pGopReferenceImagesIndexes,
            &setupReferenceSlot.slotIndex);

        assert(!pd->ref_pic_flag || (setupReferenceSlot.slotIndex >= 0));
        if (setupReferenceSlot.slotIndex >= 0) {
            setupReferenceSlot.pPictureResource = &pCurrFrameDecParams->dpbSetupPictureResource;
            pCurrFrameDecParams->decodeFrameInfo.pSetupReferenceSlot = &setupReferenceSlot;
        }

        if (pCurrFrameDecParams->numGopReferenceSlots) {
            assert(pCurrFrameDecParams->numGopReferenceSlots <= (int32_t)MAX_DPB_REF_SLOTS);
            for (uint32_t dpbEntryIdx = 0; dpbEntryIdx < (uint32_t)pCurrFrameDecParams->numGopReferenceSlots;
                 dpbEntryIdx++) {
                pCurrFrameDecParams->pictureResources[dpbEntryIdx].sType = VK_STRUCTURE_TYPE_VIDEO_PICTURE_RESOURCE_INFO_KHR;
                referenceSlots[dpbEntryIdx].pPictureResource = &pCurrFrameDecParams->pictureResources[dpbEntryIdx];
                assert(pDpbRefList[dpbEntryIdx].IsReference());
            }

            pCurrFrameDecParams->decodeFrameInfo.pReferenceSlots = referenceSlots;
            pCurrFrameDecParams->decodeFrameInfo.referenceSlotCount = pCurrFrameDecParams->numGopReferenceSlots;
        } else {
            pCurrFrameDecParams->decodeFrameInfo.pReferenceSlots = NULL;
            pCurrFrameDecParams->decodeFrameInfo.referenceSlotCount = 0;
        }

        if (m_dumpParserData) {
            for (int32_t i = 0; i < HEVC_MAX_DPB_SLOTS; i++) {
                std::cout << "\tdpbIndex: " << i;
                if (pDpbRefList[i]) {
                    std::cout << " REFERENCE FRAME";

                    std::cout << " picOrderCntValList: "
                              << (int32_t)pDpbRefList[i]
                                     .dpbSlotInfo.pStdReferenceInfo->PicOrderCntVal;

                    std::cout << "\t\t Flags: ";
                    if (pDpbRefList[i]
                            .dpbSlotInfo.pStdReferenceInfo->flags.used_for_long_term_reference) {
                        std::cout << "IS LONG TERM ";
                    }

                } else {
                    std::cout << " NOT A REFERENCE ";
                }
                std::cout << std::endl;
            }
        }

    }
    else if (m_codecType == VK_VIDEO_CODEC_OPERATION_DECODE_AV1_BIT_KHR) {
        VkParserAv1PictureData* pin = &pd->CodecSpecific.av1;

        av1 = nvVideoAV1PicParameters();
        VkVideoDecodeAV1PictureInfoKHR* pPictureInfo = &av1.pictureInfo;
        av1.pictureInfo.pStdPictureInfo = &av1.stdPictureInfo;
        StdVideoDecodeAV1PictureInfo *pStdPictureInfo = &av1.stdPictureInfo;

        pCurrFrameDecParams->pStdPps = nullptr;
        pCurrFrameDecParams->pStdSps = pin->pStdSps;
        pCurrFrameDecParams->pStdVps = nullptr;

        if (false) {
            std::cout << "\n\tCurrent AV1 Picture SPS update : "
                      << pin->pStdSps->GetUpdateSequenceCount() << std::endl;
        }

        nvVideoDecodeAV1DpbSlotInfo* dpbSlotsAv1 = av1.dpbRefList;
        pCurrFrameDecParams->numGopReferenceSlots = 
            FillDpbAV1State(pd,
                            pin,
                            dpbSlotsAv1,
                            pStdPictureInfo,
                            9,
                            referenceSlots,
                            pCurrFrameDecParams->pGopReferenceImagesIndexes,
                            &setupReferenceSlot.slotIndex);

        assert(!pd->ref_pic_flag || (setupReferenceSlot.slotIndex >= 0));
        if (setupReferenceSlot.slotIndex >= 0) {
            setupReferenceSlot.pPictureResource = &pCurrFrameDecParams->dpbSetupPictureResource;
            pCurrFrameDecParams->decodeFrameInfo.pSetupReferenceSlot = &setupReferenceSlot;
        }

        if (pCurrFrameDecParams->numGopReferenceSlots) {
            assert(pCurrFrameDecParams->numGopReferenceSlots <= (int32_t)MAX_DPB_REF_SLOTS);
            for (uint32_t dpbEntryIdx = 0; dpbEntryIdx < (uint32_t)pCurrFrameDecParams->numGopReferenceSlots;
                 dpbEntryIdx++) {
                pCurrFrameDecParams->pictureResources[dpbEntryIdx].sType = VK_STRUCTURE_TYPE_VIDEO_PICTURE_RESOURCE_INFO_KHR;
                referenceSlots[dpbEntryIdx].pPictureResource = &pCurrFrameDecParams->pictureResources[dpbEntryIdx];
            }

            pCurrFrameDecParams->decodeFrameInfo.pReferenceSlots = referenceSlots;
            pCurrFrameDecParams->decodeFrameInfo.referenceSlotCount = pCurrFrameDecParams->numGopReferenceSlots;
        } else {
            pCurrFrameDecParams->decodeFrameInfo.pReferenceSlots = NULL;
            pCurrFrameDecParams->decodeFrameInfo.referenceSlotCount = 0;
        }

        // @review: this field seems only useful for debug display, but since AV1 needs a dword, should probably change the interface.
        //pDecodePictureInfo->videoFrameType = static_cast<uint16_t>(pin->frame_type);
        pDecodePictureInfo->viewId = 0; // @review: Doesn't seem to be used in Vulkan?

        pPictureInfo->sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_AV1_PICTURE_INFO_KHR;
        pCurrFrameDecParams->decodeFrameInfo.pNext = &av1.pictureInfo;

        bool isKeyFrame = pin->std_info.frame_type == STD_VIDEO_AV1_FRAME_TYPE_KEY;
        for (size_t i = 0; i < STD_VIDEO_AV1_REFS_PER_FRAME; i++) {
            int8_t picIdx = isKeyFrame ? -1 : pin->pic_idx[pin->ref_frame_idx[i]];
            if (picIdx < 0) {
                pPictureInfo->referenceNameSlotIndices[i] = -1;
                continue;
            }

            int8_t dpbSlot = GetPicDpbSlot(picIdx);
            assert(dpbSlot >= 0);
            pPictureInfo->referenceNameSlotIndices[i] = dpbSlot;
        }

        pPictureInfo->pTileOffsets = pin->tileOffsets;
        pPictureInfo->pTileSizes = pin->tileSizes;
        pPictureInfo->tileCount = pin->khr_info.tileCount;
        StdVideoDecodeAV1PictureInfo& hdr = av1.stdPictureInfo;

        memcpy(&hdr, &pin->std_info, sizeof(hdr));
        hdr.pTileInfo = &pin->tileInfo;
        hdr.pQuantization = &pin->quantization;
        hdr.pSegmentation = &pin->segmentation;
        hdr.pLoopFilter = &pin->loopFilter;
        hdr.pCDEF = &pin->CDEF;
        hdr.pLoopRestoration = &pin->loopRestoration;
        hdr.pGlobalMotion = &pin->globalMotion;
        hdr.pFilmGrain = &pin->filmGrain;

        pin->tileInfo.pWidthInSbsMinus1 = pin->width_in_sbs_minus_1;
        pin->tileInfo.pHeightInSbsMinus1 = pin->height_in_sbs_minus_1;
        pin->tileInfo.pMiColStarts = pin->MiColStarts;
        pin->tileInfo.pMiRowStarts = pin->MiRowStarts;

        pDecodePictureInfo->flags.applyFilmGrain = pin->std_info.flags.apply_grain;
    }

    pDecodePictureInfo->displayWidth  = m_nvsi.nDisplayWidth;
    pDecodePictureInfo->displayHeight = m_nvsi.nDisplayHeight;

    bRet = (m_decoderHandler->DecodePictureWithParameters(pCurrFrameDecParams, pDecodePictureInfo) >= 0);

    if (m_dumpParserData) {
        std::cout << "\t <== VulkanVideoParser::DecodePicture " << PicIdx << std::endl;
    }
    m_nCurrentPictureID++;
    return bRet;
}
} //namespace NvVulkanDecoder

VkResult IVulkanVideoParser::Create(
    VkSharedBaseObj<IVulkanVideoDecoderHandler>& decoderHandler,
    VkSharedBaseObj<IVulkanVideoFrameBufferParserCb>& videoFrameBufferCb,
    VkVideoCodecOperationFlagBitsKHR codecType,
    uint32_t maxNumDecodeSurfaces,
    uint32_t maxNumDpbSurfaces,
    uint32_t defaultMinBufferSize,
    uint32_t bufferOffsetAlignment,
    uint32_t bufferSizeAlignment,
    uint64_t clockRate,
    uint32_t errorThreshold,
    VkSharedBaseObj<IVulkanVideoParser>& vulkanVideoParser)
{
    if (!decoderHandler || !videoFrameBufferCb) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    VkSharedBaseObj<NvVulkanDecoder::VulkanVideoParser> nvVulkanVideoParser(
            new NvVulkanDecoder::VulkanVideoParser(codecType,
                                                   maxNumDecodeSurfaces,
                                                   maxNumDpbSurfaces,
                                                   clockRate));

    if (nvVulkanVideoParser) {
        const bool outOfBandPictureParameters = true;
        VkResult result = nvVulkanVideoParser->Initialize(decoderHandler,
                                                          videoFrameBufferCb,
                                                          defaultMinBufferSize,
                                                          bufferOffsetAlignment,
                                                          bufferSizeAlignment,
                                                          outOfBandPictureParameters,
                                                          errorThreshold);

        if (result != VK_SUCCESS) {
            return result;
        }

        vulkanVideoParser = nvVulkanVideoParser;

        return VK_SUCCESS;
    }

    return VK_ERROR_OUT_OF_HOST_MEMORY;
}

VkResult vulkanCreateVideoParser(
            VkSharedBaseObj<IVulkanVideoDecoderHandler>& decoderHandler,
            VkSharedBaseObj<IVulkanVideoFrameBufferParserCb>& videoFrameBufferCb,
            VkVideoCodecOperationFlagBitsKHR videoCodecOperation,
            const VkExtensionProperties* pStdExtensionVersion,
            uint32_t maxNumDecodeSurfaces,
            uint32_t maxNumDpbSurfaces,
            uint32_t defaultMinBufferSize,
            uint32_t bufferOffsetAlignment,
            uint32_t bufferSizeAlignment,
            uint64_t clockRate,
            VkSharedBaseObj<IVulkanVideoParser>& vulkanVideoParser)
{
    if (videoCodecOperation == VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR) {
        if (!pStdExtensionVersion || strcmp(pStdExtensionVersion->extensionName, VK_STD_VULKAN_VIDEO_CODEC_H264_DECODE_EXTENSION_NAME) || (pStdExtensionVersion->specVersion != VK_STD_VULKAN_VIDEO_CODEC_H264_DECODE_SPEC_VERSION)) {
            assert(!"Decoder h264 Codec version is NOT supported");
            return VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR;
        }
    } else if (videoCodecOperation == VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR) {
        if (!pStdExtensionVersion || strcmp(pStdExtensionVersion->extensionName, VK_STD_VULKAN_VIDEO_CODEC_H265_DECODE_EXTENSION_NAME) || (pStdExtensionVersion->specVersion != VK_STD_VULKAN_VIDEO_CODEC_H265_DECODE_SPEC_VERSION)) {
            assert(!"Decoder h265 Codec version is NOT supported");
            return VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR;
        }
    } else if (videoCodecOperation == VK_VIDEO_CODEC_OPERATION_DECODE_AV1_BIT_KHR) {
        if (!pStdExtensionVersion || strcmp(pStdExtensionVersion->extensionName, VK_STD_VULKAN_VIDEO_CODEC_AV1_DECODE_EXTENSION_NAME) || (pStdExtensionVersion->specVersion != VK_STD_VULKAN_VIDEO_CODEC_AV1_DECODE_SPEC_VERSION)) {
            assert(!"Decoder AV1 Codec version is NOT supported");
            return VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR;
        }
    } else {
        assert(!"Decoder Codec is NOT supported");
        return VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR;
    }

    return IVulkanVideoParser::Create(decoderHandler, videoFrameBufferCb,
                                      videoCodecOperation,
                                      maxNumDecodeSurfaces,
                                      maxNumDpbSurfaces,
                                      defaultMinBufferSize,
                                      bufferOffsetAlignment,
                                      bufferSizeAlignment,
                                      clockRate,
                                      0, // errorThreshold
                                      vulkanVideoParser);
}
