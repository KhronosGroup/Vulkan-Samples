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

#include "VkVideoDecoder/VkParserVideoPictureParameters.h"

const char* VkParserVideoPictureParameters::m_refClassId = "VkParserVideoPictureParameters";
int32_t VkParserVideoPictureParameters::m_currentId = 0;

int32_t VkParserVideoPictureParameters::PopulateH264UpdateFields(const StdVideoPictureParametersSet* pStdPictureParametersSet,
                                                                 VkVideoDecodeH264SessionParametersAddInfoKHR& h264SessionParametersAddInfo)
{
    int32_t currentId = -1;
    if (pStdPictureParametersSet == nullptr) {
        return currentId;
    }

    assert( (pStdPictureParametersSet->GetStdType() == StdVideoPictureParametersSet::TYPE_H264_SPS) ||
            (pStdPictureParametersSet->GetStdType() == StdVideoPictureParametersSet::TYPE_H264_PPS));

    assert(h264SessionParametersAddInfo.sType == VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_SESSION_PARAMETERS_ADD_INFO_KHR);

    if (pStdPictureParametersSet->GetStdType() == StdVideoPictureParametersSet::TYPE_H264_SPS) {
        h264SessionParametersAddInfo.stdSPSCount = 1;
        h264SessionParametersAddInfo.pStdSPSs = pStdPictureParametersSet->GetStdH264Sps();
        bool isSps = false;
        currentId = pStdPictureParametersSet->GetSpsId(isSps);
        assert(isSps);
    } else if (pStdPictureParametersSet->GetStdType() ==  StdVideoPictureParametersSet::TYPE_H264_PPS ) {
        h264SessionParametersAddInfo.stdPPSCount = 1;
        h264SessionParametersAddInfo.pStdPPSs = pStdPictureParametersSet->GetStdH264Pps();
        bool isPps = false;
        currentId = pStdPictureParametersSet->GetPpsId(isPps);
        assert(isPps);
    } else {
        assert(!"Incorrect h.264 type");
    }

    return currentId;
}

int32_t VkParserVideoPictureParameters::PopulateH265UpdateFields(const StdVideoPictureParametersSet* pStdPictureParametersSet,
                                                                 VkVideoDecodeH265SessionParametersAddInfoKHR& h265SessionParametersAddInfo)
{
    int32_t currentId = -1;
    if (pStdPictureParametersSet == nullptr) {
        return currentId;
    }

    assert( (pStdPictureParametersSet->GetStdType() == StdVideoPictureParametersSet::TYPE_H265_VPS) ||
            (pStdPictureParametersSet->GetStdType() == StdVideoPictureParametersSet::TYPE_H265_SPS) ||
            (pStdPictureParametersSet->GetStdType() == StdVideoPictureParametersSet::TYPE_H265_PPS));

    assert(h265SessionParametersAddInfo.sType == VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_SESSION_PARAMETERS_ADD_INFO_KHR);

    if (pStdPictureParametersSet->GetStdType() == StdVideoPictureParametersSet::TYPE_H265_VPS) {
        h265SessionParametersAddInfo.stdVPSCount = 1;
        h265SessionParametersAddInfo.pStdVPSs = pStdPictureParametersSet->GetStdH265Vps();
        bool isVps = false;
        currentId = pStdPictureParametersSet->GetVpsId(isVps);
        assert(isVps);
    } else if (pStdPictureParametersSet->GetStdType() == StdVideoPictureParametersSet::TYPE_H265_SPS) {
        h265SessionParametersAddInfo.stdSPSCount = 1;
        h265SessionParametersAddInfo.pStdSPSs = pStdPictureParametersSet->GetStdH265Sps();
        bool isSps = false;
        currentId = pStdPictureParametersSet->GetSpsId(isSps);
        assert(isSps);
    } else if (pStdPictureParametersSet->GetStdType() == StdVideoPictureParametersSet::TYPE_H265_PPS) {
        h265SessionParametersAddInfo.stdPPSCount = 1;
        h265SessionParametersAddInfo.pStdPPSs = pStdPictureParametersSet->GetStdH265Pps();
        bool isPps = false;
        currentId = pStdPictureParametersSet->GetPpsId(isPps);
        assert(isPps);
    } else {
        assert(!"Incorrect h.265 type");
    }

    return currentId;
}

