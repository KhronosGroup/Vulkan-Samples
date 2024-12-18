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

if(IOS AND CMAKE_VERSION VERSION_LESS 3.20)
    message(FATAL_ERROR "Configuring iOS apps requires a minimum CMake version of 3.20")
elseif(APPLE AND NOT IOS AND CMAKE_VERSION VERSION_LESS 3.17)
    message(FATAL_ERROR "Configuring Xcode for macOS requires a minimum CMake version of 3.17")
endif()

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
elseif(IOS)
	list(REMOVE_AT SRC 0)
	list(APPEND SRC
			${CMAKE_CURRENT_SOURCE_DIR}/ios/main.mm
			${CMAKE_CURRENT_SOURCE_DIR}/ios/AppDelegate.h
			${CMAKE_CURRENT_SOURCE_DIR}/ios/AppDelegate.m
			${CMAKE_CURRENT_SOURCE_DIR}/ios/ViewController.h
			${CMAKE_CURRENT_SOURCE_DIR}/ios/ViewController.mm
	)
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SRC})
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
    # Set VS startup project, working directory to project root, and command line arguments to default sample
    set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_COMMAND_ARGUMENTS "sample hello_triangle")

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
    if(NOT DEFINED MACOSX_BUNDLE_GUI_IDENTIFIER)
        if(${CMAKE_OSX_SYSROOT} STREQUAL "iphoneos" AND DEFINED CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM)
            message(FATAL_ERROR "Building for iOS device with a specified development team requires setting MACOSX_BUNDLE_GUI_IDENTIFIER")
		else()
			set(MACOSX_BUNDLE_GUI_IDENTIFIER com.khronos.vulkansamples)
		endif()
	endif()
    if(NOT DEFINED CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM)
		if(NOT DEFINED CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED)
			if(${CMAKE_OSX_SYSROOT} STREQUAL "iphoneos")
				set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "YES")
			else()
				set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO")
			endif()
		endif()
        if(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED)
            message(FATAL_ERROR "Building for iOS with code signing requires setting CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM; note it is safe to build for iOS Simulator without code signing")
        else()
            message(STATUS "Building for iOS device or iOS Simulator without a code signing identity; turning off code signing")
            set_target_properties(${PROJECT_NAME} PROPERTIES
                    XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED            "NO"
                    XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY		"NO"
            )
        endif ()
    else()
		message(STATUS "Building for iOS device or iOS Simulator with a code signing identity; turning on code signing")
        set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "YES")
        set_target_properties(${PROJECT_NAME} PROPERTIES
                XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED            "YES"
                XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY		"YES"
        )
    endif ()
    # No need to search for Vulkan package or MoltenVK library, Vulkan cache variables already defined on Apple platforms by global_options.cmake
    if(Vulkan_LIBRARY AND ${Vulkan_VERSION} VERSION_GREATER_EQUAL 1.3.278)
        target_sources(${PROJECT_NAME} PRIVATE
                ${Vulkan_Target_SDK}/iOS/share/vulkan
        )
        set_source_files_properties(
                ${Vulkan_Target_SDK}/iOS/share/vulkan
                PROPERTIES
                MACOSX_PACKAGE_LOCATION Resources
        )
    endif ()
    target_sources(${PROJECT_NAME} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../assets
            ${CMAKE_CURRENT_SOURCE_DIR}/../shaders
            ${CMAKE_CURRENT_SOURCE_DIR}/ios/launch.storyboard
            ${CMAKE_CURRENT_SOURCE_DIR}/ios/Storyboard.storyboard
            )
    set_source_files_properties(
            ${CMAKE_CURRENT_SOURCE_DIR}/ios/launch.storyboard
            ${CMAKE_CURRENT_SOURCE_DIR}/ios/Storyboard.storyboard
            ${CMAKE_CURRENT_SOURCE_DIR}/../assets
            ${CMAKE_CURRENT_SOURCE_DIR}/../shaders
            PROPERTIES
            MACOSX_PACKAGE_LOCATION Resources
    )
    set(FRAMEWORKS_TO_EMBED)
	if(Vulkan_MoltenVK_LIBRARY)
		list(APPEND FRAMEWORKS_TO_EMBED "${Vulkan_MoltenVK_LIBRARY};")
	else()
		message(FATAL_ERROR "Can't find MoltenVK library. Please install the Vulkan SDK or MoltenVK project and set VULKAN_SDK.")
	endif()
	if(Vulkan_LIBRARY)
		list(APPEND FRAMEWORKS_TO_EMBED "${Vulkan_LIBRARY};")
	endif()
	if(Vulkan_Layer_VALIDATION)
		# trouble is can't turn this on/off if XCode decides to build debug and we're configured for release.  Need to revist
		# note the Vulkan validation layer must be present and enabled even in release mode for the shader_debugprintf sample
		#if(("${VKB_DEBUG}" STREQUAL "ON") OR ("${VKB_VALIDATION_LAYERS}" STREQUAL "ON"))
			list(APPEND FRAMEWORKS_TO_EMBED "${Vulkan_Layer_VALIDATION}")
		#endif()
    endif()
    set_target_properties(${PROJECT_NAME} PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST ${PROJECT_SOURCE_DIR}/ios/Info.plist
            # These are already defined by the ios Info.plist file specified above
            #MACOSX_BUNDLE_SHORT_VERSION_STRING 1.0.0
            #MACOSX_BUNDLE_BUNDLE_VERSION 1.0.0
            #MACOSX_BUNDLE_BUNDLE_NAME "vulkan samples"
    )

    set_target_properties(${PROJECT_NAME} PROPERTIES
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER ${MACOSX_BUNDLE_GUI_IDENTIFIER}
            XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES "YES"
            XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/Frameworks"
            XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic" # already default value
            XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
            XCODE_EMBED_FRAMEWORKS  "${FRAMEWORKS_TO_EMBED}"
            XCODE_EMBED_FRAMEWORKS_REMOVE_HEADERS_ON_COPY	"YES"
            XCODE_ATTRIBUTE_SKIP_INSTALL  NO
            XCODE_ATTRIBUTE_INSTALL_PATH  "$(LOCAL_APPS_DIR)"
            XCODE_ATTRIBUTE_DEAD_CODE_STRIPPING NO
            XCODE_SCHEME_ARGUMENTS "sample hello_triangle"
    )
elseif(APPLE)
	# Set Xcode working directory to project root, and command line arguments to default sample
    set_target_properties(${PROJECT_NAME} PROPERTIES
            XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            XCODE_SCHEME_ARGUMENTS "sample hello_triangle"
	)
endif()
