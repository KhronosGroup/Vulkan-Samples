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

#ifndef _VKVIDEODECODER_VKPARSERVIDEOPICTUREPARAMETERS_H_
#define _VKVIDEODECODER_VKPARSERVIDEOPICTUREPARAMETERS_H_

#include <bitset>
#include <assert.h>
#include <atomic>
#include <queue>
#include <unordered_map>
#include "VkCodecUtils/VkVideoRefCountBase.h"
#include "VkCodecUtils/VulkanDeviceContext.h"
#include "vkvideo_parser/StdVideoPictureParametersSet.h"
#include "VkVideoCore/VkVideoCoreProfile.h"
#include "VkCodecUtils/VulkanVideoSession.h"

class VkParserVideoPictureParameters : public VkVideoRefCountBase {
public:
    static const uint32_t MAX_VPS_IDS =  16;
    static const uint32_t MAX_SPS_IDS =  32;
    static const uint32_t MAX_PPS_IDS = 256;

    //! Increment the reference count by 1.
    virtual int32_t AddRef();

    //! Decrement the reference count by 1. When the reference count
    //! goes to 0 the object is automatically destroyed.
    virtual int32_t Release();

    static VkParserVideoPictureParameters* VideoPictureParametersFromBase(VkVideoRefCountBase* pBase ) {
        if (!pBase) {
            return NULL;
        }
        VkParserVideoPictureParameters* pPictureParameters = static_cast<VkParserVideoPictureParameters*>(pBase);
        if (m_refClassId == pPictureParameters->m_classId) {
            return pPictureParameters;
        }
        assert(!"Invalid VkParserVideoPictureParameters from base");
        return nullptr;
    }

    static VkResult AddPictureParameters(const VulkanDeviceContext* vkDevCtx,
                                         VkSharedBaseObj<VulkanVideoSession>& videoSession,
                                         VkSharedBaseObj<StdVideoPictureParametersSet>& stdPictureParametersSet,
                                         VkSharedBaseObj<VkParserVideoPictureParameters>& currentVideoPictureParameters);

    static bool CheckStdObjectBeforeUpdate(VkSharedBaseObj<StdVideoPictureParametersSet>& pictureParametersSet,
                                           VkSharedBaseObj<VkParserVideoPictureParameters>& currentVideoPictureParameters);

    static VkResult Create(const VulkanDeviceContext* vkDevCtx,
                           VkSharedBaseObj<VkParserVideoPictureParameters>& templatePictureParameters,
                           VkSharedBaseObj<VkParserVideoPictureParameters>& videoPictureParameters);

    static int32_t PopulateH264UpdateFields(const StdVideoPictureParametersSet* pStdPictureParametersSet,
                                            VkVideoDecodeH264SessionParametersAddInfoKHR& h264SessionParametersAddInfo);

    static int32_t PopulateH265UpdateFields(const StdVideoPictureParametersSet* pStdPictureParametersSet,
                                            VkVideoDecodeH265SessionParametersAddInfoKHR& h265SessionParametersAddInfo);

    VkResult CreateParametersObject(const VulkanDeviceContext* vkDevCtx,
                                    VkSharedBaseObj<VulkanVideoSession>& videoSession,
                                    const StdVideoPictureParametersSet* pStdVideoPictureParametersSet,
                                    VkParserVideoPictureParameters* pTemplatePictureParameters);

    VkResult UpdateParametersObject(const StdVideoPictureParametersSet* pStdVideoPictureParametersSet);

    VkResult HandleNewPictureParametersSet(VkSharedBaseObj<VulkanVideoSession>& videoSession,
                                           const StdVideoPictureParametersSet* pStdVideoPictureParametersSet);

    operator VkVideoSessionParametersKHR() const {
        assert(m_sessionParameters != VK_NULL_HANDLE);
        return m_sessionParameters;
    }

    VkVideoSessionParametersKHR GetVideoSessionParametersKHR() const {
        assert(m_sessionParameters != VK_NULL_HANDLE);
        return m_sessionParameters;
    }

    int32_t GetId() const { return m_Id; }

    bool HasVpsId(uint32_t vpsId) const {
        return m_vpsIdsUsed[vpsId];
    }

    bool HasSpsId(uint32_t spsId) const {
        return m_spsIdsUsed[spsId];
    }

    bool HasPpsId(uint32_t ppsId) const {
        return m_ppsIdsUsed[ppsId];
    }

    bool HasAv1PpsId(uint32_t ppsId) const {
        return m_av1SpsIdsUsed[ppsId];
    }

    bool UpdatePictureParametersHierarchy(VkSharedBaseObj<StdVideoPictureParametersSet>& pictureParametersObject);

    VkResult AddPictureParametersToQueue(VkSharedBaseObj<StdVideoPictureParametersSet>& pictureParametersSet);
    int32_t FlushPictureParametersQueue(VkSharedBaseObj<VulkanVideoSession>& videoSession);

protected:
    VkParserVideoPictureParameters(const VulkanDeviceContext* vkDevCtx,
                                   VkSharedBaseObj<VkParserVideoPictureParameters>& templatePictureParameters)
        : m_classId(m_refClassId),
          m_Id(-1),
          m_refCount(0),
          m_vkDevCtx(vkDevCtx),
          m_videoSession(),
          m_sessionParameters(),
          m_templatePictureParameters(templatePictureParameters) { }

    virtual ~VkParserVideoPictureParameters();

private:
    static const char*              m_refClassId;
    static int32_t                  m_currentId;
    const char*                     m_classId;
    int32_t                         m_Id;
    std::atomic<int32_t>            m_refCount;
    const VulkanDeviceContext*      m_vkDevCtx;
    VkSharedBaseObj<VulkanVideoSession> m_videoSession;
    VkVideoSessionParametersKHR     m_sessionParameters;
    std::bitset<MAX_VPS_IDS>        m_vpsIdsUsed;
    std::bitset<MAX_SPS_IDS>        m_spsIdsUsed;
    std::bitset<MAX_PPS_IDS>        m_ppsIdsUsed;
    std::bitset<MAX_SPS_IDS>        m_av1SpsIdsUsed;
    VkSharedBaseObj<VkParserVideoPictureParameters> m_templatePictureParameters; // needed only for the create

    std::queue<VkSharedBaseObj<StdVideoPictureParametersSet>>  m_pictureParametersQueue;
    VkSharedBaseObj<StdVideoPictureParametersSet>              m_lastPictParamsQueue[StdVideoPictureParametersSet::NUM_OF_TYPES];
};

#endif /* _VKVIDEODECODER_VKPARSERVIDEOPICTUREPARAMETERS_H_ */