VkResult
VkParserVideoPictureParameters::Create(const VulkanDeviceContext* vkDevCtx,
                                       VkSharedBaseObj<VkParserVideoPictureParameters>& templatePictureParameters,
                                       VkSharedBaseObj<VkParserVideoPictureParameters>& videoPictureParameters)
{
    VkSharedBaseObj<VkParserVideoPictureParameters> newVideoPictureParameters(
            new VkParserVideoPictureParameters(vkDevCtx, templatePictureParameters));
    if (!newVideoPictureParameters) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    videoPictureParameters = newVideoPictureParameters;
    return VK_SUCCESS;
}

VkResult VkParserVideoPictureParameters::CreateParametersObject(const VulkanDeviceContext* vkDevCtx,
                                                                VkSharedBaseObj<VulkanVideoSession>& videoSession,
                                                                const StdVideoPictureParametersSet* pStdVideoPictureParametersSet,
                                                                VkParserVideoPictureParameters* pTemplatePictureParameters)
{
    int32_t currentId = -1;

    VkVideoSessionParametersCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_VIDEO_SESSION_PARAMETERS_CREATE_INFO_KHR };

    VkVideoDecodeH264SessionParametersCreateInfoKHR h264SessionParametersCreateInfo = { VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_SESSION_PARAMETERS_CREATE_INFO_KHR};
    VkVideoDecodeH264SessionParametersAddInfoKHR h264SessionParametersAddInfo = { VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_SESSION_PARAMETERS_ADD_INFO_KHR };

    VkVideoDecodeH265SessionParametersCreateInfoKHR h265SessionParametersCreateInfo = { VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_SESSION_PARAMETERS_CREATE_INFO_KHR };
    VkVideoDecodeH265SessionParametersAddInfoKHR h265SessionParametersAddInfo = { VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_SESSION_PARAMETERS_ADD_INFO_KHR};

    VkVideoDecodeAV1SessionParametersCreateInfoKHR av1SessionParametersCreateInfo = { VK_STRUCTURE_TYPE_VIDEO_DECODE_AV1_SESSION_PARAMETERS_CREATE_INFO_KHR };

    StdVideoPictureParametersSet::StdType updateType = pStdVideoPictureParametersSet->GetStdType();
    switch (updateType)
    {
        case StdVideoPictureParametersSet::TYPE_H264_SPS:
        case StdVideoPictureParametersSet::TYPE_H264_PPS:
        {
            createInfo.pNext =  &h264SessionParametersCreateInfo;
            h264SessionParametersCreateInfo.maxStdSPSCount = MAX_SPS_IDS;
            h264SessionParametersCreateInfo.maxStdPPSCount = MAX_PPS_IDS;
            h264SessionParametersCreateInfo.pParametersAddInfo = &h264SessionParametersAddInfo;

            currentId = PopulateH264UpdateFields(pStdVideoPictureParametersSet, h264SessionParametersAddInfo);

        }
        break;
        case StdVideoPictureParametersSet::TYPE_H265_VPS:
        case StdVideoPictureParametersSet::TYPE_H265_SPS:
        case StdVideoPictureParametersSet::TYPE_H265_PPS:
        {
            createInfo.pNext =  &h265SessionParametersCreateInfo;
            h265SessionParametersCreateInfo.maxStdVPSCount = MAX_VPS_IDS;
            h265SessionParametersCreateInfo.maxStdSPSCount = MAX_SPS_IDS;
            h265SessionParametersCreateInfo.maxStdPPSCount = MAX_PPS_IDS;
            h265SessionParametersCreateInfo.pParametersAddInfo = &h265SessionParametersAddInfo;

            currentId = PopulateH265UpdateFields(pStdVideoPictureParametersSet, h265SessionParametersAddInfo);
        }
        break;
        case StdVideoPictureParametersSet::TYPE_AV1_SPS:
        {
            createInfo.pNext = &av1SessionParametersCreateInfo;
            if (pStdVideoPictureParametersSet == nullptr) {
                currentId = -1;
            } else {
                assert(pStdVideoPictureParametersSet->GetStdType() == StdVideoPictureParametersSet::TYPE_AV1_SPS);
                assert(av1SessionParametersCreateInfo.sType == VK_STRUCTURE_TYPE_VIDEO_DECODE_AV1_SESSION_PARAMETERS_CREATE_INFO_KHR);
                av1SessionParametersCreateInfo.pStdSequenceHeader = const_cast<StdVideoAV1SequenceHeader*>(pStdVideoPictureParametersSet->GetStdAV1Sps());
                currentId = 0;
            }

            // VUID-VkVideoSessionParametersCreateInfoKHR-videoSession-09258
            // If videoSession was created with the video codec operation VK_VIDEO_CODEC_OPERATION_DECODE_AV1_BIT_KHR,
            // then videoSessionParametersTemplate must be VK_NULL_HANDLE
            // AV1 does not support template parameters.
            pTemplatePictureParameters = nullptr;
        }
        break;
        default:
            assert(!"Invalid Parser format");
            return VK_ERROR_INITIALIZATION_FAILED;
    }

    createInfo.videoSessionParametersTemplate = pTemplatePictureParameters ? VkVideoSessionParametersKHR(*pTemplatePictureParameters) : VK_NULL_HANDLE;
    createInfo.videoSession = videoSession->GetVideoSession();
    VkResult result = vkDevCtx->CreateVideoSessionParametersKHR(*vkDevCtx,
                                                                &createInfo,
                                                                nullptr,
                                                                &m_sessionParameters);
    if (result != VK_SUCCESS) {

        assert(!"Could not create Session Parameters Object");
        return result;

    } else {

        m_videoSession = videoSession;

        if (pTemplatePictureParameters) {
            m_vpsIdsUsed = pTemplatePictureParameters->m_vpsIdsUsed;
            m_spsIdsUsed = pTemplatePictureParameters->m_spsIdsUsed;
            m_ppsIdsUsed = pTemplatePictureParameters->m_ppsIdsUsed;
            m_av1SpsIdsUsed = pTemplatePictureParameters->m_av1SpsIdsUsed;
        }

        assert (currentId >= 0);
        switch (pStdVideoPictureParametersSet->GetParameterType()) {
            case StdVideoPictureParametersSet::PPS_TYPE:
                m_ppsIdsUsed.set(currentId, true);
                break;

            case StdVideoPictureParametersSet::SPS_TYPE:
                m_spsIdsUsed.set(currentId, true);
                break;

            case StdVideoPictureParametersSet::VPS_TYPE:
                m_vpsIdsUsed.set(currentId, true);
            break;

            case StdVideoPictureParametersSet::AV1_SPS_TYPE:
                m_av1SpsIdsUsed.set(currentId, true);
                break;
            default:
                assert(!"Invalid StdVideoPictureParametersSet Parameter Type!");
        }
        m_Id = ++m_currentId;
    }

    return result;
}

