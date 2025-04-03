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
#include "VkCodecUtils/VulkanDeviceMemoryImpl.h"
#include "VkCodecUtils/Helpers.h"

VkResult
VulkanDeviceMemoryImpl::Create(const VkMemoryRequirements& memoryRequirements,
                               VkMemoryPropertyFlags& memoryPropertyFlags,
                               const void* pInitializeMemory, VkDeviceSize initializeMemorySize, bool clearMemory,
                               VkSharedBaseObj<VulkanDeviceMemoryImpl>& vulkanDeviceMemory)
{
    VkSharedBaseObj<VulkanDeviceMemoryImpl> vkDeviceMemory(new VulkanDeviceMemoryImpl());
    if (!vkDeviceMemory) {
        assert(!"Couldn't allocate host memory!");
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    VkResult result = vkDeviceMemory->Initialize(memoryRequirements, memoryPropertyFlags,
                                                 pInitializeMemory,
                                                 initializeMemorySize,
                                                 clearMemory);
    if (result == VK_SUCCESS) {
        vulkanDeviceMemory = vkDeviceMemory;
    }

    return result;
}

VkResult VulkanDeviceMemoryImpl::CreateDeviceMemory(const VkMemoryRequirements& memoryRequirements,
                                                    VkMemoryPropertyFlags& memoryPropertyFlags,
                                                    VkDeviceMemory& deviceMemory,
                                                    VkDeviceSize&   deviceMemoryOffset)
{
    assert(memoryRequirements.size ==
            ((memoryRequirements.size + (memoryRequirements.alignment - 1)) & ~(memoryRequirements.alignment - 1)));
    deviceMemoryOffset = 0;

    VkMemoryAllocateInfo allocInfo = VkMemoryAllocateInfo();
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.memoryTypeIndex = 0;  // Memory type assigned in the next step

    // Assign the proper memory type for that buffer
    allocInfo.allocationSize = memoryRequirements.size;
	vk::MapMemoryTypeToIndex(VulkanDeviceContext::GetThe()->getPhysicalDevice(),
                         memoryRequirements.memoryTypeBits,
                         memoryPropertyFlags,
                         &allocInfo.memoryTypeIndex);

    // Allocate memory for the buffer
    VkResult result = VulkanDeviceContext::GetThe()->AllocateMemory(VulkanDeviceContext::GetThe()->getDevice(), &allocInfo, nullptr, &deviceMemory);
    if (result != VK_SUCCESS) {
        assert(!"Couldn't allocate device memory!");
        return result;
    }

    return result;
}

VkResult VulkanDeviceMemoryImpl::Initialize(const VkMemoryRequirements& memoryRequirements,
                                            VkMemoryPropertyFlags& memoryPropertyFlags,
                                            const void* pInitializeMemory,
                                            VkDeviceSize initializeMemorySize,
                                            bool clearMemory)
{
    if (m_memoryRequirements.size >= memoryRequirements.size) {
        if (clearMemory) {
            VkDeviceSize ret = MemsetData(0x00, 0, m_memoryRequirements.size);
            if (ret != m_memoryRequirements.size) {
                assert(!"Couldn't allocate device memory!");
                return VK_ERROR_INITIALIZATION_FAILED;
            }
        }
        return VK_SUCCESS;
    }

    Deinitialize();

    VkResult result = CreateDeviceMemory(memoryRequirements,
                                         memoryPropertyFlags,
                                         m_deviceMemory,
                                         m_deviceMemoryOffset);

    if (result != VK_SUCCESS) {
        assert(!"Couldn't CreateDeviceMemory()!");
        return result;
    }

    m_memoryPropertyFlags = memoryPropertyFlags;
    m_memoryRequirements = memoryRequirements;

    if (m_memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {

        VkDeviceSize copySize = std::min<VkDeviceSize>(initializeMemorySize, m_memoryRequirements.size);
        CopyDataFromBuffer((const uint8_t*)pInitializeMemory,
                           0, // srcOffset
                           0, // dstOffset
                           copySize);

        if (clearMemory) {
            MemsetData(0x0, copySize, m_memoryRequirements.size - copySize);
        }
    }
    return result;
}

void VulkanDeviceMemoryImpl::Deinitialize()
{
    if (m_deviceMemoryDataPtr != nullptr) {
        VulkanDeviceContext::GetThe()->UnmapMemory(VulkanDeviceContext::GetThe()->getDevice(), m_deviceMemory);
        m_deviceMemoryDataPtr = nullptr;
    }

    if (m_deviceMemory) {
        VulkanDeviceContext::GetThe()->FreeMemory(VulkanDeviceContext::GetThe()->getDevice(), m_deviceMemory, nullptr);
        m_deviceMemory = VK_NULL_HANDLE;
    }

    m_deviceMemoryOffset = 0;
}

VkResult VulkanDeviceMemoryImpl::FlushInvalidateMappedMemoryRange(VkDeviceSize offset, VkDeviceSize size,
                                                                  bool flush) const
{
    VkResult result = VK_SUCCESS;

    if (((m_memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) != 0) &&
        ((m_memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)) {

        const VkMappedMemoryRange range = {
            VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,  // sType
            NULL,                                   // pNext
            m_deviceMemory,                         // memory
            offset,                                 // offset
            size,                                   // size
        };

        if (flush) {
            result = VulkanDeviceContext::GetThe()->FlushMappedMemoryRanges(VulkanDeviceContext::GetThe()->getDevice(), 1u, &range);
        } else {
            result = VulkanDeviceContext::GetThe()->InvalidateMappedMemoryRanges(VulkanDeviceContext::GetThe()->getDevice(), 1u, &range);
        }
    }

    return result;
}

void VulkanDeviceMemoryImpl::FlushRange(VkDeviceSize offset, VkDeviceSize size) const
{
    FlushInvalidateMappedMemoryRange(offset, size);
}

void VulkanDeviceMemoryImpl::InvalidateRange(VkDeviceSize offset, VkDeviceSize size) const
{
    FlushInvalidateMappedMemoryRange(offset, size, false);
}

VkResult VulkanDeviceMemoryImpl::CopyDataToMemory(const uint8_t* pData,
                                                  VkDeviceSize size,
                                                  VkDeviceSize memoryOffset) const
{
    if ((pData == nullptr) || (size == 0)) {
        assert(!"Couldn't CopyDataToMemory()!");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    uint8_t* pDst = NULL;
    assert((memoryOffset + size) <= m_memoryRequirements.size);
    VkResult result = VulkanDeviceContext::GetThe()->MapMemory(VulkanDeviceContext::GetThe()->getDevice(), m_deviceMemory, memoryOffset,
                                            size, 0, (void**)&pDst);

    if (result != VK_SUCCESS) {
        return result;
    }

    memcpy(pDst, pData, (size_t)size);

    result = FlushInvalidateMappedMemoryRange(memoryOffset, size);
    if (result != VK_SUCCESS) {
        assert(!"Couldn't FlushMappedMemoryRange()!");
        return result;
    }

    VulkanDeviceContext::GetThe()->UnmapMemory(VulkanDeviceContext::GetThe()->getDevice(), m_deviceMemory);

    return VK_SUCCESS;
}

VkDeviceSize VulkanDeviceMemoryImpl::GetMaxSize() const
{
    return m_memoryRequirements.size;
}

VkDeviceSize VulkanDeviceMemoryImpl::GetSizeAlignment() const
{
    return m_memoryRequirements.alignment;
}

VkDeviceSize VulkanDeviceMemoryImpl::Resize(VkDeviceSize newSize, VkDeviceSize copySize, VkDeviceSize copyOffset)
{
    if (m_memoryRequirements.size >= newSize) {
        return VK_SUCCESS;
    }

    VkMemoryRequirements memoryRequirements(m_memoryRequirements);
    memoryRequirements.size = ((newSize + (memoryRequirements.alignment - 1)) & ~(memoryRequirements.alignment - 1));
    VkDeviceMemory  newDeviceMemory = VK_NULL_HANDLE;
    VkDeviceSize    newBufferOffset = 0;
    VkMemoryPropertyFlags newMemoryPropertyFlags = m_memoryPropertyFlags;
    VkResult result = CreateDeviceMemory(
                                         memoryRequirements,
                                         newMemoryPropertyFlags,
                                         newDeviceMemory,
                                         newBufferOffset);

    if (result != VK_SUCCESS) {
        assert(!"Couldn't CreateDeviceMemory()!");
        return 0;
    }

    uint8_t* newBufferDataPtr = nullptr;
    if (copySize != 0) {
        result = VulkanDeviceContext::GetThe()->MapMemory(VulkanDeviceContext::GetThe()->getDevice(), newDeviceMemory, newBufferOffset,
                                        newSize, 0, (void**)&newBufferDataPtr);

        if ((result != VK_SUCCESS) || (newBufferDataPtr == nullptr)) {
            VulkanDeviceContext::GetThe()->UnmapMemory(VulkanDeviceContext::GetThe()->getDevice(), newDeviceMemory);
            VulkanDeviceContext::GetThe()->FreeMemory(VulkanDeviceContext::GetThe()->getDevice(), newDeviceMemory, nullptr);
            assert(!"Couldn't MapMemory()!");
            return 0;
        }

        copySize = std::min(copyOffset + copySize, m_memoryRequirements.size);
#ifdef CLEAR_DEVICE_MEMORY_ON_CREATE
        memset(newBufferDataPtr + copySize, 0x00, (size_t)(newSize - copySize));
#endif
        // Copy the old data.
        uint8_t* readData = CheckAccess(copyOffset, copySize);
        memcpy(newBufferDataPtr, readData, (size_t)copySize);
    }

    Deinitialize();

    m_memoryRequirements = memoryRequirements;
    m_memoryPropertyFlags = newMemoryPropertyFlags;
    m_deviceMemory = newDeviceMemory;
    m_deviceMemoryOffset = newBufferOffset;
    m_deviceMemoryDataPtr = newBufferDataPtr;

    if (copySize == 0) {
#ifdef CLEAR_DEVICE_MEMORY_ON_CREATE
        MemsetData(0x0, 0, newSize);
#endif
    }

    return newSize;
}

uint8_t* VulkanDeviceMemoryImpl::CheckAccess(VkDeviceSize offset, VkDeviceSize size)
{
    if (size == VK_WHOLE_SIZE) {
        size = m_memoryRequirements.size;
        if (offset < size) {
            size -= offset;
        } else {
            // invalid offset
            assert(!"CheckAccess() failed - buffer out of range!");
            return nullptr;
        }
    }

    if (offset + size <= m_memoryRequirements.size) {
        if (m_deviceMemoryDataPtr == nullptr) {
            VkResult result = VulkanDeviceContext::GetThe()->MapMemory(VulkanDeviceContext::GetThe()->getDevice(), m_deviceMemory, m_deviceMemoryOffset,
                                                    m_memoryRequirements.size, 0, (void**)&m_deviceMemoryDataPtr);
            if ((result != VK_SUCCESS) || (m_deviceMemoryDataPtr == nullptr)) {
                assert(!"Couldn't MapMemory()!");
                return nullptr;
            }
        }
        return m_deviceMemoryDataPtr + offset;
    }

    assert(!"CheckAccess() failed - buffer out of range!");
    return nullptr;
}

int64_t VulkanDeviceMemoryImpl::MemsetData(uint32_t value, VkDeviceSize offset, VkDeviceSize size)
{
    if (size == 0) {
        return 0;
    }
    uint8_t* setData = CheckAccess(offset, size);
    if (setData == nullptr) {
        assert(!"MemsetData() failed - buffer out of range!");
        return -1;
    }

    assert(size < std::numeric_limits<size_t>::max());
    memset(setData, value, (size_t)size);
    return size;
}

int64_t VulkanDeviceMemoryImpl::CopyDataToBuffer(uint8_t *dstBuffer, VkDeviceSize dstOffset,
                                                 VkDeviceSize srcOffset, VkDeviceSize size)
{
    if (size == 0) {
        return 0;
    }
    const uint8_t* readData = CheckAccess(srcOffset, size);
    if (readData == nullptr) {
        assert(!"CopyDataToBuffer() failed - buffer out of range!");
        return -1;
    }
    assert(size < std::numeric_limits<size_t>::max());
    memcpy(dstBuffer + dstOffset, readData, (size_t)size);
    return size;
}

int64_t VulkanDeviceMemoryImpl::CopyDataToBuffer(VkSharedBaseObj<VulkanDeviceMemoryImpl>& dstBuffer, VkDeviceSize dstOffset,
                                                 VkDeviceSize srcOffset, VkDeviceSize size)
{
    if (size == 0) {
        return 0;
    }
    const uint8_t* readData = CheckAccess(srcOffset, size);
    if (readData == nullptr) {
        assert(!"CopyDataToBuffer() failed - buffer out of range!");
        return -1;
    }

    dstBuffer->CopyDataFromBuffer(readData, 0, dstOffset, size);
    return size;
}

int64_t  VulkanDeviceMemoryImpl::CopyDataFromBuffer(const uint8_t* sourceBuffer, VkDeviceSize srcOffset,
                                                    VkDeviceSize dstOffset, VkDeviceSize size)
{
    uint8_t* writeData = CheckAccess(dstOffset, size);
    if (writeData == nullptr) {
        assert(!"CopyDataFromBuffer() failed - buffer out of range!");
        return -1;
    }
    if ((size != 0) && (sourceBuffer != nullptr)) {
        assert(size < std::numeric_limits<size_t>::max());
        memcpy(writeData, sourceBuffer + srcOffset, (size_t)size);
    }
    return size;
}

int64_t VulkanDeviceMemoryImpl::CopyDataFromBuffer(const VkSharedBaseObj<VulkanDeviceMemoryImpl>& sourceMemory,
                                                   VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size)
{
    if (size == 0) {
        return 0;
    }
    uint8_t* writeData = CheckAccess(dstOffset, size);
    if (writeData == nullptr) {
        assert(!"CopyDataFromBuffer() failed - buffer out of range!");
        return -1;
    }

    VkDeviceSize maxSize = 0;
    const uint8_t* srcPtr = sourceMemory->GetReadOnlyDataPtr(srcOffset, maxSize);
    if ((srcPtr == nullptr) || (maxSize < size)) {
        assert(!"GetReadOnlyDataPtr() failed - buffer out of range!");
        return -1;
    }

    assert(size < std::numeric_limits<size_t>::max());
    memcpy(writeData, srcPtr, (size_t)size);
    return size;
}

uint8_t* VulkanDeviceMemoryImpl::GetDataPtr(VkDeviceSize offset, VkDeviceSize &maxSize)
{
    uint8_t* readData = CheckAccess(offset, VK_WHOLE_SIZE);
    if (readData == nullptr) {
        assert(!"GetDataPtr() failed - buffer out of range!");
        return nullptr;
    }
    maxSize = m_memoryRequirements.size - offset;
    return (uint8_t*)readData;
}

const uint8_t* VulkanDeviceMemoryImpl::GetReadOnlyDataPtr(VkDeviceSize offset, VkDeviceSize &maxSize)
{
    const uint8_t* readData = CheckAccess(offset, 1);
    if (readData == nullptr) {
        assert(!"GetReadOnlyDataPtr() failed - buffer out of range!");
        return nullptr;
    }
    maxSize = m_memoryRequirements.size - offset;
    return readData;
}
