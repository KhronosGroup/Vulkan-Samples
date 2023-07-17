# Copyright (c) 2023-2024, Thomas Atkinson
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 the "License";
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

vkb__register_component(
    NAME core
    HEADERS
        include/core/platform/context.hpp
        include/core/platform/entrypoint.hpp

        include/core/util/strings.hpp
        include/core/util/error.hpp
        include/core/util/hash.hpp
        include/core/util/logging.hpp
        include/core/util/profiling.hpp
    SRC
        src/strings.cpp
        src/logging.cpp
        src/profiling.cpp
    LINK_LIBS
        spdlog::spdlog
        TracyClient
)

vkb__register_tests(
    COMPONENT core
    NAME utils
    SRC
        tests/strings.test.cpp
    LINK_LIBS
        vkb__core
)

if(ANDROID)
    target_compile_definitions(vkb__core PUBLIC VK_USE_PLATFORM_ANDROID_KHR PLATFORM__ANDROID)
elseif(WIN32)
    target_compile_definitions(vkb__core PUBLIC VK_USE_PLATFORM_WIN32_KHR PLATFORM__WINDOWS)
elseif(APPLE)
    target_compile_definitions(vkb__core PUBLIC VK_USE_PLATFORM_METAL_EXT PLATFORM__MACOS)
elseif(UNIX)
    target_compile_definitions(vkb__core PUBLIC PLATFORM__LINUX)

    # Choose WSI based on VKB_WSI_SELECTION
    if (VKB_WSI_SELECTION STREQUAL XCB OR VKB_WSI_SELECTION STREQUAL XLIB OR VKB_WSI_SELECTION STREQUAL WAYLAND)
        find_package(PkgConfig REQUIRED)
    endif()
    if (VKB_WSI_SELECTION STREQUAL XCB)
        pkg_check_modules(XCB xcb REQUIRED)
        if (XCB_FOUND)
            target_compile_definitions(vkb__core PUBLIC VK_USE_PLATFORM_XCB_KHR)
        endif()
    elseif (VKB_WSI_SELECTION STREQUAL XLIB)
        pkg_check_modules(X11 x11 REQUIRED)
        if (X11_FOUND)
            target_compile_definitions(vkb__core PUBLIC VK_USE_PLATFORM_XLIB_KHR)
        endif()
    elseif (VKB_WSI_SELECTION STREQUAL WAYLAND)
        pkg_check_modules(WAYLAND wayland-client REQUIRED)
        if (WAYLAND_FOUND)
            target_compile_definitions(vkb__core PUBLIC VK_USE_PLATFORM_WAYLAND_KHR)
        endif()
    elseif (VKB_WSI_SELECTION STREQUAL D2D)
        set(DIRECT_TO_DISPLAY TRUE)
        set(DIRECT_TO_DISPLAY TRUE PARENT_SCOPE)
        target_compile_definitions(vkb__core PUBLIC VK_USE_PLATFORM_DISPLAY_KHR PLATFORM__LINUX_D2D)
    else()
        message(FATAL_ERROR "Unknown WSI")
    endif()
endif()