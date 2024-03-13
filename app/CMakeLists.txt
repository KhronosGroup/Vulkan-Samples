# Copyright (c) 2019-2024, Arm Limited and Contributors
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
#

cmake_minimum_required(VERSION 3.16)
include(android_package)

# create sample app project
project(vulkan_samples LANGUAGES C CXX)

add_subdirectory(plugins)
add_subdirectory(apps)

set(SRC
    main.cpp
)

source_group("\\" FILES ${SRC})

# select target type based on platform
if(ANDROID)
    if(CMAKE_VS_NsightTegra_VERSION)
        list(APPEND SRC ${CMAKE_CURRENT_SOURCE_DIR}/android/AndroidManifest.xml)
    endif()

    add_library(${PROJECT_NAME} SHARED ${SRC})
else()
    add_executable(${PROJECT_NAME} WIN32 ${SRC})
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE vkb__core vkb__filesystem apps plugins)

# Create android project
if(ANDROID)
    if(CMAKE_VS_NsightTegra_VERSION)
        set_property(TARGET ${PROJECT_NAME} PROPERTY ANDROID_GUI ON)
        set_property(TARGET ${PROJECT_NAME} PROPERTY ANDROID_ASSETS_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/../assets)
        set_property(TARGET ${PROJECT_NAME} PROPERTY ANDROID_JAVA_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../bldsys/android/java)
    endif()
    
    # Add packaging project only if not using CMake's toolchain
    if(CMAKE_SYSTEM_VERSION GREATER 1)
        add_android_package_project(
            NAME ${PROJECT_NAME}_package
            DEPENDS ${PROJECT_NAME}
            ASSET_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../assets
            JAVA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android/java
            RES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android/res
            MANIFEST_FILE ${CMAKE_CURRENT_SOURCE_DIR}/android/AndroidManifest.xml)
    endif()

    # Sync assets and shaders
    android_sync_folder(PATH ${CMAKE_CURRENT_SOURCE_DIR}/../assets)
    android_sync_folder(PATH ${CMAKE_CURRENT_SOURCE_DIR}/../shaders)
endif()

# Create MSVC project
if(MSVC)
    #Set the working directory to the source of the project so developer dont have to
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

    #Configure output paths
    foreach(CONFIG_TYPE ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${CONFIG_TYPE} SUFFIX)
        string(TOLOWER ${CONFIG_TYPE} CONFIG_DIR)
        set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${SUFFIX} ${CMAKE_CURRENT_BINARY_DIR}/bin/${CONFIG_DIR}/${TARGET_ARCH})
        set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${SUFFIX} ${CMAKE_CURRENT_BINARY_DIR}/lib/${CONFIG_DIR}/${TARGET_ARCH})
        set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_${SUFFIX} ${CMAKE_CURRENT_BINARY_DIR}/lib/${CONFIG_DIR}/${TARGET_ARCH})
    endforeach()
endif()

# Enable building from XCode for IOS
if(IOS)
    set(CMAKE_MACOSX_BUNDLE YES)
    set(MACOSX_BUNDLE_GUI_IDENTIFIER com.khronos.vulkan_samples)
    set_target_properties(${PROJECT_NAME} PROPERTIES
            BUNDLE_IDENTIFIER com.khronos.vulkan_samples
            XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO" # not CODE_SIGNING_REQUIRED anymore
            XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic" # already default value
            XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
            MACOSX_BUNDLE_SHORT_VERSION_STRING 1.0.0
            MACOSX_BUNDLE_BUNDLE_VERSION 1.0.0
            #            XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "SOME_VALID_ID"
            )
endif ()