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

#include <string.h>
#include "VkCodecUtils/VkBufferResource.h"
#include "VkCodecUtils/Helpers.h"

VkResult
VkBufferResource::Create(VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryPropertyFlags,
                         VkDeviceSize bufferSize,
                         VkSharedBaseObj<VkBufferResource>& vulkanBitstreamBuffer,
                         VkDeviceSize bufferOffsetAlignment, VkDeviceSize bufferSizeAlignment,
                         VkDeviceSize initializeBufferMemorySize, const void* pInitializeBufferMemory,
                         uint32_t queueFamilyCount, uint32_t* queueFamilyIndexes)
{
    VkSharedBaseObj<VkBufferResource> vkBitstreamBuffer(new VkBufferResource(usage,
                                                                             memoryPropertyFlags,
                                                                             bufferOffsetAlignment,
                                                                             bufferSizeAlignment,
                                                                             queueFamilyCount,
                                                                             queueFamilyIndexes));
    if (!vkBitstreamBuffer) {
        assert(!"Out of host memory!");
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    VkResult result = vkBitstreamBuffer->Initialize(bufferSize,
                                                    pInitializeBufferMemory,
                                                    initializeBufferMemorySize);
    if (result == VK_SUCCESS) {
        vulkanBitstreamBuffer = vkBitstreamBuffer;
    } else {
        assert(!"Initialize failed!");
    }

    return result;
}

VkDeviceSize VkBufferResource::Clone(VkDeviceSize newSize, VkDeviceSize copySize, VkDeviceSize copyOffset,
                                     VkSharedBaseObj<VkBufferResource>& vulkanBitstreamBuffer)
{
    VkSharedBaseObj<VkBufferResource> vkBitstreamBuffer(new VkBufferResource(m_usage,
                                                                             m_memoryPropertyFlags,
                                                                             m_bufferOffsetAlignment,
                                                                             m_bufferSizeAlignment,
                                                                             (uint32_t)m_queueFamilyIndexes.size(),
                                                                             m_queueFamilyIndexes.data()));
    if (!vkBitstreamBuffer) {
        assert(!"Out of host memory!");
        return 0;
    }

    uint8_t* oldBufPtr = nullptr;
    if (copySize) {
        VkDeviceSize maxSize = copyOffset;
        oldBufPtr = GetDataPtr(copyOffset, maxSize);
    }
    VkResult result = vkBitstreamBuffer->Initialize(newSize, oldBufPtr, copySize);
    if (result == VK_SUCCESS) {
        vulkanBitstreamBuffer = vkBitstreamBuffer;
    } else {
        assert(!"Initialize failed!");
    }

    return newSize;
}

VkResult VkBufferResource::CreateBuffer(VkBufferUsageFlags usage,
                                        VkDeviceSize& bufferSize,
                                        VkDeviceSize  bufferSizeAlignment,
                                        VkBuffer& buffer,
                                        VkDeviceSize& bufferOffset,
                                        VkMemoryPropertyFlags& memoryPropertyFlags,
                                        VkDeviceSize initializeBufferMemorySize,
                                        const void* pInitializeBufferMemory,
                                        const std::vector<uint32_t>& queueFamilyIndexes,
                                        VkSharedBaseObj<VulkanDeviceMemoryImpl>& vulkanDeviceMemory)
{
    bufferSize = ((bufferSize + (bufferSizeAlignment - 1)) & ~(bufferSizeAlignment - 1));
    bufferOffset = 0;

    // Create the buffer
    VkBufferCreateInfo createBufferInfo = VkBufferCreateInfo();
    createBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createBufferInfo.size = bufferSize;
    createBufferInfo.usage = usage;
    createBufferInfo.flags = 0;
    createBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createBufferInfo.queueFamilyIndexCount = (uint32_t)queueFamilyIndexes.size();
    createBufferInfo.pQueueFamilyIndices = queueFamilyIndexes.data();

    VkResult result = VulkanDeviceContext::GetThe()->CreateBuffer(VulkanDeviceContext::GetThe()->getDevice(), &createBufferInfo, nullptr, &buffer);
    if (result != VK_SUCCESS) {
        assert(!"Create Buffer failed!");
        return result;
    }

    VkMemoryRequirements memoryRequirements = VkMemoryRequirements();
    VulkanDeviceContext::GetThe()->GetBufferMemoryRequirements(VulkanDeviceContext::GetThe()->getDevice(), buffer, &memoryRequirements);

    // Allocate memory for the buffer
    VkSharedBaseObj<VulkanDeviceMemoryImpl> vkDeviceMemory;
    result = VulkanDeviceMemoryImpl::Create(memoryRequirements,
                                            memoryPropertyFlags,
                                            pInitializeBufferMemory,
                                            initializeBufferMemorySize,
#ifdef CLEAR_BITSTREAM_BUFFERS_ON_CREATE
                                            true, // clearMemory
#else
                                            false, // clearMemory
#endif
                                            vkDeviceMemory);
    if (result != VK_SUCCESS) {
        VulkanDeviceContext::GetThe()->DestroyBuffer(VulkanDeviceContext::GetThe()->getDevice(), buffer, nullptr);
        assert(!"Create Memory Failed!");
        return result;
    }

    result = VulkanDeviceContext::GetThe()->BindBufferMemory(VulkanDeviceContext::GetThe()->getDevice(), buffer, *vkDeviceMemory, bufferOffset);
    if (result != VK_SUCCESS) {
        VulkanDeviceContext::GetThe()->DestroyBuffer(VulkanDeviceContext::GetThe()->getDevice(), buffer, nullptr);
        assert(!"Bind buffer memory failed!");
        return result;
    }

    vulkanDeviceMemory = vkDeviceMemory;

    return result;
}

VkResult VkBufferResource::Initialize(VkDeviceSize bufferSize,
                                      const void* pInitializeBufferMemory,
                                      VkDeviceSize initializeBufferMemorySize)
{
    if (m_bufferSize >= bufferSize) {
#ifdef CLEAR_BITSTREAM_BUFFERS_ON_CREATE
        VkDeviceSize ret = MemsetData(0x00, 0, m_bufferSize);
        if (ret != m_bufferSize) {
            assert(!"Could't MemsetData()!");
            return VK_ERROR_INITIALIZATION_FAILED;
        }
#endif
        return VK_SUCCESS;
    }

    Deinitialize();

    VkResult result = CreateBuffer(m_usage,
                                   bufferSize,
                                   m_bufferSizeAlignment,
                                   m_buffer,
                                   m_bufferOffset,
                                   m_memoryPropertyFlags,
                                   initializeBufferMemorySize,
                                   pInitializeBufferMemory,
                                   m_queueFamilyIndexes,
                                   m_vulkanDeviceMemory);

    if (result != VK_SUCCESS) {
        assert(!"Create new buffer failed!");
        return result;
    }

    m_bufferSize = bufferSize;

    return result;
}

void VkBufferResource::Deinitialize()
{
    if (m_buffer) {
        VulkanDeviceContext::GetThe()->DestroyBuffer(VulkanDeviceContext::GetThe()->getDevice(), m_buffer, nullptr);
        m_buffer = VK_NULL_HANDLE;
    }

    m_vulkanDeviceMemory = nullptr;

    m_bufferOffset = 0;
    m_bufferSize = 0;
}

VkResult VkBufferResource::CopyDataToBuffer(const uint8_t* pData,
                                            VkDeviceSize size,
                                            VkDeviceSize &dstBufferOffset) const
{
    if ((pData == nullptr) || (size == 0)) {
        assert(!"CopyDataToBuffer failed!");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    dstBufferOffset = ((dstBufferOffset + (m_bufferOffsetAlignment - 1)) & ~(m_bufferOffsetAlignment - 1));
    assert((dstBufferOffset + size) <= m_bufferSize);

    return m_vulkanDeviceMemory->CopyDataToMemory(pData, size,  m_bufferOffset + dstBufferOffset);
}

VkDeviceSize VkBufferResource::GetMaxSize() const
{
    return m_bufferSize;
}

VkDeviceSize VkBufferResource::GetOffsetAlignment() const
{
    return m_bufferOffsetAlignment;
}

VkDeviceSize VkBufferResource::GetSizeAlignment() const
{
    return m_vulkanDeviceMemory->GetMemoryRequirements().alignment;
}

VkDeviceSize VkBufferResource::Resize(VkDeviceSize newSize, VkDeviceSize copySize, VkDeviceSize copyOffset)
{
    if (m_bufferSize >= newSize) {
        return m_bufferSize;
    }

    VkBuffer        newBuffer = VK_NULL_HANDLE;
    VkDeviceSize    newBufferOffset = 0;
    VkSharedBaseObj<VulkanDeviceMemoryImpl> newDeviceMemory;

    const uint8_t* pInitializeBufferMemory = nullptr;
    if (copySize) {
        VkDeviceSize maxSize = 0;
        pInitializeBufferMemory = m_vulkanDeviceMemory->GetReadOnlyDataPtr(copyOffset, maxSize);
        assert(pInitializeBufferMemory);
        assert(copySize <= maxSize);
    }
    VkResult result = CreateBuffer(m_usage,
                                   newSize,
                                   m_bufferSizeAlignment,
                                   newBuffer,
                                   newBufferOffset,
                                   m_memoryPropertyFlags,
                                   copySize,
                                   pInitializeBufferMemory,
                                   m_queueFamilyIndexes,
                                   newDeviceMemory);

    if (result != VK_SUCCESS) {
        assert(!"CreateBuffer failed!");
        return 0;
    }

    Deinitialize();

    m_buffer = newBuffer;
    m_vulkanDeviceMemory = newDeviceMemory;
    m_bufferOffset = newBufferOffset;
    m_bufferSize = newSize;

    return newSize;
}

uint8_t* VkBufferResource::CheckAccess(VkDeviceSize offset, VkDeviceSize size) const
{
    if (offset + size <= m_bufferSize) {

        uint8_t* bufferDataPtr = m_vulkanDeviceMemory->CheckAccess(m_bufferOffset, size);

        if (bufferDataPtr == nullptr) {
            assert(!"Bad buffer access - can't map buffer!");
            return nullptr;
        }
        return bufferDataPtr + offset;
    }

    assert(!"Bad buffer access - out of range!");
    return nullptr;
}

int64_t VkBufferResource::MemsetData(uint32_t value, VkDeviceSize offset, VkDeviceSize size)
{
    if (size == 0) {
        return 0;
    }
    return m_vulkanDeviceMemory->MemsetData(value, m_bufferOffset + offset, size);
}

int64_t VkBufferResource::CopyDataToBuffer(uint8_t *dstBuffer, VkDeviceSize dstOffset,
                                           VkDeviceSize srcOffset, VkDeviceSize size) const
{
    if (size == 0) {
        return 0;
    }
    return m_vulkanDeviceMemory->CopyDataToBuffer(dstBuffer, dstOffset, m_bufferOffset + srcOffset, size);
}

int64_t VkBufferResource::CopyDataToBuffer(VkSharedBaseObj<VkBufferResource>& dstBuffer, VkDeviceSize dstOffset,
                                           VkDeviceSize srcOffset, VkDeviceSize size) const
{
    if (size == 0) {
        return 0;
    }
    const uint8_t* readData = CheckAccess(srcOffset, size);
    if (readData == nullptr) {
        assert(!"Could not CopyDataToBuffer!");
        return -1;
    }
    return dstBuffer->CopyDataFromBuffer(readData, 0, m_bufferOffset + dstOffset, size);
}

int64_t  VkBufferResource::CopyDataFromBuffer(const uint8_t *sourceBuffer, VkDeviceSize srcOffset,
                                              VkDeviceSize dstOffset, VkDeviceSize size)
{
    if (size == 0) {
        return 0;
    }
    return m_vulkanDeviceMemory->CopyDataFromBuffer(sourceBuffer, srcOffset, m_bufferOffset + dstOffset, size);
}

int64_t VkBufferResource::CopyDataFromBuffer(const VkSharedBaseObj<VkBufferResource>& sourceBuffer,
                                             VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size)
{
    if (size == 0) {
        return 0;
    }
    const uint8_t* readData = sourceBuffer->GetReadOnlyDataPtr(srcOffset, size);
    if (readData == nullptr) {
        assert(!"Could not CopyDataFromBuffer!");
        return -1;
    }

    return m_vulkanDeviceMemory->CopyDataFromBuffer(readData, 0, m_bufferOffset + dstOffset, size);
}

uint8_t* VkBufferResource::GetDataPtr(VkDeviceSize offset, VkDeviceSize &maxSize)
{
    uint8_t* readData = CheckAccess(offset, 1);
    if (readData == nullptr) {
        assert(!"Could not GetDataPtr()!");
        return nullptr;
    }
    maxSize = m_bufferSize - offset;
    return (uint8_t*)readData;
}

const uint8_t* VkBufferResource::GetReadOnlyDataPtr(VkDeviceSize offset, VkDeviceSize &maxSize) const
{
    uint8_t* readData = CheckAccess(offset, 1);
    if (readData == nullptr) {
        assert(!"Could not GetReadOnlyDataPtr()!");
        return nullptr;
    }
    maxSize = m_bufferSize - offset;
    return readData;
}

void VkBufferResource::FlushRange(VkDeviceSize offset, VkDeviceSize size) const
{
    if (size == 0) {
        return;
    }
    m_vulkanDeviceMemory->FlushRange(offset, size);
}

void VkBufferResource::InvalidateRange(VkDeviceSize offset, VkDeviceSize size) const
{
    if (size == 0) {
        return;
    }
    m_vulkanDeviceMemory->InvalidateRange(offset, size);
}