VkResult VkParserVideoPictureParameters::UpdateParametersObject(const StdVideoPictureParametersSet* pStdVideoPictureParametersSet)
{
    if (pStdVideoPictureParametersSet == nullptr) {
        return VK_SUCCESS;
    }

    int32_t currentId = -1;
    VkVideoSessionParametersUpdateInfoKHR updateInfo = { VK_STRUCTURE_TYPE_VIDEO_SESSION_PARAMETERS_UPDATE_INFO_KHR };
    VkVideoDecodeH264SessionParametersAddInfoKHR h264SessionParametersAddInfo = { VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_SESSION_PARAMETERS_ADD_INFO_KHR };
    VkVideoDecodeH265SessionParametersAddInfoKHR h265SessionParametersAddInfo = { VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_SESSION_PARAMETERS_ADD_INFO_KHR};

    StdVideoPictureParametersSet::StdType updateType = pStdVideoPictureParametersSet->GetStdType();
    switch (updateType)
    {
        case StdVideoPictureParametersSet::TYPE_H264_SPS:
        case StdVideoPictureParametersSet::TYPE_H264_PPS:
        {
            updateInfo.pNext = &h264SessionParametersAddInfo;
            currentId = PopulateH264UpdateFields(pStdVideoPictureParametersSet, h264SessionParametersAddInfo);
        }
        break;
        case StdVideoPictureParametersSet::TYPE_H265_VPS:
        case StdVideoPictureParametersSet::TYPE_H265_SPS:
        case StdVideoPictureParametersSet::TYPE_H265_PPS:
        {
            updateInfo.pNext = &h265SessionParametersAddInfo;
            currentId = PopulateH265UpdateFields(pStdVideoPictureParametersSet, h265SessionParametersAddInfo);
        }
        break;
        case StdVideoPictureParametersSet::TYPE_AV1_SPS:
        {
            assert(false && "There should be no calls to UpdateParametersObject for AV1");
            return VK_SUCCESS;
        }
        break;
        default:
            assert(!"Invalid Parser format");
            return VK_ERROR_INITIALIZATION_FAILED;
    }

    updateInfo.updateSequenceCount = std::max(pStdVideoPictureParametersSet->GetUpdateSequenceCount(), updateInfo.updateSequenceCount);


    VkResult result = m_vkDevCtx->UpdateVideoSessionParametersKHR(*m_vkDevCtx,
                                                                  m_sessionParameters,
                                                                  &updateInfo);

    if (result == VK_SUCCESS) {

        assert (currentId >= 0);
        switch (pStdVideoPictureParametersSet->GetParameterType()) {
            case StdVideoPictureParametersSet::PPS_TYPE:
                m_ppsIdsUsed.set(currentId, true);
                break;

            case StdVideoPictureParametersSet::SPS_TYPE:
                m_spsIdsUsed.set(currentId, true);
                break;

            case StdVideoPictureParametersSet::VPS_TYPE:
                m_vpsIdsUsed.set(currentId, true);
            break;

            case StdVideoPictureParametersSet::AV1_SPS_TYPE:
            m_av1SpsIdsUsed.set(currentId, true);
            break;
            default:
                assert(!"Invalid StdVideoPictureParametersSet Parameter Type!");
        }

    } else {
        assert(!"Could not update Session Parameters Object");
    }

    return result;
}

