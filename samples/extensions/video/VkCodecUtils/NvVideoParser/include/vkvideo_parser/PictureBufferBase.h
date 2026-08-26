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

#ifndef _PICTUREBUFFERBASE_H_
#define _PICTUREBUFFERBASE_H_

#include <assert.h>
#include <atomic>
#include <stdint.h>
#include <string.h>

class VkPicIf {
public:
    virtual void AddRef() = 0;
    virtual void Release() = 0;
    int32_t decodeWidth;
    int32_t decodeHeight;
    int32_t decodeSuperResWidth;
    int32_t reserved[16 - 5];
protected:
    virtual ~VkPicIf() { }
};

class vkPicBuffBase : public VkPicIf {
private:
    std::atomic<int32_t> m_refCount;

public:
    int32_t  m_picIdx;
    uint32_t m_displayOrder;
    uint64_t m_decodeOrder;
    uint64_t m_timestamp;
    uint64_t m_presentTime;

public:
    virtual void AddRef()
    {
        assert(m_refCount >= 0);
        ++m_refCount;
    }
    virtual void Release()
    {
        assert(m_refCount > 0);
        int32_t ref = --m_refCount;
        if (ref == 0) {
            Reset();
        }
    }

    vkPicBuffBase()
        : m_refCount(0)
        , m_picIdx(-1)
        , m_displayOrder((uint32_t)-1)
        , m_decodeOrder(0)
        , m_timestamp(0)
        , m_presentTime(0)
    {
    }

    bool IsAvailable() const
    {
        assert(m_refCount >= 0);
        return (m_refCount == 0);
    }

    int32_t Reset()
    {
        int32_t ref = m_refCount;
        m_refCount = 0;
        return ref;
    }

    virtual ~vkPicBuffBase()
    {
        Reset();
    }
};

#endif /* _PICTUREBUFFERBASE_H_ */
