# FindVulkanHpp.cmake
#
# Finds or downloads the Vulkan-Hpp headers and Vulkan Profiles headers
#
# This will define the following variables
#
#    VulkanHpp_FOUND
#    VulkanHpp_INCLUDE_DIRS
#
# and the following imported targets
#
#    VulkanHpp::VulkanHpp
#

# Try to find the package using standard find_path
find_path(VulkanHpp_INCLUDE_DIR
  NAMES vulkan/vulkan.hpp
  PATHS
    ${Vulkan_INCLUDE_DIR}
    /usr/include
    /usr/local/include
    $ENV{VULKAN_SDK}/include
    ${ANDROID_NDK}/sources/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../external
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../attachments/external
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../attachments/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../attachments/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../../external
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../../third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../../include
)

# Also try to find vulkan.cppm
find_path(VulkanHpp_CPPM_DIR
  NAMES vulkan/vulkan.cppm
  PATHS
    ${Vulkan_INCLUDE_DIR}
    /usr/include
    /usr/local/include
    $ENV{VULKAN_SDK}/include
    ${ANDROID_NDK}/sources/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../external
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../attachments/external
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../attachments/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../attachments/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../../external
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../../third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../../include
)

# Try to find vulkan_profiles.hpp
find_path(VulkanProfiles_INCLUDE_DIR
  NAMES vulkan/vulkan_profiles.hpp
  PATHS
    ${Vulkan_INCLUDE_DIR}
    /usr/include
    /usr/local/include
    $ENV{VULKAN_SDK}/include
    ${ANDROID_NDK}/sources/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../external
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../attachments/external
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../attachments/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../attachments/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../../external
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../../third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../../include
)

# Function to extract Vulkan version from vulkan_core.h
function(extract_vulkan_version VULKAN_CORE_H_PATH OUTPUT_VERSION_TAG)
  # Extract the version information from vulkan_core.h
  file(STRINGS ${VULKAN_CORE_H_PATH} VULKAN_VERSION_MAJOR_LINE REGEX "^#define VK_VERSION_MAJOR")
  file(STRINGS ${VULKAN_CORE_H_PATH} VULKAN_VERSION_MINOR_LINE REGEX "^#define VK_VERSION_MINOR")
  file(STRINGS ${VULKAN_CORE_H_PATH} VULKAN_HEADER_VERSION_LINE REGEX "^#define VK_HEADER_VERSION")

  set(VERSION_TAG "v1.3.275") # Default fallback

  if(VULKAN_VERSION_MAJOR_LINE AND VULKAN_VERSION_MINOR_LINE AND VULKAN_HEADER_VERSION_LINE)
    string(REGEX REPLACE "^#define VK_VERSION_MAJOR[ \t]+([0-9]+).*$" "\\1" VULKAN_VERSION_MAJOR "${VULKAN_VERSION_MAJOR_LINE}")
    string(REGEX REPLACE "^#define VK_VERSION_MINOR[ \t]+([0-9]+).*$" "\\1" VULKAN_VERSION_MINOR "${VULKAN_VERSION_MINOR_LINE}")
    string(REGEX REPLACE "^#define VK_HEADER_VERSION[ \t]+([0-9]+).*$" "\\1" VULKAN_HEADER_VERSION "${VULKAN_HEADER_VERSION_LINE}")

    # Construct the version tag
    set(VERSION_TAG "v${VULKAN_VERSION_MAJOR}.${VULKAN_VERSION_MINOR}.${VULKAN_HEADER_VERSION}")
  else()
    # Alternative approach: look for VK_HEADER_VERSION_COMPLETE
    file(STRINGS ${VULKAN_CORE_H_PATH} VULKAN_HEADER_VERSION_COMPLETE_LINE REGEX "^#define VK_HEADER_VERSION_COMPLETE")
    file(STRINGS ${VULKAN_CORE_H_PATH} VULKAN_HEADER_VERSION_LINE REGEX "^#define VK_HEADER_VERSION")

    if(VULKAN_HEADER_VERSION_COMPLETE_LINE AND VULKAN_HEADER_VERSION_LINE)
      # Extract the header version
      string(REGEX REPLACE "^#define VK_HEADER_VERSION[ \t]+([0-9]+).*$" "\\1" VULKAN_HEADER_VERSION "${VULKAN_HEADER_VERSION_LINE}")

      # Check if the complete version line contains the major and minor versions
      if(VULKAN_HEADER_VERSION_COMPLETE_LINE MATCHES "VK_MAKE_API_VERSION\\(.*,[ \t]*([0-9]+),[ \t]*([0-9]+),[ \t]*VK_HEADER_VERSION\\)")
        set(VULKAN_VERSION_MAJOR "${CMAKE_MATCH_1}")
        set(VULKAN_VERSION_MINOR "${CMAKE_MATCH_2}")
        set(VERSION_TAG "v${VULKAN_VERSION_MAJOR}.${VULKAN_VERSION_MINOR}.${VULKAN_HEADER_VERSION}")
      endif()
    endif()
  endif()

  # Return the version tag
  set(${OUTPUT_VERSION_TAG} ${VERSION_TAG} PARENT_SCOPE)
