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

#ifndef _VKBUFFERRESOURCE_H_
#define _VKBUFFERRESOURCE_H_

#include <vector>
#include <atomic>
#include <iostream>
#include "VkCodecUtils/VulkanDeviceContext.h"
#include "VkCodecUtils/VulkanDeviceMemoryImpl.h"

class VkBufferResource : public VkVideoRefCountBase
{
public:

    static VkResult Create(VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryPropertyFlags,
                           VkDeviceSize bufferSize,
                           VkSharedBaseObj<VkBufferResource>& vulkanBitstreamBuffer,
                           VkDeviceSize bufferOffsetAlignment = 1, VkDeviceSize bufferSizeAlignment = 1,
                           VkDeviceSize initializeBufferMemorySize = 0, const void* pInitializeBufferMemory = nullptr,
                           uint32_t queueFamilyCount = 0, uint32_t* queueFamilyIndexes = nullptr);

    virtual int32_t AddRef()
    {
        return ++m_refCount;
    }

    virtual int32_t Release()
    {
        uint32_t ret = --m_refCount;
        // Destroy the buffer if ref-count reaches zero
        if (ret == 0) {
            // std::cout << "Delete bitstream buffer " << this << " with size " << GetMaxSize() << std::endl;
            delete this;
        }
        return ret;
    }

    virtual int32_t GetRefCount()
    {
        assert(m_refCount > 0);
        return m_refCount;
    }

    virtual VkDeviceSize GetMaxSize() const;
    virtual VkDeviceSize GetOffsetAlignment() const;
    virtual VkDeviceSize GetSizeAlignment() const;
    virtual VkDeviceSize Resize(VkDeviceSize newSize, VkDeviceSize copySize = 0, VkDeviceSize copyOffset = 0);
    virtual VkDeviceSize Clone(VkDeviceSize newSize, VkDeviceSize copySize, VkDeviceSize copyOffset,
                               VkSharedBaseObj<VkBufferResource>& vulkanBitstreamBuffer);

    virtual int64_t  MemsetData(uint32_t value, VkDeviceSize offset, VkDeviceSize size);
    virtual int64_t  CopyDataToBuffer(uint8_t *dstBuffer, VkDeviceSize dstOffset,
                                      VkDeviceSize srcOffset, VkDeviceSize size) const;
    virtual int64_t  CopyDataToBuffer(VkSharedBaseObj<VkBufferResource>& dstBuffer, VkDeviceSize dstOffset,
                                      VkDeviceSize srcOffset, VkDeviceSize size) const;
    virtual int64_t  CopyDataFromBuffer(const uint8_t *sourceBuffer, VkDeviceSize srcOffset,
                                        VkDeviceSize dstOffset, VkDeviceSize size);
    virtual int64_t  CopyDataFromBuffer(const VkSharedBaseObj<VkBufferResource>& sourceBuffer, VkDeviceSize srcOffset,
                                        VkDeviceSize dstOffset, VkDeviceSize size);
    virtual uint8_t* GetDataPtr(VkDeviceSize offset, VkDeviceSize &maxSize);
    virtual const uint8_t* GetReadOnlyDataPtr(VkDeviceSize offset, VkDeviceSize &maxSize) const;

    virtual void FlushRange(VkDeviceSize offset, VkDeviceSize size) const;
    virtual void InvalidateRange(VkDeviceSize offset, VkDeviceSize size) const;

    virtual VkBuffer GetBuffer() const { return m_buffer; }
    virtual VkDeviceMemory GetDeviceMemory() const { return *m_vulkanDeviceMemory; }

    operator VkDeviceMemory() { return GetDeviceMemory(); }
    operator bool() { return m_buffer != VK_NULL_HANDLE; }

    VkResult CopyDataToBuffer(const uint8_t* pData, VkDeviceSize size,
                              VkDeviceSize &dstBufferOffset) const;

private:

    static VkResult CreateBuffer(VkBufferUsageFlags usage,
                                 VkDeviceSize& bufferSize,
                                 VkDeviceSize bufferSizeAlignment,
                                 VkBuffer& buffer,
                                 VkDeviceSize& bufferOffset,
                                 VkMemoryPropertyFlags& memoryPropertyFlags,
                                 VkDeviceSize initializeBufferMemorySize,
                                 const void* pInitializeBufferMemory,
                                 const std::vector<uint32_t>& queueFamilyIndexes,
                                 VkSharedBaseObj<VulkanDeviceMemoryImpl>& vulkanDeviceMemory);

    uint8_t* CheckAccess(VkDeviceSize offset, VkDeviceSize size) const;

    VkResult Initialize(VkDeviceSize bufferSize,
                        const void* pInitializeBufferMemory, VkDeviceSize initializeBufferMemorySize);

    VkBufferResource(VkBufferUsageFlags usage,
                     VkMemoryPropertyFlags memoryPropertyFlags,
                     VkDeviceSize bufferOffsetAlignment = 1,
                     VkDeviceSize bufferSizeAlignment = 1,
                     uint32_t queueFamilyCount = 0,
                     uint32_t* queueFamilyIndexes = nullptr)
        : m_refCount(0)
        , m_usage(usage)
        , m_memoryPropertyFlags(memoryPropertyFlags)
        , m_buffer()
        , m_bufferOffset()
        , m_bufferSize()
        , m_bufferOffsetAlignment(bufferOffsetAlignment)
        , m_bufferSizeAlignment(bufferSizeAlignment)
        , m_queueFamilyIndexes(queueFamilyIndexes, queueFamilyIndexes + queueFamilyCount)
        , m_vulkanDeviceMemory() { }

    void Deinitialize();

    virtual ~VkBufferResource() { Deinitialize(); }

private:
    std::atomic<int32_t>       m_refCount;
    VkBufferUsageFlags         m_usage;
    VkMemoryPropertyFlags      m_memoryPropertyFlags;
    VkBuffer                   m_buffer;
    VkDeviceSize               m_bufferOffset;
    VkDeviceSize               m_bufferSize;
    VkDeviceSize               m_bufferOffsetAlignment;
    VkDeviceSize               m_bufferSizeAlignment;
    std::vector<uint32_t>      m_queueFamilyIndexes;
    VkSharedBaseObj<VulkanDeviceMemoryImpl> m_vulkanDeviceMemory;
};

#endif /* _VKBUFFERRESOURCE_H_ */