VkParserVideoPictureParameters::~VkParserVideoPictureParameters()
{
    if (m_sessionParameters) {
        m_vkDevCtx->DestroyVideoSessionParametersKHR(*m_vkDevCtx, m_sessionParameters, nullptr);
        m_sessionParameters = VkVideoSessionParametersKHR();
    }
    m_videoSession = nullptr;
}

bool VkParserVideoPictureParameters::UpdatePictureParametersHierarchy(
        VkSharedBaseObj<StdVideoPictureParametersSet>& pictureParametersObject)
{
    int32_t nodeId = -1;
    bool isNodeId = false;
    StdVideoPictureParametersSet::ParameterType nodeParent = StdVideoPictureParametersSet::INVALID_TYPE;
    StdVideoPictureParametersSet::ParameterType nodeChild = StdVideoPictureParametersSet::INVALID_TYPE;
    switch (pictureParametersObject->GetParameterType()) {
    case StdVideoPictureParametersSet::PPS_TYPE:
        nodeParent = StdVideoPictureParametersSet::SPS_TYPE;
        nodeId = pictureParametersObject->GetPpsId(isNodeId);
        if (!((uint32_t)nodeId < VkParserVideoPictureParameters::MAX_PPS_IDS)) {
            assert(!"PPS ID is out of bounds");
            return false;
        }
        assert(isNodeId);
        if (m_lastPictParamsQueue[nodeParent]) {
            bool isParentId = false;
            const int32_t spsParentId = pictureParametersObject->GetSpsId(isParentId);
            assert(!isParentId);
            if (spsParentId == m_lastPictParamsQueue[nodeParent]->GetSpsId(isParentId)) {
                assert(isParentId);
                pictureParametersObject->m_parent = m_lastPictParamsQueue[nodeParent];
            }
        }
        break;
    case StdVideoPictureParametersSet::SPS_TYPE:
        nodeParent = StdVideoPictureParametersSet::VPS_TYPE;
        nodeChild = StdVideoPictureParametersSet::PPS_TYPE;
        nodeId = pictureParametersObject->GetSpsId(isNodeId);
        if (!((uint32_t)nodeId < VkParserVideoPictureParameters::MAX_SPS_IDS)) {
            assert(!"SPS ID is out of bounds");
            return false;
        }
        assert(isNodeId);
        if (m_lastPictParamsQueue[nodeChild]) {
            const int32_t spsChildId = m_lastPictParamsQueue[nodeChild]->GetSpsId(isNodeId);
            assert(!isNodeId);
            if (spsChildId == nodeId) {
                m_lastPictParamsQueue[nodeChild]->m_parent = pictureParametersObject;
            }
        }
        if (m_lastPictParamsQueue[nodeParent]) {
            const int32_t vpsParentId = pictureParametersObject->GetVpsId(isNodeId);
            assert(!isNodeId);
            if (vpsParentId == m_lastPictParamsQueue[nodeParent]->GetVpsId(isNodeId)) {
                pictureParametersObject->m_parent = m_lastPictParamsQueue[nodeParent];
                assert(isNodeId);
            }
        }
        break;
    case StdVideoPictureParametersSet::VPS_TYPE:
        nodeChild = StdVideoPictureParametersSet::SPS_TYPE;
        nodeId = pictureParametersObject->GetVpsId(isNodeId);
        if (!((uint32_t)nodeId < VkParserVideoPictureParameters::MAX_VPS_IDS)) {
            assert(!"VPS ID is out of bounds");
            return false;
        }
        assert(isNodeId);
        if (m_lastPictParamsQueue[nodeChild]) {
            const int32_t vpsParentId = m_lastPictParamsQueue[nodeChild]->GetVpsId(isNodeId);
            assert(!isNodeId);
            if (vpsParentId == nodeId) {
                m_lastPictParamsQueue[nodeChild]->m_parent = pictureParametersObject;
            }
        }
        break;
    default:
        assert("!Invalid STD type");
        return false;
    }
    m_lastPictParamsQueue[pictureParametersObject->GetParameterType()] = pictureParametersObject;

    return true;
}

