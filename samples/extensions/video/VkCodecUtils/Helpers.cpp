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

#include "VkCodecUtils/Helpers.h"

using namespace vk;

NativeHandle::NativeHandle () :
#if defined(VK_USE_PLATFORM_ANDROID_KHR) || defined(VK_PLATFORM_IS_UNIX)
      m_fd(-1),
#endif // defined(VK_USE_PLATFORM_ANDROID_KHR) || defined(VK_PLATFORM_IS_UNIX)
#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
    m_androidHardwareBuffer(NULL),
#endif // defined(VK_ANDROID_external_memory_android_hardware_buffer)
    m_externalMemoryHandleType(VkExternalMemoryHandleTypeFlagBits(0))
{
}

NativeHandle::NativeHandle (const NativeHandle& other) :
#if defined(VK_USE_PLATFORM_ANDROID_KHR) || defined(VK_PLATFORM_IS_UNIX)
      m_fd(-1),
#endif // defined(VK_USE_PLATFORM_ANDROID_KHR) || defined(VK_PLATFORM_IS_UNIX)
#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
    m_androidHardwareBuffer(NULL),
#endif
    m_externalMemoryHandleType(VkExternalMemoryHandleTypeFlagBits(0))
{
#if defined(VK_USE_PLATFORM_ANDROID_KHR) || defined(VK_PLATFORM_IS_UNIX)
    if ((m_externalMemoryHandleType == VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT) && (other.m_fd >= 0)) {
        assert(m_fd >= 0);
    }
#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
    else if ((m_externalMemoryHandleType == VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID) && other.m_androidHardwareBuffer) {
        m_androidHardwareBuffer = other.m_androidHardwareBuffer;
        assert(m_androidHardwareBuffer);
    }
#endif //defined(VK_ANDROID_external_memory_android_hardware_buffer)
#endif // defined(VK_USE_PLATFORM_ANDROID_KHR) || defined(VK_PLATFORM_IS_UNIX)
}

#if defined(VK_USE_PLATFORM_ANDROID_KHR) || defined(VK_PLATFORM_IS_UNIX)
NativeHandle::NativeHandle (int fd) :
    m_fd                      (fd),
#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
    m_androidHardwareBuffer   (NULL),
#endif // defined(VK_ANDROID_external_memory_android_hardware_buffer)
    m_externalMemoryHandleType(VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT)
{
}
#endif // defined(VK_USE_PLATFORM_ANDROID_KHR) || defined(VK_PLATFORM_IS_UNIX)

#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
NativeHandle::NativeHandle (AHardwareBufferHandle buffer) :
    m_fd                      (-1),
    m_androidHardwareBuffer   (buffer),
    m_externalMemoryHandleType(VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID)
{
}
#endif // defined(VK_ANDROID_external_memory_android_hardware_buffer)

NativeHandle::~NativeHandle (void)
{
    // Release the object reference.
    releaseReference();
}

void NativeHandle::releaseReference (void)
{
#if defined(VK_USE_PLATFORM_ANDROID_KHR) || defined(VK_PLATFORM_IS_UNIX)
    if ((m_externalMemoryHandleType == VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT) && (m_fd >= 0)) {
        ::close(m_fd);
    }
#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
    if ((m_externalMemoryHandleType == VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID) && m_androidHardwareBuffer) {
        NvReleaseHardwareBufferHandle(m_androidHardwareBuffer);
    }
#endif // defined(VK_ANDROID_external_memory_android_hardware_buffer)
#endif // defined(VK_USE_PLATFORM_ANDROID_KHR) || defined(VK_PLATFORM_IS_UNIX)

    disown();
}

bool NativeHandle::isValid (void) const
{
#if defined(VK_USE_PLATFORM_ANDROID_KHR) || defined(VK_PLATFORM_IS_UNIX)
    if ((m_externalMemoryHandleType == VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT) && (m_fd >= 0)) {
        return true;
    }
#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
    if ((m_externalMemoryHandleType == VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID) && m_androidHardwareBuffer) {
        return true;
    }
#endif // defined(VK_ANDROID_external_memory_android_hardware_buffer)
#endif // defined(VK_USE_PLATFORM_ANDROID_KHR) || defined(VK_PLATFORM_IS_UNIX)

    return false;
}

#if defined(VK_USE_PLATFORM_ANDROID_KHR) || defined(VK_PLATFORM_IS_UNIX)
NativeHandle& NativeHandle::operator= (int fd)
{
    releaseReference();
    m_fd = fd;
    m_externalMemoryHandleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
    return *this;
}
#endif // defined(VK_USE_PLATFORM_ANDROID_KHR) || defined(VK_PLATFORM_IS_UNIX)

#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
NativeHandle& NativeHandle::operator= (AHardwareBufferHandle buffer)
{
    releaseReference();
    m_androidHardwareBuffer = buffer;
    m_externalMemoryHandleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID;
    return *this;
}
#endif // defined(VK_ANDROID_external_memory_android_hardware_buffer)

void NativeHandle::disown (void)
{
#if defined(VK_USE_PLATFORM_ANDROID_KHR) || defined(VK_PLATFORM_IS_UNIX)
    m_fd = -1;
#endif // defined(VK_USE_PLATFORM_ANDROID_KHR) || defined(VK_PLATFORM_IS_UNIX)
#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
    m_androidHardwareBuffer = AHardwareBufferHandle(NULL);
#endif // defined(VK_ANDROID_external_memory_android_hardware_buffer)
    m_externalMemoryHandleType = VkExternalMemoryHandleTypeFlagBits(0);
}

#if defined(VK_USE_PLATFORM_ANDROID_KHR) || defined(VK_PLATFORM_IS_UNIX)
int NativeHandle::getFd (void) const
{
    assert(m_externalMemoryHandleType == VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT);
    return m_fd;
}
#endif // defined(VK_USE_PLATFORM_ANDROID_KHR) || defined(VK_PLATFORM_IS_UNIX)

#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
AHardwareBufferHandle NativeHandle::getAndroidHardwareBuffer (void) const
{
    assert(m_fd == -1);
    assert(m_externalMemoryHandleType == VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID);
    return m_androidHardwareBuffer;
}
#endif // defined(VK_ANDROID_external_memory_android_hardware_buffer)

NativeHandle NativeHandle::InvalidNativeHandle = NativeHandle();
