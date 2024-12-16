/*
 * Copyright (C) 2016 Google, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef HELPERS_H
#define HELPERS_H

#include "HelpersDispatchTable.h"
#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <VulkanDeviceContext.h>

namespace vk {

struct Vertex {
    float position[2];
    float texCoord[2];
};

struct Vec2 {
    Vec2(float val0, float val1)
        : val{val0, val1} {}
    float val[2];
};

struct Vec4 {
    Vec4(float val0, float val1, float val2, float val3)
        : val{val0, val1, val2, val3} {}
    float val[4];
};

struct TransformPushConstants {
    TransformPushConstants()
        : posMatrix {{1.0f, 0.0f, 0.0f, 0.0f},
                     {0.0f, 1.0f, 0.0f, 0.0f},
                     {0.0f, 0.0f, 1.0f, 0.0f},
                     {0.0f, 0.0f, 0.0f, 1.0f}}
          , texMatrix {{1.0f, 0.0f},
                       {0.0f, 1.0f}}
    {
    }
    Vec4 posMatrix[4];
    Vec2 texMatrix[2];
};

template <class valueType, class alignmentType>
valueType alignedSize(valueType value, alignmentType alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

#if defined(VK_USE_PLATFORM_XCB_KHR) || defined (VK_USE_PLATFORM_XLIB_KHR) || defined(VK_USE_PLATFORM_WAYLAND_KHR)
#define VK_PLATFORM_IS_UNIX 1
#endif

class NativeHandle {
public:
    static NativeHandle InvalidNativeHandle;

    NativeHandle();
    NativeHandle(const NativeHandle& other);
#if defined(VK_USE_PLATFORM_ANDROID_KHR) || defined(VK_PLATFORM_IS_UNIX)
    NativeHandle(int fd);
#endif // defined(VK_USE_PLATFORM_ANDROID_KHR) || defined(VK_PLATFORM_IS_UNIX)
#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
    NativeHandle (AHardwareBufferHandle buffer);
#endif // defined(VK_ANDROID_external_memory_android_hardware_buffer)
    ~NativeHandle ();

#if defined(VK_USE_PLATFORM_ANDROID_KHR) || defined(VK_PLATFORM_IS_UNIX)
    NativeHandle& operator= (int fd);
#endif // defined(VK_USE_PLATFORM_ANDROID_KHR) || defined(VK_PLATFORM_IS_UNIX)
#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
    NativeHandle& operator= (AHardwareBufferHandle buffer);
#endif // defined(VK_ANDROID_external_memory_android_hardware_buffer)
#if defined(VK_USE_PLATFORM_ANDROID_KHR) || defined(VK_PLATFORM_IS_UNIX)
    int getFd(void) const;
    operator int() const { return getFd(); }
#endif // defined(VK_USE_PLATFORM_ANDROID_KHR) || defined(VK_PLATFORM_IS_UNIX)
#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
    AHardwareBufferHandle getAndroidHardwareBuffer(void) const;
    operator AHardwareBufferHandle() const { return getAndroidHardwareBuffer(); }
#endif // defined(VK_ANDROID_external_memory_android_hardware_buffer)
    VkExternalMemoryHandleTypeFlagBits getExternalMemoryHandleType () const
    {
        return m_externalMemoryHandleType;
    }
    void disown();
    bool isValid() const;
    operator bool() const { return isValid(); }
    // This should only be called on an import error or on handle replacement.
    void releaseReference();

private:
#if defined(VK_USE_PLATFORM_ANDROID_KHR) || defined(VK_PLATFORM_IS_UNIX)
    int                                 m_fd;
#endif // defined(VK_USE_PLATFORM_ANDROID_KHR) || defined(VK_PLATFORM_IS_UNIX)
#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
    AHardwareBufferHandle               m_androidHardwareBuffer;
#endif // defined(VK_ANDROID_external_memory_android_hardware_buffer)
    VkExternalMemoryHandleTypeFlagBits  m_externalMemoryHandleType;

    // Disabled
    NativeHandle& operator= (const NativeHandle&) = delete;
};


inline VkResult enumerate(const VkInterfaceFunctions* vkIf, const char *layer, std::vector<VkExtensionProperties> &exts) {
    uint32_t count = 0;
    vkIf->EnumerateInstanceExtensionProperties(layer, &count, nullptr);

    exts.resize(count);
    return vkIf->EnumerateInstanceExtensionProperties(layer, &count, exts.data());
}

inline VkResult enumerate(const VkInterfaceFunctions* vkIf, VkPhysicalDevice phy, const char *layer, std::vector<VkExtensionProperties> &exts) {
    uint32_t count = 0;
    vkIf->EnumerateDeviceExtensionProperties(phy, layer, &count, nullptr);

    exts.resize(count);
    return vkIf->EnumerateDeviceExtensionProperties(phy, layer, &count, exts.data());
}

inline VkResult enumerate(const VkInterfaceFunctions* vkIf, VkInstance instance, std::vector<VkPhysicalDevice> &phys) {
    uint32_t count = 0;
    vkIf->EnumeratePhysicalDevices(instance, &count, nullptr);

    phys.resize(count);
    return vkIf->EnumeratePhysicalDevices(instance, &count, phys.data());
}

inline VkResult enumerate(const VkInterfaceFunctions* vkIf, std::vector<VkLayerProperties> &layer_props) {
    uint32_t count = 0;
    vkIf->EnumerateInstanceLayerProperties(&count, nullptr);

    layer_props.resize(count);
    return vkIf->EnumerateInstanceLayerProperties(&count, layer_props.data());
}

inline VkResult get(const VkInterfaceFunctions* vkIf,
                    VkPhysicalDevice phy, std::vector<VkQueueFamilyProperties2> &queues,
                    std::vector<VkQueueFamilyVideoPropertiesKHR> &videoQueues,
                    std::vector<VkQueueFamilyQueryResultStatusPropertiesKHR> &queryResultStatus) {
    uint32_t count = 0;
    vkIf->GetPhysicalDeviceQueueFamilyProperties2(phy, &count, nullptr);

    queues.resize(count);
    videoQueues.resize(count);
    queryResultStatus.resize(count);
    for (uint32_t i = 0; i < queues.size(); i++) {
        queues[i].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
        videoQueues[i].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_VIDEO_PROPERTIES_KHR;
        queues[i].pNext = &videoQueues[i];
        queryResultStatus[i].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_QUERY_RESULT_STATUS_PROPERTIES_KHR;
        videoQueues[i].pNext = &queryResultStatus[i];
    }

    vkIf->GetPhysicalDeviceQueueFamilyProperties2(phy, &count, queues.data());

    return VK_SUCCESS;
}

inline VkResult get(const VkInterfaceFunctions* vkIf,
                    VkPhysicalDevice phy, VkSurfaceKHR surface, std::vector<VkSurfaceFormatKHR> &formats) {
    uint32_t count = 0;
    vkIf->GetPhysicalDeviceSurfaceFormatsKHR(phy, surface, &count, nullptr);

    formats.resize(count);
    return vkIf->GetPhysicalDeviceSurfaceFormatsKHR(phy, surface, &count, formats.data());
}

inline VkResult get(const VkInterfaceFunctions* vkIf,
                    VkPhysicalDevice phy, VkSurfaceKHR surface, std::vector<VkPresentModeKHR> &modes) {
    uint32_t count = 0;
    vkIf->GetPhysicalDeviceSurfacePresentModesKHR(phy, surface, &count, nullptr);

    modes.resize(count);
    return vkIf->GetPhysicalDeviceSurfacePresentModesKHR(phy, surface, &count, modes.data());
}

inline VkResult get(const VkInterfaceFunctions* vkIf,
                    VkDevice dev, VkSwapchainKHR swapchain, std::vector<VkImage> &images) {
    uint32_t count = 0;
    vkIf->GetSwapchainImagesKHR(dev, swapchain, &count, nullptr);

    images.resize(count);
    return vkIf->GetSwapchainImagesKHR(dev, swapchain, &count, images.data());
}

inline VkResult MapMemoryTypeToIndex(VkPhysicalDevice vkPhysicalDev,
                                     uint32_t typeBits,
                                     VkFlags requirements_mask, uint32_t *typeIndex)
{
    VkPhysicalDeviceMemoryProperties memoryProperties;
    VulkanDeviceContext::GetThe()->GetPhysicalDeviceMemoryProperties(vkPhysicalDev, &memoryProperties);
    // Search memtypes to find first index with those properties
    for (uint32_t i = 0; i < 32; i++) {
        if ((typeBits & 1) == 1) {
            // Type is available, does it match user properties?
            if ((memoryProperties.memoryTypes[i].propertyFlags & requirements_mask) ==
                    requirements_mask) {
                *typeIndex = i;
                return VK_SUCCESS;
            }
        }
        typeBits >>= 1;
    }
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

inline VkResult WaitAndResetFence(VkDevice device, VkFence fence,
                                  bool resetAfterWait = true, const char* fenceName = "unknown",
                                  const uint64_t fenceWaitTimeout = 100ULL * 1000ULL * 1000ULL /* 100 mSec */,
                                  const uint64_t fenceTotalWaitTimeout = 5ULL * 1000ULL * 1000ULL * 1000ULL /* 5 sec */) {

    assert(device != VK_NULL_HANDLE);
    assert(fence != VK_NULL_HANDLE);

    uint64_t fenceCurrentWaitTimeout = 0;

    VkResult result = VK_SUCCESS;

    while (fenceTotalWaitTimeout >= fenceCurrentWaitTimeout) {

        result = VulkanDeviceContext::GetThe()->WaitForFences(device, 1, &fence, true, fenceWaitTimeout);
        if (result != VK_SUCCESS) {
            fprintf(stderr, "\t **** WARNING: fence  %s(%llu) is not done after %llu nSec with result 0x%x ****\n",
                            fenceName, (long long unsigned int)fence, (long long unsigned int)fenceWaitTimeout, result);
            assert(!"Fence is not signaled yet after more than 100 mSec wait");
        }

        if (result != VK_TIMEOUT) {
            break;
        }

        fenceCurrentWaitTimeout += fenceWaitTimeout;
    }

    if (result != VK_SUCCESS) {
        fprintf(stderr, "\t **** ERROR: fence  %s(%llu) is not done after %llu nSec with result 0x%x ****\n",
                        fenceName, (long long unsigned int)fence, (long long unsigned int)fenceTotalWaitTimeout, VulkanDeviceContext::GetThe()->GetFenceStatus(device, fence));
        assert(!"Fence is not signaled yet after more than 100 mSec wait");
    }

    if (resetAfterWait) {
        result = VulkanDeviceContext::GetThe()->ResetFences(device, 1, &fence);
        if (result != VK_SUCCESS) {
            fprintf(stderr, "\nERROR: ResetFences() result: 0x%x\n", result);
            assert(result == VK_SUCCESS);
        }

        assert(VulkanDeviceContext::GetThe()->GetFenceStatus(device, fence) == VK_NOT_READY);
    }
    return result;
}

