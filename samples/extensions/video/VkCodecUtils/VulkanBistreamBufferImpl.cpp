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
#include "VkCodecUtils/VulkanBistreamBufferImpl.h"
#include "VkCodecUtils/Helpers.h"

VkResult
VulkanBitstreamBufferImpl::Create(
        uint32_t queueFamilyIndex, VkBufferUsageFlags usage,
        VkDeviceSize bufferSize, VkDeviceSize bufferOffsetAlignment, VkDeviceSize bufferSizeAlignment,
        const void* pInitializeBufferMemory, VkDeviceSize initializeBufferMemorySize,
        VkSharedBaseObj<VulkanBitstreamBufferImpl>& vulkanBitstreamBuffer)
{
    VkSharedBaseObj<VulkanBitstreamBufferImpl> vkBitstreamBuffer(new VulkanBitstreamBufferImpl(
                                                                      queueFamilyIndex,
                                                                      usage,
                                                                      bufferOffsetAlignment,
                                                                      bufferSizeAlignment));
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

VkDeviceSize VulkanBitstreamBufferImpl::Clone(VkDeviceSize newSize, VkDeviceSize copySize, VkDeviceSize copyOffset,
                                                      VkSharedBaseObj<VulkanBitstreamBuffer>& vulkanBitstreamBuffer)
{
    VkSharedBaseObj<VulkanBitstreamBufferImpl> vkBitstreamBuffer(new VulkanBitstreamBufferImpl(
                                                                      m_queueFamilyIndex,
                                                                      m_usage,
                                                                      m_bufferOffsetAlignment,
                                                                      m_bufferSizeAlignment));
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

VkResult VulkanBitstreamBufferImpl::CreateBuffer(uint32_t queueFamilyIndex,
                                                 VkBufferUsageFlags usage,
                                                 VkDeviceSize& bufferSize,
                                                 VkDeviceSize  bufferSizeAlignment,
                                                 VkBuffer& buffer,
                                                 VkDeviceSize& bufferOffset,
                                                 VkMemoryPropertyFlags& memoryPropertyFlags,
                                                 const void* pInitializeBufferMemory,
                                                 VkDeviceSize initializeBufferMemorySize,
                                                 VkSharedBaseObj<VulkanDeviceMemoryImpl>& vulkanDeviceMemory)
{
    bufferSize = ((bufferSize + (bufferSizeAlignment - 1)) & ~(bufferSizeAlignment - 1));
    bufferOffset = 0;

    // Create the buffer
    VkBufferCreateInfo createBufferInfo = VkBufferCreateInfo();
    createBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createBufferInfo.size = bufferSize;
    createBufferInfo.usage = usage;
    createBufferInfo.flags = VK_BUFFER_CREATE_VIDEO_PROFILE_INDEPENDENT_BIT_KHR;
    createBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createBufferInfo.queueFamilyIndexCount = 1;
    createBufferInfo.pQueueFamilyIndices = &queueFamilyIndex;

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

VkResult VulkanBitstreamBufferImpl::Initialize(VkDeviceSize bufferSize,
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

    m_memoryPropertyFlags = (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT  |
                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                             VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
    VkResult result = CreateBuffer(m_queueFamilyIndex,
                                   m_usage,
                                   bufferSize,
                                   m_bufferSizeAlignment,
                                   m_buffer,
                                   m_bufferOffset,
                                   m_memoryPropertyFlags,
                                   pInitializeBufferMemory,
                                   initializeBufferMemorySize,
                                   m_vulkanDeviceMemory);

    if (result != VK_SUCCESS) {
        assert(!"Create new buffer failed!");
        return result;
    }

    m_bufferSize = bufferSize;

    return result;
}

void VulkanBitstreamBufferImpl::Deinitialize()
{
    if (m_buffer) {
        VulkanDeviceContext::GetThe()->DestroyBuffer(VulkanDeviceContext::GetThe()->getDevice(), m_buffer, nullptr);
        m_buffer = VK_NULL_HANDLE;
    }

    m_vulkanDeviceMemory = nullptr;

    m_bufferOffset = 0;
    m_bufferSize = 0;
}

VkResult VulkanBitstreamBufferImpl::CopyDataToBuffer(const uint8_t* pData,
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

VkDeviceSize VulkanBitstreamBufferImpl::GetMaxSize() const
{
    return m_bufferSize;
}

VkDeviceSize VulkanBitstreamBufferImpl::GetOffsetAlignment() const
{
    return m_bufferOffsetAlignment;
}

VkDeviceSize VulkanBitstreamBufferImpl::GetSizeAlignment() const
{
    return m_vulkanDeviceMemory->GetMemoryRequirements().alignment;
}

VkDeviceSize VulkanBitstreamBufferImpl::Resize(VkDeviceSize newSize, VkDeviceSize copySize, VkDeviceSize copyOffset)
{
    if (m_bufferSize >= newSize) {
        return m_bufferSize;
    }

    std::cout << " ======= Req resize old " << m_bufferSize << " -> new " << newSize << " ====== " << std::endl;

    VkBuffer        newBuffer = VK_NULL_HANDLE;
    VkDeviceSize    newBufferOffset = 0;
    VkSharedBaseObj<VulkanDeviceMemoryImpl> newDeviceMemory;
    VkMemoryPropertyFlags newMemoryPropertyFlags = (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT  |
                                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                                                    VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

    const uint8_t* pInitializeBufferMemory = nullptr;
    if (copySize) {
        VkDeviceSize maxSize = 0;
        pInitializeBufferMemory = m_vulkanDeviceMemory->GetReadOnlyDataPtr(copyOffset, maxSize);
        assert(pInitializeBufferMemory);
        assert(copySize <= maxSize);
    }
    VkResult result = CreateBuffer(
                                   m_queueFamilyIndex,
                                   m_usage,
                                   newSize,
                                   m_bufferSizeAlignment,
                                   newBuffer,
                                   newBufferOffset,
                                   newMemoryPropertyFlags,
                                   pInitializeBufferMemory,
                                   copySize,
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
    m_memoryPropertyFlags = newMemoryPropertyFlags;

    return newSize;
}

uint8_t* VulkanBitstreamBufferImpl::CheckAccess(VkDeviceSize offset, VkDeviceSize size) const
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

int64_t VulkanBitstreamBufferImpl::MemsetData(uint32_t value, VkDeviceSize offset, VkDeviceSize size)
{
    if (size == 0) {
        return 0;
    }
    return m_vulkanDeviceMemory->MemsetData(value, m_bufferOffset + offset, size);
}

int64_t VulkanBitstreamBufferImpl::CopyDataToBuffer(uint8_t *dstBuffer, VkDeviceSize dstOffset,
                                                    VkDeviceSize srcOffset, VkDeviceSize size) const
{
    if (size == 0) {
        return 0;
    }
    return m_vulkanDeviceMemory->CopyDataToBuffer(dstBuffer, dstOffset, m_bufferOffset + srcOffset, size);
}

int64_t VulkanBitstreamBufferImpl::CopyDataToBuffer(VkSharedBaseObj<VulkanBitstreamBuffer>& dstBuffer, VkDeviceSize dstOffset,
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

int64_t  VulkanBitstreamBufferImpl::CopyDataFromBuffer(const uint8_t *sourceBuffer, VkDeviceSize srcOffset,
                                                       VkDeviceSize dstOffset, VkDeviceSize size)
{
    if (size == 0) {
        return 0;
    }
    return m_vulkanDeviceMemory->CopyDataFromBuffer(sourceBuffer, srcOffset, m_bufferOffset + dstOffset, size);
}

int64_t VulkanBitstreamBufferImpl::CopyDataFromBuffer(const VkSharedBaseObj<VulkanBitstreamBuffer>& sourceBuffer,
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

uint8_t* VulkanBitstreamBufferImpl::GetDataPtr(VkDeviceSize offset, VkDeviceSize &maxSize)
{
    uint8_t* readData = CheckAccess(offset, 1);
    if (readData == nullptr) {
        assert(!"Could not GetDataPtr()!");
        return nullptr;
    }
    maxSize = m_bufferSize - offset;
    return (uint8_t*)readData;
}

const uint8_t* VulkanBitstreamBufferImpl::GetReadOnlyDataPtr(VkDeviceSize offset, VkDeviceSize &maxSize) const
{
    uint8_t* readData = CheckAccess(offset, 1);
    if (readData == nullptr) {
        assert(!"Could not GetReadOnlyDataPtr()!");
        return nullptr;
    }
    maxSize = m_bufferSize - offset;
    return readData;
}

void VulkanBitstreamBufferImpl::FlushRange(VkDeviceSize offset, VkDeviceSize size) const
{
    if (size == 0) {
        return;
    }
    m_vulkanDeviceMemory->FlushRange(offset, size);
}

void VulkanBitstreamBufferImpl::InvalidateRange(VkDeviceSize offset, VkDeviceSize size) const
{
    if (size == 0) {
        return;
    }
    m_vulkanDeviceMemory->InvalidateRange(offset, size);
}

uint32_t VulkanBitstreamBufferImpl::AddStreamMarker(uint32_t streamOffset)
{
    m_streamMarkers.push_back(streamOffset);
    return (uint32_t)(m_streamMarkers.size() - 1);
}

uint32_t VulkanBitstreamBufferImpl::SetStreamMarker(uint32_t streamOffset, uint32_t index)
{
    assert(index < (uint32_t)m_streamMarkers.size());
    if (!(index < (uint32_t)m_streamMarkers.size())) {
        return uint32_t(-1);
    }
    m_streamMarkers[index] = streamOffset;
    return index;
}

uint32_t VulkanBitstreamBufferImpl::GetStreamMarker(uint32_t index) const
{
    assert(index < (uint32_t)m_streamMarkers.size());
    return m_streamMarkers[index];
}

uint32_t VulkanBitstreamBufferImpl::GetStreamMarkersCount() const
{
    return (uint32_t)m_streamMarkers.size();
}

const uint32_t* VulkanBitstreamBufferImpl::GetStreamMarkersPtr(uint32_t startIndex, uint32_t& maxCount) const
{
    maxCount = (uint32_t)m_streamMarkers.size() - startIndex;
    return m_streamMarkers.data() + startIndex;
}

uint32_t VulkanBitstreamBufferImpl::ResetStreamMarkers()
{
    uint32_t oldSize = (uint32_t)m_streamMarkers.size();
    m_streamMarkers.clear();
    return oldSize;
}
