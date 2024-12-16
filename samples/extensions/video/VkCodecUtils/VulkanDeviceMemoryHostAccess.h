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

#ifndef _VULKANDEVICEMEMORYHOSTACCESS_H_
#define _VULKANDEVICEMEMORYHOSTACCESS_H_

#include <cstdint>
#include "VkCodecUtils/VulkanDeviceContext.h"
#include "VkCodecUtils/Helpers.h"
#include "VkCodecUtils/VulkanDeviceMemoryImpl.h"

class VulkanDeviceMemoryHostAccess {
public:
    VulkanDeviceMemoryHostAccess(size_t offset, size_t size,
                                 VkSharedBaseObj<VulkanDeviceMemoryImpl>& vulkanDeviceMemory)
    : m_vulkanDeviceMemory(vulkanDeviceMemory),
      m_bufferDataPtr()
    {


    }
    ~VulkanDeviceMemoryHostAccess()
    {

    }
private:
    VkSharedBaseObj<VulkanDeviceMemoryImpl> m_vulkanDeviceMemory
    uint8_t*  m_bufferDataPtr;
};


#endif /* _VULKANDEVICEMEMORYHOSTACCESS_H_ */
