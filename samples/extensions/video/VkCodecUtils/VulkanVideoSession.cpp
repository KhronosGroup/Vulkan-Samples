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
#include <string.h>
#include "vk_video/vulkan_video_codecs_common.h"
#include "VkCodecUtils/Helpers.h"
#include "VkCodecUtils/HelpersDispatchTable.h"
#include "VkCodecUtils/VulkanDeviceContext.h"
#include "VkVideoCore/VkVideoCoreProfile.h"
#include "VkCodecUtils/VulkanVideoSession.h"

VkResult VulkanVideoSession::Create(VkVideoSessionCreateFlagsKHR sessionCreateFlags,
                                    uint32_t            videoQueueFamily,
                                    VkVideoCoreProfile* pVideoProfile,
                                    VkFormat            pictureFormat,
                                    const VkExtent2D&   maxCodedExtent,
                                    VkFormat            referencePicturesFormat,
                                    uint32_t            maxDpbSlots,
                                    uint32_t            maxActiveReferencePictures,
                                    VkSharedBaseObj<VulkanVideoSession>& videoSession)
{
    VulkanVideoSession* pNewVideoSession = new VulkanVideoSession(pVideoProfile);

    static const VkExtensionProperties h264DecodeStdExtensionVersion = { VK_STD_VULKAN_VIDEO_CODEC_H264_DECODE_EXTENSION_NAME, VK_STD_VULKAN_VIDEO_CODEC_H264_DECODE_SPEC_VERSION };
    static const VkExtensionProperties h265DecodeStdExtensionVersion = { VK_STD_VULKAN_VIDEO_CODEC_H265_DECODE_EXTENSION_NAME, VK_STD_VULKAN_VIDEO_CODEC_H265_DECODE_SPEC_VERSION };
    static const VkExtensionProperties av1DecodeStdExtensionVersion =  { VK_STD_VULKAN_VIDEO_CODEC_AV1_DECODE_EXTENSION_NAME, VK_STD_VULKAN_VIDEO_CODEC_AV1_DECODE_SPEC_VERSION };
    static const VkExtensionProperties h264EncodeStdExtensionVersion = { VK_STD_VULKAN_VIDEO_CODEC_H264_ENCODE_EXTENSION_NAME, VK_STD_VULKAN_VIDEO_CODEC_H264_ENCODE_SPEC_VERSION };
    static const VkExtensionProperties h265EncodeStdExtensionVersion = { VK_STD_VULKAN_VIDEO_CODEC_H265_ENCODE_EXTENSION_NAME, VK_STD_VULKAN_VIDEO_CODEC_H265_ENCODE_SPEC_VERSION };

    VkVideoSessionCreateInfoKHR& createInfo = pNewVideoSession->m_createInfo;
    createInfo.flags = sessionCreateFlags;
    createInfo.pVideoProfile = pVideoProfile->GetProfile();
    createInfo.queueFamilyIndex = videoQueueFamily;
    createInfo.pictureFormat = pictureFormat;
    createInfo.maxCodedExtent = maxCodedExtent;
    createInfo.maxDpbSlots = maxDpbSlots + 1;
    createInfo.maxActiveReferencePictures = maxActiveReferencePictures;
    createInfo.referencePictureFormat = referencePicturesFormat;

    switch ((int32_t)pVideoProfile->GetCodecType()) {
    case VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR:
        createInfo.pStdHeaderVersion = &h264DecodeStdExtensionVersion;
        break;
    case VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR:
        createInfo.pStdHeaderVersion = &h265DecodeStdExtensionVersion;
        break;
    case VK_VIDEO_CODEC_OPERATION_DECODE_AV1_BIT_KHR:
        createInfo.pStdHeaderVersion = &av1DecodeStdExtensionVersion;
        break;
    case VK_VIDEO_CODEC_OPERATION_ENCODE_H264_BIT_KHR:
        createInfo.pStdHeaderVersion = &h264EncodeStdExtensionVersion;
        break;
    case VK_VIDEO_CODEC_OPERATION_ENCODE_H265_BIT_KHR:
        createInfo.pStdHeaderVersion = &h265EncodeStdExtensionVersion;
        break;
    default:
        assert(0);
    }
    VkResult result = VulkanDeviceContext::GetThe()->CreateVideoSessionKHR(*VulkanDeviceContext::GetThe(), &createInfo, NULL, &pNewVideoSession->m_videoSession);
    if (result != VK_SUCCESS) {
        return result;
    }

    uint32_t videoSessionMemoryRequirementsCount = 0;
    VkVideoSessionMemoryRequirementsKHR decodeSessionMemoryRequirements[MAX_BOUND_MEMORY];
    // Get the count first
    result = VulkanDeviceContext::GetThe()->GetVideoSessionMemoryRequirementsKHR(*VulkanDeviceContext::GetThe(), pNewVideoSession->m_videoSession,
        &videoSessionMemoryRequirementsCount, NULL);
    assert(result == VK_SUCCESS);
    assert(videoSessionMemoryRequirementsCount <= MAX_BOUND_MEMORY);

    memset(decodeSessionMemoryRequirements, 0x00, sizeof(decodeSessionMemoryRequirements));
    for (uint32_t i = 0; i < videoSessionMemoryRequirementsCount; i++) {
        decodeSessionMemoryRequirements[i].sType = VK_STRUCTURE_TYPE_VIDEO_SESSION_MEMORY_REQUIREMENTS_KHR;
    }

    result = VulkanDeviceContext::GetThe()->GetVideoSessionMemoryRequirementsKHR(*VulkanDeviceContext::GetThe(), pNewVideoSession->m_videoSession,
                                                            &videoSessionMemoryRequirementsCount,
                                                            decodeSessionMemoryRequirements);
    if (result != VK_SUCCESS) {
        return result;
    }

    uint32_t decodeSessionBindMemoryCount = videoSessionMemoryRequirementsCount;
    VkBindVideoSessionMemoryInfoKHR decodeSessionBindMemory[MAX_BOUND_MEMORY];

    for (uint32_t memIdx = 0; memIdx < decodeSessionBindMemoryCount; memIdx++) {

        uint32_t memoryTypeIndex = 0;
        uint32_t memoryTypeBits = decodeSessionMemoryRequirements[memIdx].memoryRequirements.memoryTypeBits;
        if (memoryTypeBits == 0) {
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        // Find an available memory type that satisfies the requested properties.
        for (; !(memoryTypeBits & 1); memoryTypeIndex++  ) {
            memoryTypeBits >>= 1;
        }

        VkMemoryAllocateInfo memInfo = {
            VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,                          // sType
            NULL,                                                            // pNext
            decodeSessionMemoryRequirements[memIdx].memoryRequirements.size, // allocationSize
            memoryTypeIndex,                                                 // memoryTypeIndex
        };

        result = VulkanDeviceContext::GetThe()->AllocateMemory(*VulkanDeviceContext::GetThe(), &memInfo, 0,
                                                   &pNewVideoSession->m_memoryBound[memIdx]);
        if (result != VK_SUCCESS) {
            return result;
        }

        assert(result == VK_SUCCESS);
        decodeSessionBindMemory[memIdx].pNext = NULL;
        decodeSessionBindMemory[memIdx].sType = VK_STRUCTURE_TYPE_BIND_VIDEO_SESSION_MEMORY_INFO_KHR;
        decodeSessionBindMemory[memIdx].memory = pNewVideoSession->m_memoryBound[memIdx];

        decodeSessionBindMemory[memIdx].memoryBindIndex = decodeSessionMemoryRequirements[memIdx].memoryBindIndex;
        decodeSessionBindMemory[memIdx].memoryOffset = 0;
        decodeSessionBindMemory[memIdx].memorySize = decodeSessionMemoryRequirements[memIdx].memoryRequirements.size;
    }

    result = VulkanDeviceContext::GetThe()->BindVideoSessionMemoryKHR(*VulkanDeviceContext::GetThe(), pNewVideoSession->m_videoSession, decodeSessionBindMemoryCount,
                                                 decodeSessionBindMemory);
    assert(result == VK_SUCCESS);

    videoSession = pNewVideoSession;

    // Make sure we do not use dangling (on the stack) pointers
    createInfo.pNext = nullptr;

    return result;
}
