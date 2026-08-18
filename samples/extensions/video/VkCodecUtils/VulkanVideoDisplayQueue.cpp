/*
 * Copyright 2020 NVIDIA Corporation.
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

#include <cstdint>

#include "VkCodecUtils/VulkanDeviceContext.h"
#include "VkCodecUtils/VulkanVideoDisplayQueue.h"

#include "VkCodecUtils/VulkanVideoEncodeDisplayQueue.h"

VkResult CreateVulkanVideoEncodeDisplayQueue(const VulkanDeviceContext* vkDevCtx,
                                             int32_t defaultWidth, int32_t defaultHeight,
                                             int32_t defaultBitDepth, VkFormat defaultImageFormat,
                                             VkSharedBaseObj<VulkanVideoDisplayQueue<VulkanEncoderInputFrame>>& vulkanVideoEncodeDisplayQueue)
{
    VkSharedBaseObj<VulkanVideoDisplayQueue<VulkanEncoderInputFrame>> vulkanVideoDisplayQueue;
    VkResult result = VulkanVideoDisplayQueue<VulkanEncoderInputFrame>::Create(vkDevCtx,
                                                                               defaultWidth,
                                                                               defaultHeight,
                                                                               defaultBitDepth,
                                                                               defaultImageFormat,
                                                                               vulkanVideoDisplayQueue);
    if (result != VK_SUCCESS) {
        return result;
    }

    if (vulkanVideoDisplayQueue) {
        vulkanVideoEncodeDisplayQueue = vulkanVideoDisplayQueue;
        return VK_SUCCESS;
    }
    return VK_ERROR_OUT_OF_HOST_MEMORY;
}
