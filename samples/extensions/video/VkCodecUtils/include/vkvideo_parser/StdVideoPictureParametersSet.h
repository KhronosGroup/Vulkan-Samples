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

#ifndef _VKVIDEODECODER_STDVIDEOPICTUREPARAMETERSSET_H_
#define _VKVIDEODECODER_STDVIDEOPICTUREPARAMETERSSET_H_

#include "vulkan_interfaces.h"

class StdVideoPictureParametersSet : public VkVideoRefCountBase
{
public:

    enum StdType {
        TYPE_H264_SPS = 0,
        TYPE_H264_PPS,
        TYPE_H265_VPS,
        TYPE_H265_SPS,
        TYPE_H265_PPS,
        TYPE_AV1_SPS,
    };

    enum ParameterType {
        PPS_TYPE = 0,
        SPS_TYPE,
        VPS_TYPE,
        AV1_SPS_TYPE,
        NUM_OF_TYPES,
        INVALID_TYPE,
    };

    virtual int32_t GetVpsId(bool& isVps) const = 0;
    virtual int32_t GetSpsId(bool& isSps) const = 0;
    virtual int32_t GetPpsId(bool& isPps) const = 0;

    virtual const StdVideoH264SequenceParameterSet* GetStdH264Sps() const { return nullptr; }
    virtual const StdVideoH264PictureParameterSet*  GetStdH264Pps() const { return nullptr; }
    virtual const StdVideoH265VideoParameterSet*    GetStdH265Vps() const { return nullptr; }
    virtual const StdVideoH265SequenceParameterSet* GetStdH265Sps() const { return nullptr; }
    virtual const StdVideoH265PictureParameterSet*  GetStdH265Pps() const { return nullptr; }
    virtual const StdVideoAV1SequenceHeader*        GetStdAV1Sps() const { return nullptr; }

    virtual const char* GetRefClassId() const = 0;

    static const StdVideoPictureParametersSet* StdVideoPictureParametersSetFromBase(const VkVideoRefCountBase* pBase ) {
        if (!pBase) {
            return NULL;
        }
        const StdVideoPictureParametersSet* pPictureParameters = static_cast<const StdVideoPictureParametersSet*>(pBase);
        if (pPictureParameters->IsMyClassId(pPictureParameters->GetRefClassId())) {
            return pPictureParameters;
        }
        assert(!"Invalid StdVideoPictureParametersSet from base");
        return nullptr;
    }

    virtual int32_t AddRef()
    {
        return ++m_refCount;
    }

    virtual int32_t Release()
    {
        uint32_t ret = --m_refCount;
        // Destroy the device if refcount reaches zero
        if (ret == 0) {
            delete this;
        }
        return ret;
    }

    bool IsMyClassId(const char* refClassId) const {
        if (m_classId == refClassId) {
            return true;
        }
        return false;
    }

    StdType GetStdType() const { return m_stdType; }
    ParameterType GetParameterType() const { return m_parameterType; }
    uint32_t GetUpdateSequenceCount() const { return m_updateSequenceCount; }

    // VkParserVideoPictureParameters
    virtual bool GetClientObject(VkSharedBaseObj<VkVideoRefCountBase>& clientObject) const = 0;

protected:
    StdVideoPictureParametersSet(StdType updateType,
                                 ParameterType itemType, const char* refClassId,
                                 uint64_t updateSequenceCount)
        : m_classId(refClassId)
        , m_refCount(0)
        , m_stdType(updateType)
        , m_parameterType(itemType)
        , m_updateSequenceCount((uint32_t)updateSequenceCount)
        , m_parent() { }

    virtual ~StdVideoPictureParametersSet()
    {
        m_parent = nullptr;
    }

private:
    const char*                                      m_classId;
    std::atomic<int32_t>                             m_refCount;
    StdType                                          m_stdType;
    ParameterType                                    m_parameterType;
protected:
    uint32_t                                         m_updateSequenceCount;
public:
    VkSharedBaseObj<StdVideoPictureParametersSet>    m_parent;        // SPS or PPS parent

};

#endif /* _VKVIDEODECODER_STDVIDEOPICTUREPARAMETERSSET_H_ */
