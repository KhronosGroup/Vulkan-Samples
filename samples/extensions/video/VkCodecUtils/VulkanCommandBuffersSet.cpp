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

#include "VulkanCommandBuffersSet.h"

VkResult VulkanCommandBuffersSet::CreateCommandBufferPool(uint32_t queueFamilyIndex,
                                                          uint32_t maxCommandBuffersCount)
{
    DestroyCommandBuffer();
    DestroyCommandBufferPool();

     // -----------------------------------------------
     // Create a pool of command buffers to allocate command buffer from
     VkCommandPoolCreateInfo cmdPoolCreateInfo = VkCommandPoolCreateInfo();
     cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
     cmdPoolCreateInfo.pNext = nullptr;
     cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
     cmdPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
     VkResult result = VulkanDeviceContext::GetThe()->CreateCommandPool(VulkanDeviceContext::GetThe()->getDevice(), &cmdPoolCreateInfo, nullptr, &m_cmdPool);
     if (result != VK_SUCCESS) {
         assert(!"ERROR: Can't allocate command buffer pool");
         return result;
     }

     m_cmdBuffer.resize(maxCommandBuffersCount);
     VkCommandBufferAllocateInfo cmdBufferCreateInfo = VkCommandBufferAllocateInfo();
     cmdBufferCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
     cmdBufferCreateInfo.pNext = nullptr;
     cmdBufferCreateInfo.commandPool = m_cmdPool;
     cmdBufferCreateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
     cmdBufferCreateInfo.commandBufferCount = maxCommandBuffersCount;
     result = VulkanDeviceContext::GetThe()->AllocateCommandBuffers(VulkanDeviceContext::GetThe()->getDevice(), &cmdBufferCreateInfo, m_cmdBuffer.data());
     if (result != VK_SUCCESS) {
         assert(!"ERROR: Can't get command buffer");
     }
     return result;
}
