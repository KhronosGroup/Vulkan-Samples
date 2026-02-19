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

#ifndef _VULKANDEVICEMEMORYIMPL_H_
#define _VULKANDEVICEMEMORYIMPL_H_

#include <atomic>
#include "VkCodecUtils/VkVideoRefCountBase.h"
#include "VkCodecUtils/VulkanDeviceContext.h"

class VulkanDeviceMemoryImpl : public VkVideoRefCountBase
{
public:

    static VkResult Create(const VkMemoryRequirements& memoryRequirements,
                           VkMemoryPropertyFlags& memoryPropertyFlags,
                           const void* pInitializeMemory, VkDeviceSize initializeMemorySize, bool clearMemory,
                           VkSharedBaseObj<VulkanDeviceMemoryImpl>& vulkanDeviceMemory);

    virtual int32_t AddRef()
    {
        return ++m_refCount;
    }

    virtual int32_t Release()
    {
        uint32_t ret = --m_refCount;
        // Destroy the buffer if ref-count reaches zero
        if (ret == 0) {
            delete this;
        }
        return ret;
    }

    virtual VkDeviceSize GetMaxSize() const;
    virtual VkDeviceSize GetSizeAlignment() const;
    virtual VkDeviceSize Resize(VkDeviceSize newSize, VkDeviceSize copySize = 0, VkDeviceSize copyOffset = 0);

    virtual int64_t  MemsetData(uint32_t value, VkDeviceSize offset, VkDeviceSize size);
    virtual int64_t  CopyDataToBuffer(uint8_t *dstBuffer, VkDeviceSize dstOffset,
                                      VkDeviceSize srcOffset, VkDeviceSize size);
    virtual int64_t  CopyDataToBuffer(VkSharedBaseObj<VulkanDeviceMemoryImpl>& dstBuffer, VkDeviceSize dstOffset,
                                      VkDeviceSize srcOffset, VkDeviceSize size);
    virtual int64_t  CopyDataFromBuffer(const uint8_t *sourceBuffer, VkDeviceSize srcOffset,
                                        VkDeviceSize dstOffset, VkDeviceSize size);
    virtual int64_t  CopyDataFromBuffer(const VkSharedBaseObj<VulkanDeviceMemoryImpl>& sourceMemory, VkDeviceSize srcOffset,
                                        VkDeviceSize dstOffset, VkDeviceSize size);
    virtual uint8_t* GetDataPtr(VkDeviceSize offset, VkDeviceSize &maxSize);
    virtual const uint8_t* GetReadOnlyDataPtr(VkDeviceSize offset, VkDeviceSize &maxSize);

    virtual void FlushRange(VkDeviceSize offset, VkDeviceSize size) const;
    virtual void InvalidateRange(VkDeviceSize offset, VkDeviceSize size) const;

    virtual VkDeviceMemory GetDeviceMemory() const { return m_deviceMemory; }
    operator VkDeviceMemory() { return m_deviceMemory; }
    operator bool() { return m_deviceMemory != VK_NULL_HANDLE; }

    VkMemoryPropertyFlags GetMemoryPropertyFlags() const { return m_memoryPropertyFlags; }

    const VkMemoryRequirements& GetMemoryRequirements() const { return m_memoryRequirements; }

    VkResult FlushInvalidateMappedMemoryRange(VkDeviceSize offset, VkDeviceSize size, bool flush = true)  const;

    VkResult CopyDataToMemory(const uint8_t* pData, VkDeviceSize size,
                              VkDeviceSize memoryOffset) const;

    uint8_t* CheckAccess(VkDeviceSize offset, VkDeviceSize size);

private:

    static VkResult CreateDeviceMemory(const VkMemoryRequirements& memoryRequirements,
                                       VkMemoryPropertyFlags& memoryPropertyFlags,
                                       VkDeviceMemory& deviceMemory,
                                       VkDeviceSize&   deviceMemoryOffset);


    VkResult Initialize(const VkMemoryRequirements& memoryRequirements,
                        VkMemoryPropertyFlags& memoryPropertyFlags,
                        const void* pInitializeMemory,
                        VkDeviceSize initializeMemorySize,
                        bool clearMemory);

    VulkanDeviceMemoryImpl()
        : m_refCount(0)
        , m_memoryRequirements()
        , m_memoryPropertyFlags()
        , m_deviceMemory()
        , m_deviceMemoryOffset()
        , m_deviceMemoryDataPtr(nullptr) { }

    void Deinitialize();

    ~VulkanDeviceMemoryImpl() override { Deinitialize(); }

	std::atomic<int32_t>       m_refCount;
    VkMemoryRequirements       m_memoryRequirements;
    VkMemoryPropertyFlags      m_memoryPropertyFlags;
    VkDeviceMemory             m_deviceMemory;
    VkDeviceSize               m_deviceMemoryOffset;
    uint8_t*                   m_deviceMemoryDataPtr;
};

#endif /* _VULKANDEVICEMEMORYIMPL_H_ */
