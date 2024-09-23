#[[
 Copyright (c) 2019-2024, Arm Limited and Contributors

 SPDX-License-Identifier: Apache-2.0

 Licensed under the Apache License, Version 2.0 the "License";
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

 ]]

if(NOT CMAKE_ANDROID_NDK)
    set(CMAKE_ANDROID_NDK ${ANDROID_NDK})
endif()

# Enable group projects in folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")

if(ANDROID)
    set(TARGET_ARCH ${CMAKE_ANDROID_ARCH_ABI})
else()
    set(TARGET_ARCH ${CMAKE_SYSTEM_PROCESSOR})
endif()

if(APPLE)
    cmake_minimum_required(VERSION 3.24)
    set(VKB_ENABLE_PORTABILITY ON CACHE BOOL "Enable portability enumeration and subset features in the framework.  This is required to be set when running on Apple platforms." FORCE)

	find_package(Vulkan OPTIONAL_COMPONENTS MoltenVK)
	if(USE_MoltenVK OR NOT Vulkan_FOUND OR (IOS AND (NOT Vulkan_MoltenVK_FOUND OR (${CMAKE_OSX_SYSROOT} STREQUAL "iphonesimulator" AND ${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "x86_64"))))
		# if Vulkan loader or MoltenVK for iOS was not found or using iOS Simulator on x86_64, look for MoltenVK in the Vulkan SDK, MoltenVK, and legacy MoltenVK locations
		if(NOT Vulkan_MoltenVK_LIBRARY)
			# since both are available in the Vulkan SDK and MoltenVK github project, make sure we look for MoltenVK framework on iOS and dylib on macOS
			set(_saved_cmake_find_framework ${CMAKE_FIND_FRAMEWORK})
			if(IOS)
				set(CMAKE_FIND_FRAMEWORK ALWAYS)
			else()
				set(CMAKE_FIND_FRAMEWORK NEVER)
			endif()
			find_library(Vulkan_MoltenVK_LIBRARY NAMES MoltenVK HINTS "$ENV{VULKAN_SDK}/lib" "$ENV{VULKAN_SDK}/dynamic" "$ENV{VULKAN_SDK}/dylib/macOS")
			set(CMAKE_FIND_FRAMEWORK ${_saved_cmake_find_framework})
			unset(_saved_cmake_find_framework)
		endif()

		if(Vulkan_MoltenVK_LIBRARY)
			# set up global Vulkan Library defines so that MoltenVK is dynamically loaded versus the Vulkan loader
			if(IOS)
				add_compile_definitions(_HPP_VULKAN_LIBRARY="MoltenVK.framework/MoltenVK")
				# unset FindVulkan.cmake cache variables so Vulkan loader, Validation Layer, and icd/layer json files will not be embedded on iOS
				unset(Vulkan_LIBRARY CACHE)
				unset(Vulkan_Layer_VALIDATION CACHE)

			# on macOS make sure VULKAN_SDK is defined, ensuring that MoltenVK_LIBRARY_PATH will point to the Vulkan SDK or MoltenVK project installation
			# otherwise if DYLD_LIBRARY_PATH points to the System Global installation, Volk may dynamically load libvulkan.dylib versus libMoltenVK.dylib
			elseif(DEFINED ENV{VULKAN_SDK})
				add_compile_definitions(_HPP_VULKAN_LIBRARY="libMoltenVK.dylib")
				add_compile_definitions(_GLFW_VULKAN_LIBRARY="libMoltenVK.dylib")
				get_filename_component(MoltenVK_LIBRARY_PATH ${Vulkan_MoltenVK_LIBRARY} DIRECTORY)
				set(ENV{DYLD_LIBRARY_PATH} "${MoltenVK_LIBRARY_PATH}:$ENV{DYLD_LIBRARY_PATH}")
			else()
				message(FATAL_ERROR "VULKAN_SDK is not defined. Please set VULKAN_SDK to the Vulkan SDK or MoltenVK project install location.")
			endif()
			message(STATUS "Using MoltenVK Vulkan Portability library at ${Vulkan_MoltenVK_LIBRARY}")
		else()
			message(FATAL_ERROR "Can't find MoltenVK library. Please install the Vulkan SDK or MoltenVK project and set VULKAN_SDK.")
		endif()
	endif()

	if(CMAKE_GENERATOR MATCHES "Xcode")
		set(CMAKE_XCODE_GENERATE_SCHEME ON)
		set(CMAKE_XCODE_SCHEME_ENABLE_GPU_API_VALIDATION OFF)

		if(NOT IOS)
			# If the Vulkan library's or loader's environment variables are defined, make them available within Xcode schemes
			if(DEFINED ENV{DYLD_LIBRARY_PATH})
				set(CMAKE_XCODE_SCHEME_ENVIRONMENT "${CMAKE_XCODE_SCHEME_ENVIRONMENT};DYLD_LIBRARY_PATH=$ENV{DYLD_LIBRARY_PATH}")
			endif()
			if(DEFINED ENV{VK_ADD_LAYER_PATH})
				set(CMAKE_XCODE_SCHEME_ENVIRONMENT "${CMAKE_XCODE_SCHEME_ENVIRONMENT};VK_ADD_LAYER_PATH=$ENV{VK_ADD_LAYER_PATH}")
			endif()
			if(DEFINED ENV{VK_ICD_FILENAMES})
				set(CMAKE_XCODE_SCHEME_ENVIRONMENT "${CMAKE_XCODE_SCHEME_ENVIRONMENT};VK_ICD_FILENAMES=$ENV{VK_ICD_FILENAMES}")
			endif()
			if(DEFINED ENV{VK_DRIVER_FILES})
				set(CMAKE_XCODE_SCHEME_ENVIRONMENT "${CMAKE_XCODE_SCHEME_ENVIRONMENT};VK_DRIVER_FILES=$ENV{VK_DRIVER_FILES}")
			endif()

			# Suppress regeneration for Xcode since environment variables will be lost if not set in Xcode locations/custom paths
			set(CMAKE_SUPPRESS_REGENERATION ON)
		endif()
	endif()
endif()

set(VKB_WARNINGS_AS_ERRORS ON CACHE BOOL "Enable Warnings as Errors")
set(VKB_VALIDATION_LAYERS OFF CACHE BOOL "Enable validation layers for every application.")
set(VKB_VALIDATION_LAYERS_GPU_ASSISTED OFF CACHE BOOL "Enable GPU assisted validation layers for every application (implicitly enables VKB_VALIDATION_LAYERS).")
set(VKB_VALIDATION_LAYERS_BEST_PRACTICES OFF CACHE BOOL "Enable best practices validation layers for every application (implicitly enables VKB_VALIDATION_LAYERS).")
set(VKB_VALIDATION_LAYERS_SYNCHRONIZATION OFF CACHE BOOL "Enable synchronization validation layers for every application (implicitly enables VKB_VALIDATION_LAYERS).")
set(VKB_VULKAN_DEBUG ON CACHE BOOL "Enable VK_EXT_debug_utils or VK_EXT_debug_marker if supported.")
set(VKB_BUILD_SAMPLES ON CACHE BOOL "Enable generation and building of Vulkan best practice samples.")
set(VKB_BUILD_TESTS OFF CACHE BOOL "Enable generation and building of Vulkan best practice tests.")
set(VKB_WSI_SELECTION "XCB" CACHE STRING "Select WSI target (XCB, XLIB, WAYLAND, D2D)")
set(VKB_CLANG_TIDY OFF CACHE STRING "Use CMake Clang Tidy integration")
set(VKB_CLANG_TIDY_EXTRAS "-header-filter=framework,samples,app;-checks=-*,google-*,-google-runtime-references;--fix;--fix-errors" CACHE STRING "Clang Tidy Parameters")
set(VKB_PROFILING OFF CACHE BOOL "Enable Tracy profiling")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/${CMAKE_BUILD_TYPE}/${TARGET_ARCH}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "lib/${CMAKE_BUILD_TYPE}/${TARGET_ARCH}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "lib/${CMAKE_BUILD_TYPE}/${TARGET_ARCH}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

string(LENGTH "${CMAKE_SOURCE_DIR}/" ROOT_PATH_SIZE)
add_definitions(-DROOT_PATH_SIZE=${ROOT_PATH_SIZE})

set(CMAKE_C_FLAGS_DEBUG   "-DDEBUG=0 ${CMAKE_C_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG=0 ${CMAKE_CXX_FLAGS_DEBUG}")

if (VKB_CLANG_TIDY)
    find_program(CLANG_TIDY "clang-tidy" "clang-tidy-15" REQUIRED)
    set(VKB_DO_CLANG_TIDY ${CLANG_TIDY} ${VKB_CLANG_TIDY_EXTRAS})
endif()

if (ANDROID AND VKB_PROFILING)
    message(WARNING "Tracy Profiling is not supported on android yet")
    set(VKB_PROFILING OFF)
endif()

set(TRACY_ENABLE ${VKB_PROFILING})
