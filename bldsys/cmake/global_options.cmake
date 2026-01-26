#[[
 Copyright (c) 2019-2025, Arm Limited and Contributors

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
    option(VKB_ENABLE_PORTABILITY "Enable portability enumeration and subset features in the framework. This is default ON for Apple platforms." ON)

	# the following assumes MoltenVK is used for these cases: a) standalone deployment on macOS (i.e. set USE_MoltenVK=ON), and b) for iOS deployment
	# if this assumption changes (e.g. KosmicKrisp adds standalone or iOS support), then this section will require modification to handle optionality
	find_package(Vulkan QUIET OPTIONAL_COMPONENTS MoltenVK)
	if(USE_MoltenVK OR (IOS AND (NOT Vulkan_MoltenVK_FOUND OR ${CMAKE_OSX_SYSROOT} STREQUAL "iphonesimulator")))
		# if using MoltenVK standalone, or MoltenVK for iOS not found or using iOS Simulator, look for MoltenVK in Vulkan SDK and MoltenVK project paths
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
			get_filename_component(MoltenVK_LIBRARY_PATH ${Vulkan_MoltenVK_LIBRARY} DIRECTORY)

			# For both iOS and macOS: set up global Vulkan Library defines so that MoltenVK is dynamically loaded versus the Vulkan loader
			# on iOS we can control Vulkan library loading priority by selecting which libraries are embedded in the iOS application bundle
			if(IOS)
				add_compile_definitions(_HPP_VULKAN_LIBRARY="MoltenVK.framework/MoltenVK")
				# unset FindVulkan.cmake cache variables so Vulkan loader and Validation Layer libraries are not embedded on iOS Simulator
				# the iOS Simulator supports arm64 & x86_64 hosts, but the Vulkan loader and Validation Layer are compiled for arm64 only
				unset(Vulkan_LIBRARY CACHE)
				unset(Vulkan_Layer_VALIDATION CACHE)

			# on macOS make sure that MoltenVK_LIBRARY_PATH points to the MoltenVK project installation and not to the Vulkan_LIBRARY location
			# otherwise if DYLD_LIBRARY_PATH points to a common search path, Volk may dynamically load libvulkan.dylib versus libMoltenVK.dylib
			elseif(NOT Vulkan_LIBRARY MATCHES "${MoltenVK_LIBRARY_PATH}")
				add_compile_definitions(_HPP_VULKAN_LIBRARY="libMoltenVK.dylib")
				add_compile_definitions(_GLFW_VULKAN_LIBRARY="libMoltenVK.dylib")
				set(ENV{DYLD_LIBRARY_PATH} "${MoltenVK_LIBRARY_PATH}:$ENV{DYLD_LIBRARY_PATH}")
			else()
				message(FATAL_ERROR "Vulkan loader found in MoltenVK search path. Please set VULKAN_SDK to the MoltenVK project install location.")
			endif()
			message(STATUS "Using MoltenVK standalone: ${Vulkan_MoltenVK_LIBRARY}")
		else()
			message(FATAL_ERROR "Can't find MoltenVK library. Please install the Vulkan SDK or MoltenVK project and set VULKAN_SDK.")
		endif()
	#elseif(OTHER_VULKAN_DRIVER)
		# handle any special processing here for other Vulkan driver (e.g. KosmicKrisp) for standalone usage on macOS or deployment to iOS
		# would likely require extensions to CMake find_package() OPTIONAL_COMPONENTS and library variables to identify & use other driver
	#else()
		# if not using standalone driver, retain find_package() results for Vulkan driver, Vulkan loader, and Validation Layer library variables
		# no need to override with _HPP_VULKAN_LIBRARY in this case since Vulkan DynamicLoader will find/load Vulkan library on macOS & iOS
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
else()
    option(VKB_ENABLE_PORTABILITY "Enable portability enumeration and subset features in the framework. This is default OFF for non-Apple platforms." OFF)
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
set(VKB_SKIP_SLANG_SHADER_COMPILATION OFF CACHE BOOL "Skips compilation for Slang shader")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/${CMAKE_BUILD_TYPE}/${TARGET_ARCH}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "lib/${CMAKE_BUILD_TYPE}/${TARGET_ARCH}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "lib/${CMAKE_BUILD_TYPE}/${TARGET_ARCH}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

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