VkResult VkParserVideoPictureParameters::AddPictureParametersToQueue(VkSharedBaseObj<StdVideoPictureParametersSet>& pictureParametersSet)
{
    m_pictureParametersQueue.push(pictureParametersSet);
    return VK_SUCCESS;
}

VkResult VkParserVideoPictureParameters::HandleNewPictureParametersSet(VkSharedBaseObj<VulkanVideoSession>& videoSession,
        const StdVideoPictureParametersSet* pStdVideoPictureParametersSet)
{
    VkResult result;
    if (m_sessionParameters == VK_NULL_HANDLE) {
        assert(videoSession != VK_NULL_HANDLE);
        assert(m_videoSession == VK_NULL_HANDLE);
        if (m_templatePictureParameters) {
            m_templatePictureParameters->FlushPictureParametersQueue(videoSession);
        }
        result = CreateParametersObject(m_vkDevCtx, videoSession, pStdVideoPictureParametersSet,
                                        m_templatePictureParameters);
        assert(result == VK_SUCCESS);
        m_templatePictureParameters = nullptr; // the template object is not needed anymore
        m_videoSession = videoSession;

    } else {

        assert(m_videoSession != VK_NULL_HANDLE);
        assert(m_sessionParameters != VK_NULL_HANDLE);
        result = UpdateParametersObject(pStdVideoPictureParametersSet);
        assert(result == VK_SUCCESS);
    }

    return result;
}