endfunction()

# Determine the Vulkan version to use for Vulkan-Hpp and Vulkan-Profiles
set(VULKAN_VERSION_TAG "v1.3.275") # Default version

# Try to detect the Vulkan version
set(VULKAN_CORE_H "")

# If we're building for Android, try to detect the NDK's Vulkan version
if(DEFINED ANDROID_NDK)
  # Find the vulkan_core.h file in the NDK
  find_file(VULKAN_CORE_H vulkan_core.h
    PATHS
      ${ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include/vulkan
      ${ANDROID_NDK}/toolchains/llvm/prebuilt/darwin-x86_64/sysroot/usr/include/vulkan
      ${ANDROID_NDK}/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include/vulkan
      ${ANDROID_NDK}/toolchains/llvm/prebuilt/windows/sysroot/usr/include/vulkan
    NO_DEFAULT_PATH
  )

  if(VULKAN_CORE_H)
    extract_vulkan_version(${VULKAN_CORE_H} VULKAN_VERSION_TAG)
    message(STATUS "Detected NDK Vulkan version: ${VULKAN_VERSION_TAG}")
  else()
    message(STATUS "Could not find vulkan_core.h in NDK, using default version: ${VULKAN_VERSION_TAG}")
  endif()
# For desktop builds, try to detect the Vulkan SDK version
elseif(DEFINED ENV{VULKAN_SDK})
  # Find the vulkan_core.h file in the Vulkan SDK
  find_file(VULKAN_CORE_H vulkan_core.h
    PATHS
      $ENV{VULKAN_SDK}/include/vulkan
    NO_DEFAULT_PATH
  )

  if(VULKAN_CORE_H)
    extract_vulkan_version(${VULKAN_CORE_H} VULKAN_VERSION_TAG)
    message(STATUS "Detected Vulkan SDK version: ${VULKAN_VERSION_TAG}")
  else()
    message(STATUS "Could not find vulkan_core.h in Vulkan SDK, using default version: ${VULKAN_VERSION_TAG}")
  endif()
# If Vulkan package was already found, try to use its include directory
elseif(DEFINED Vulkan_INCLUDE_DIR)
  # Find the vulkan_core.h file in the Vulkan include directory
  find_file(VULKAN_CORE_H vulkan_core.h
    PATHS
      ${Vulkan_INCLUDE_DIR}/vulkan
    NO_DEFAULT_PATH
  )

  if(VULKAN_CORE_H)
    extract_vulkan_version(${VULKAN_CORE_H} VULKAN_VERSION_TAG)
    message(STATUS "Detected Vulkan version from include directory: ${VULKAN_VERSION_TAG}")
  else()
    message(STATUS "Could not find vulkan_core.h in Vulkan include directory, using default version: ${VULKAN_VERSION_TAG}")
  endif()
else()
  # Try to find vulkan_core.h in system paths
  find_file(VULKAN_CORE_H vulkan_core.h
    PATHS
      /usr/include/vulkan
      /usr/local/include/vulkan
  )

  if(VULKAN_CORE_H)
    extract_vulkan_version(${VULKAN_CORE_H} VULKAN_VERSION_TAG)
    message(STATUS "Detected system Vulkan version: ${VULKAN_VERSION_TAG}")
  else()
    message(STATUS "Could not find vulkan_core.h in system paths, using default version: ${VULKAN_VERSION_TAG}")
  endif()
endif()

# If the include directory wasn't found, use FetchContent to download and build
if(NOT VulkanHpp_INCLUDE_DIR OR NOT VulkanHpp_CPPM_DIR)
  # If not found, use FetchContent to download
  include(FetchContent)

  message(STATUS "Vulkan-Hpp not found, fetching from GitHub with version ${VULKAN_VERSION_TAG}...")
  FetchContent_Declare(
    VulkanHpp
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Hpp.git
    GIT_TAG ${VULKAN_VERSION_TAG}  # Use the detected or default version
  )

  # Set policy to suppress the deprecation warning
  if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
  endif()

  # Make sure FetchContent is available
  include(FetchContent)

  # Populate the content
  FetchContent_GetProperties(VulkanHpp SOURCE_DIR VulkanHpp_SOURCE_DIR)
  if(NOT VulkanHpp_POPULATED)
    FetchContent_Populate(VulkanHpp)
    # Get the source directory after populating
    FetchContent_GetProperties(VulkanHpp SOURCE_DIR VulkanHpp_SOURCE_DIR)
  endif()

  # Set the include directory to the source directory
  set(VulkanHpp_INCLUDE_DIR ${VulkanHpp_SOURCE_DIR})
  message(STATUS "VulkanHpp_SOURCE_DIR: ${VulkanHpp_SOURCE_DIR}")
  message(STATUS "VulkanHpp_INCLUDE_DIR: ${VulkanHpp_INCLUDE_DIR}")

  # Check if vulkan.cppm exists in the downloaded repository
  if(EXISTS "${VulkanHpp_SOURCE_DIR}/vulkan/vulkan.cppm")
    set(VulkanHpp_CPPM_DIR ${VulkanHpp_SOURCE_DIR})
  else()
    # If vulkan.cppm doesn't exist, we need to create it
      set(VulkanHpp_CPPM_DIR ${CMAKE_CURRENT_BINARY_DIR}/VulkanHpp)
      file(MAKE_DIRECTORY ${VulkanHpp_CPPM_DIR}/vulkan)

      # Create vulkan.cppm file
      file(WRITE "${VulkanHpp_CPPM_DIR}/vulkan/vulkan.cppm"
"// Auto-generated vulkan.cppm file
module;
#include <vulkan/vulkan.hpp>
export module vulkan;
export namespace vk {
  using namespace VULKAN_HPP_NAMESPACE;
}
")
  endif()
endif()

# If the Vulkan Profiles include directory wasn't found, use FetchContent to download
if(NOT VulkanProfiles_INCLUDE_DIR)
  # If not found, use FetchContent to download
  include(FetchContent)

  message(STATUS "Vulkan-Profiles not found, fetching from GitHub main branch...")
  FetchContent_Declare(
    VulkanProfiles
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Profiles.git
    GIT_TAG main  # Use main branch instead of a specific tag
  )

  # Set policy to suppress the deprecation warning
  if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
  endif()

  # Populate the content
  FetchContent_GetProperties(VulkanProfiles SOURCE_DIR VulkanProfiles_SOURCE_DIR)
  if(NOT VulkanProfiles_POPULATED)
    FetchContent_Populate(VulkanProfiles)
    # Get the source directory after populating
    FetchContent_GetProperties(VulkanProfiles SOURCE_DIR VulkanProfiles_SOURCE_DIR)
  endif()

  # Create the include directory structure if it doesn't exist
  set(VulkanProfiles_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/VulkanProfiles/include)
  file(MAKE_DIRECTORY ${VulkanProfiles_INCLUDE_DIR}/vulkan)

  # Create a stub vulkan_profiles.hpp file if it doesn't exist
  if(NOT EXISTS "${VulkanProfiles_INCLUDE_DIR}/vulkan/vulkan_profiles.hpp")
    file(WRITE "${VulkanProfiles_INCLUDE_DIR}/vulkan/vulkan_profiles.hpp"
"// Auto-generated vulkan_profiles.hpp stub file
#pragma once
#include <vulkan/vulkan.hpp>

namespace vp {
    // Stub implementation for Vulkan Profiles
    struct ProfileDesc {
        const char* name;
        uint32_t specVersion;
    };

    inline bool GetProfileSupport(VkPhysicalDevice physicalDevice, const ProfileDesc* pProfile, VkBool32* pSupported) {
        *pSupported = VK_TRUE;
        return true;
    }
}
")
  endif()

  message(STATUS "VulkanProfiles_SOURCE_DIR: ${VulkanProfiles_SOURCE_DIR}")
  message(STATUS "VulkanProfiles_INCLUDE_DIR: ${VulkanProfiles_INCLUDE_DIR}")
endif()

# Set the variables
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VulkanHpp
  REQUIRED_VARS VulkanHpp_INCLUDE_DIR
  FAIL_MESSAGE "Could NOT find VulkanHpp. Install it or set VulkanHpp_INCLUDE_DIR to the directory containing vulkan/vulkan.hpp"
)

# Debug output
message(STATUS "VulkanHpp_FOUND: ${VulkanHpp_FOUND}")
message(STATUS "VULKANHPP_FOUND: ${VULKANHPP_FOUND}")

if(VulkanHpp_FOUND)
  set(VulkanHpp_INCLUDE_DIRS ${VulkanHpp_INCLUDE_DIR})

  # Make sure VulkanHpp_CPPM_DIR is set
  if(NOT DEFINED VulkanHpp_CPPM_DIR)
    # Check if vulkan.cppm exists in the include directory
    if(EXISTS "${VulkanHpp_INCLUDE_DIR}/vulkan/vulkan.cppm")
      set(VulkanHpp_CPPM_DIR ${VulkanHpp_INCLUDE_DIR})
      message(STATUS "Found vulkan.cppm in VulkanHpp_INCLUDE_DIR: ${VulkanHpp_CPPM_DIR}")
    elseif(DEFINED VulkanHpp_SOURCE_DIR AND EXISTS "${VulkanHpp_SOURCE_DIR}/vulkan/vulkan.cppm")
      set(VulkanHpp_CPPM_DIR ${VulkanHpp_SOURCE_DIR})
      message(STATUS "Found vulkan.cppm in VulkanHpp_SOURCE_DIR: ${VulkanHpp_CPPM_DIR}")
    elseif(DEFINED vulkanhpp_SOURCE_DIR AND EXISTS "${vulkanhpp_SOURCE_DIR}/vulkan/vulkan.cppm")
      set(VulkanHpp_CPPM_DIR ${vulkanhpp_SOURCE_DIR})
      message(STATUS "Found vulkan.cppm in vulkanhpp_SOURCE_DIR: ${VulkanHpp_CPPM_DIR}")
    else()
      # If vulkan.cppm doesn't exist, we need to create it
      set(VulkanHpp_CPPM_DIR ${CMAKE_CURRENT_BINARY_DIR}/VulkanHpp)
      file(MAKE_DIRECTORY ${VulkanHpp_CPPM_DIR}/vulkan)
      message(STATUS "Creating vulkan.cppm in ${VulkanHpp_CPPM_DIR}")

      # Create vulkan.cppm file
      file(WRITE "${VulkanHpp_CPPM_DIR}/vulkan/vulkan.cppm"
"// Auto-generated vulkan.cppm file
module;
#include <vulkan/vulkan.hpp>
export module vulkan;
export namespace vk {
  using namespace VULKAN_HPP_NAMESPACE;
}
")
    endif()
  endif()

  message(STATUS "Final VulkanHpp_CPPM_DIR: ${VulkanHpp_CPPM_DIR}")

  # Add Vulkan Profiles include directory if found
  if(VulkanProfiles_INCLUDE_DIR AND EXISTS "${VulkanProfiles_INCLUDE_DIR}/vulkan/vulkan_profiles.hpp")
    list(APPEND VulkanHpp_INCLUDE_DIRS ${VulkanProfiles_INCLUDE_DIR})
    message(STATUS "Added Vulkan Profiles include directory: ${VulkanProfiles_INCLUDE_DIR}")
  endif()

  # Create an imported target
  if(NOT TARGET VulkanHpp::VulkanHpp)
    add_library(VulkanHpp::VulkanHpp INTERFACE IMPORTED)
    set_target_properties(VulkanHpp::VulkanHpp PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${VulkanHpp_INCLUDE_DIRS}"
    )
  endif()
elseif(DEFINED VulkanHpp_SOURCE_DIR OR DEFINED vulkanhpp_SOURCE_DIR)
  # If find_package_handle_standard_args failed but we have a VulkanHpp source directory from FetchContent
  # Create an imported target
  if(NOT TARGET VulkanHpp::VulkanHpp)
    add_library(VulkanHpp::VulkanHpp INTERFACE IMPORTED)

    # Determine the source directory
    if(DEFINED VulkanHpp_SOURCE_DIR)
      set(_vulkanhpp_source_dir ${VulkanHpp_SOURCE_DIR})
    elseif(DEFINED vulkanhpp_SOURCE_DIR)
      set(_vulkanhpp_source_dir ${vulkanhpp_SOURCE_DIR})
    endif()

    message(STATUS "Using fallback VulkanHpp source directory: ${_vulkanhpp_source_dir}")

    set_target_properties(VulkanHpp::VulkanHpp PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${_vulkanhpp_source_dir}"
    )
  endif()

  # Set variables to indicate that VulkanHpp was found
  set(VulkanHpp_FOUND TRUE)
  set(VULKANHPP_FOUND TRUE)

  # Set include directories
  if(DEFINED _vulkanhpp_source_dir)
    set(VulkanHpp_INCLUDE_DIR ${_vulkanhpp_source_dir})
  elseif(DEFINED VulkanHpp_SOURCE_DIR)
    set(VulkanHpp_INCLUDE_DIR ${VulkanHpp_SOURCE_DIR})
  elseif(DEFINED vulkanhpp_SOURCE_DIR)
    set(VulkanHpp_INCLUDE_DIR ${vulkanhpp_SOURCE_DIR})
  endif()
  set(VulkanHpp_INCLUDE_DIRS ${VulkanHpp_INCLUDE_DIR})

  # Add Vulkan Profiles include directory if found
  if(VulkanProfiles_INCLUDE_DIR AND EXISTS "${VulkanProfiles_INCLUDE_DIR}/vulkan/vulkan_profiles.hpp")
    list(APPEND VulkanHpp_INCLUDE_DIRS ${VulkanProfiles_INCLUDE_DIR})
    message(STATUS "Added Vulkan Profiles include directory to fallback: ${VulkanProfiles_INCLUDE_DIR}")
  endif()

  # Make sure VulkanHpp_CPPM_DIR is set
  if(NOT DEFINED VulkanHpp_CPPM_DIR)
    # Check if vulkan.cppm exists in the downloaded repository
    if(DEFINED VulkanHpp_INCLUDE_DIR AND EXISTS "${VulkanHpp_INCLUDE_DIR}/vulkan/vulkan.cppm")
      set(VulkanHpp_CPPM_DIR ${VulkanHpp_INCLUDE_DIR})
      message(STATUS "Found vulkan.cppm in VulkanHpp_INCLUDE_DIR: ${VulkanHpp_CPPM_DIR}")
    elseif(DEFINED _vulkanhpp_source_dir AND EXISTS "${_vulkanhpp_source_dir}/vulkan/vulkan.cppm")
      set(VulkanHpp_CPPM_DIR ${_vulkanhpp_source_dir})
      message(STATUS "Found vulkan.cppm in _vulkanhpp_source_dir: ${VulkanHpp_CPPM_DIR}")
    elseif(DEFINED VulkanHpp_SOURCE_DIR AND EXISTS "${VulkanHpp_SOURCE_DIR}/vulkan/vulkan.cppm")
      set(VulkanHpp_CPPM_DIR ${VulkanHpp_SOURCE_DIR})
      message(STATUS "Found vulkan.cppm in VulkanHpp_SOURCE_DIR: ${VulkanHpp_CPPM_DIR}")
    elseif(DEFINED vulkanhpp_SOURCE_DIR AND EXISTS "${vulkanhpp_SOURCE_DIR}/vulkan/vulkan.cppm")
      set(VulkanHpp_CPPM_DIR ${vulkanhpp_SOURCE_DIR})
      message(STATUS "Found vulkan.cppm in vulkanhpp_SOURCE_DIR: ${VulkanHpp_CPPM_DIR}")
    else()
      # If vulkan.cppm doesn't exist, we need to create it
      set(VulkanHpp_CPPM_DIR ${CMAKE_CURRENT_BINARY_DIR}/VulkanHpp)
      file(MAKE_DIRECTORY ${VulkanHpp_CPPM_DIR}/vulkan)
      message(STATUS "Creating vulkan.cppm in ${VulkanHpp_CPPM_DIR}")

      # Create vulkan.cppm file
      file(WRITE "${VulkanHpp_CPPM_DIR}/vulkan/vulkan.cppm"
"// Auto-generated vulkan.cppm file
module;
#include <vulkan/vulkan.hpp>
export module vulkan;
export namespace vk {
  using namespace VULKAN_HPP_NAMESPACE;
}
")
    endif()
  endif()

  message(STATUS "Final VulkanHpp_CPPM_DIR: ${VulkanHpp_CPPM_DIR}")
endif()

mark_as_advanced(VulkanHpp_INCLUDE_DIR VulkanHpp_CPPM_DIR)