inline VkResult WaitAndGetStatus(VkDevice device, VkFence fence,
                                 VkQueryPool queryPool, int32_t startQueryId, uint32_t pictureIndex,
                                  bool resetAfterWait = true, const char* fenceName = "unknown",
                                  const uint64_t fenceWaitTimeout = 100ULL * 1000ULL * 1000ULL /* 100 mSec */,
                                  const uint64_t fenceTotalWaitTimeout = 5ULL * 1000ULL * 1000ULL * 1000ULL /* 5 sec */,
                                  uint32_t retryCount = 6 /* 30s */) {

    VkResult result;

    do {
        result = WaitAndResetFence(device, fence, resetAfterWait, fenceName, fenceWaitTimeout, fenceTotalWaitTimeout);
        if (result != VK_SUCCESS) {
            std::cout << "WaitForFences timeout " << fenceWaitTimeout
                    << " result " << result << " retry " << retryCount << std::endl << std::flush;

            VkQueryResultStatusKHR decodeStatus = VK_QUERY_RESULT_STATUS_NOT_READY_KHR;
            VkResult queryResult = VulkanDeviceContext::GetThe()->GetQueryPoolResults(device,
                                                     queryPool,
                                                     startQueryId,
                                                     1,
                                                     sizeof(decodeStatus),
                                                     &decodeStatus,
                                                     sizeof(decodeStatus),
                                                     VK_QUERY_RESULT_WITH_STATUS_BIT_KHR);

            printf("\nERROR: GetQueryPoolResults() result: 0x%x\n", queryResult);
            std::cout << "\t +++++++++++++++++++++++++++< " << pictureIndex
                    << " >++++++++++++++++++++++++++++++" << std::endl;
            std::cout << "\t => Decode Status for CurrPicIdx: " << pictureIndex << std::endl
                    << "\t\tdecodeStatus: " << decodeStatus << std::endl;

            if (queryResult == VK_ERROR_DEVICE_LOST) {
                std::cout << "\t Dropping frame" << std::endl;
                break;
            }

            if ((queryResult == VK_SUCCESS) && (decodeStatus == VK_QUERY_RESULT_STATUS_ERROR_KHR)) {
                std::cout << "\t Decoding of the frame failed." << std::endl;
                break;
            }
        }

        retryCount--;
    } while ((result == VK_TIMEOUT) && (retryCount > 0));

    return result;
 }

}  // namespace vk

#endif  // HELPERS_H