int32_t VkParserVideoPictureParameters::FlushPictureParametersQueue(VkSharedBaseObj<VulkanVideoSession>& videoSession)
{
    if (!videoSession) {
        return -1;
    }
    uint32_t numQueueItems = 0;
    while (!m_pictureParametersQueue.empty()) {
        VkSharedBaseObj<StdVideoPictureParametersSet>& stdVideoPictureParametersSet = m_pictureParametersQueue.front();

        VkResult result =  HandleNewPictureParametersSet(videoSession, stdVideoPictureParametersSet);
        if (result != VK_SUCCESS) {
            return -1;
        }

        m_pictureParametersQueue.pop();
        numQueueItems++;
    }

    return numQueueItems;
}

bool VkParserVideoPictureParameters::CheckStdObjectBeforeUpdate(VkSharedBaseObj<StdVideoPictureParametersSet>& stdPictureParametersSet,
                                                                VkSharedBaseObj<VkParserVideoPictureParameters>& currentVideoPictureParameters)
{
    if (!stdPictureParametersSet) {
        return false;
    }

    bool stdObjectUpdate = (stdPictureParametersSet->GetUpdateSequenceCount() > 0);

    if (!currentVideoPictureParameters || stdObjectUpdate) {

        // Create new Vulkan Picture Parameters object
        return true;

    } else { // existing VkParserVideoPictureParameters object
        assert(currentVideoPictureParameters);
        // Update with the existing Vulkan Picture Parameters object
    }

    VkSharedBaseObj<VkVideoRefCountBase> clientObject;
    stdPictureParametersSet->GetClientObject(clientObject);
    assert(!clientObject);

    return false;
}

VkResult
VkParserVideoPictureParameters::AddPictureParameters(const VulkanDeviceContext* vkDevCtx,
                                                     VkSharedBaseObj<VulkanVideoSession>& videoSession,
                                                     VkSharedBaseObj<StdVideoPictureParametersSet>& stdPictureParametersSet,
                                                     VkSharedBaseObj<VkParserVideoPictureParameters>& currentVideoPictureParameters)
{

    if (!stdPictureParametersSet) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    if (currentVideoPictureParameters) {
        currentVideoPictureParameters->FlushPictureParametersQueue(videoSession);
    }

    VkResult result;
    if (CheckStdObjectBeforeUpdate(stdPictureParametersSet, currentVideoPictureParameters)) {
        result = VkParserVideoPictureParameters::Create(vkDevCtx,
                                                        currentVideoPictureParameters,
                                                        currentVideoPictureParameters);
    }

    if (videoSession) {
        result = currentVideoPictureParameters->HandleNewPictureParametersSet(videoSession, stdPictureParametersSet);
    } else {
        result = currentVideoPictureParameters->AddPictureParametersToQueue(stdPictureParametersSet);
    }

    return result;
}


int32_t VkParserVideoPictureParameters::AddRef()
{
    return ++m_refCount;
}

int32_t VkParserVideoPictureParameters::Release()
{
    uint32_t ret;
    ret = --m_refCount;
    // Destroy the device if refcount reaches zero
    if (ret == 0) {
        delete this;
    }
    return ret;
}
