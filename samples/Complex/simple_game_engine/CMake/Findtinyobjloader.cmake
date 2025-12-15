# Findtinyobjloader.cmake
# Find the tinyobjloader library
#
# This module defines the following variables:
#   tinyobjloader_FOUND        - True if tinyobjloader was found
#   tinyobjloader_INCLUDE_DIRS - Include directories for tinyobjloader
#   tinyobjloader_LIBRARIES    - Libraries to link against tinyobjloader
#
# It also defines the following targets:
#   tinyobjloader::tinyobjloader

# Try to find the package using pkg-config first
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_tinyobjloader QUIET tinyobjloader)
endif()

# Find the include directory
find_path(tinyobjloader_INCLUDE_DIR
  NAMES tiny_obj_loader.h
  PATHS
    ${PC_tinyobjloader_INCLUDE_DIRS}
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
  PATH_SUFFIXES tinyobjloader tiny_obj_loader
)

# Find the library
find_library(tinyobjloader_LIBRARY
  NAMES tinyobjloader
  PATHS
    ${PC_tinyobjloader_LIBRARY_DIRS}
    /usr/lib
    /usr/local/lib
    $ENV{VULKAN_SDK}/lib
    ${ANDROID_NDK}/sources/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../external
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../attachments/external
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../attachments/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../attachments/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../../external
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../../third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../../lib
  PATH_SUFFIXES lib
)

# If the include directory wasn't found, use FetchContent to download and build
if(NOT tinyobjloader_INCLUDE_DIR)
  # If not found, use FetchContent to download and build
  include(FetchContent)

  message(STATUS "tinyobjloader not found, fetching from GitHub...")
  FetchContent_Declare(
    tinyobjloader
    GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
    GIT_TAG v2.0.0rc10  # Use a specific tag for stability
  )

  # Set options before making tinyobjloader available
  set(TINYOBJLOADER_BUILD_TEST_LOADER OFF CACHE BOOL "Do not build test loader" FORCE)
  set(TINYOBJLOADER_BUILD_OBJ_STICHER OFF CACHE BOOL "Do not build obj sticher" FORCE)
  set(TINYOBJLOADER_INSTALL OFF CACHE BOOL "Do not install tinyobjloader" FORCE)

  # Update CMake policy to suppress the deprecation warning
  if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
  endif()

  # Populate the content but don't configure it yet
  FetchContent_GetProperties(tinyobjloader)
  if(NOT tinyobjloader_POPULATED)
    FetchContent_Populate(tinyobjloader)

    # Update the minimum required CMake version before including the CMakeLists.txt
    file(READ "${tinyobjloader_SOURCE_DIR}/CMakeLists.txt" TINYOBJLOADER_CMAKE_CONTENT)
    string(REPLACE "cmake_minimum_required(VERSION 3.2)"
                   "cmake_minimum_required(VERSION 3.10)"
                   TINYOBJLOADER_CMAKE_CONTENT "${TINYOBJLOADER_CMAKE_CONTENT}")
    string(REPLACE "cmake_minimum_required(VERSION 3.5)"
                   "cmake_minimum_required(VERSION 3.10)"
                   TINYOBJLOADER_CMAKE_CONTENT "${TINYOBJLOADER_CMAKE_CONTENT}")
    file(WRITE "${tinyobjloader_SOURCE_DIR}/CMakeLists.txt" "${TINYOBJLOADER_CMAKE_CONTENT}")

    # Now add the subdirectory manually
    add_subdirectory(${tinyobjloader_SOURCE_DIR} ${tinyobjloader_BINARY_DIR})
  else()
    # If already populated, just make it available
    FetchContent_MakeAvailable(tinyobjloader)
  endif()

  # Get the include directory from the target
  get_target_property(tinyobjloader_INCLUDE_DIR tinyobjloader INTERFACE_INCLUDE_DIRECTORIES)
  if(NOT tinyobjloader_INCLUDE_DIR)
    # If we can't get the include directory from the target, use the source directory
    set(tinyobjloader_INCLUDE_DIR ${tinyobjloader_SOURCE_DIR})
  endif()
endif()

# Set the variables
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(tinyobjloader 
  REQUIRED_VARS tinyobjloader_INCLUDE_DIR
)

if(tinyobjloader_FOUND)
  set(tinyobjloader_INCLUDE_DIRS ${tinyobjloader_INCLUDE_DIR})

  if(tinyobjloader_LIBRARY)
    set(tinyobjloader_LIBRARIES ${tinyobjloader_LIBRARY})
  else()
    # tinyobjloader is a header-only library, so no library is needed
    set(tinyobjloader_LIBRARIES "")
  endif()

  # Create an imported target
  if(NOT TARGET tinyobjloader::tinyobjloader)
    add_library(tinyobjloader::tinyobjloader INTERFACE IMPORTED)
    set_target_properties(tinyobjloader::tinyobjloader PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${tinyobjloader_INCLUDE_DIRS}"
    )
    if(tinyobjloader_LIBRARIES)
      set_target_properties(tinyobjloader::tinyobjloader PROPERTIES
        INTERFACE_LINK_LIBRARIES "${tinyobjloader_LIBRARIES}"
      )
    endif()
  endif()
elseif(TARGET tinyobjloader)
  # If find_package_handle_standard_args failed but we have a tinyobjloader target from FetchContent
  # Create an alias for the tinyobjloader target
  if(NOT TARGET tinyobjloader::tinyobjloader)
    add_library(tinyobjloader::tinyobjloader ALIAS tinyobjloader)
  endif()

  # Set variables to indicate that tinyobjloader was found
  set(tinyobjloader_FOUND TRUE)
  set(TINYOBJLOADER_FOUND TRUE)

  # Set include directories
  get_target_property(tinyobjloader_INCLUDE_DIR tinyobjloader INTERFACE_INCLUDE_DIRECTORIES)
  if(tinyobjloader_INCLUDE_DIR)
    set(tinyobjloader_INCLUDE_DIRS ${tinyobjloader_INCLUDE_DIR})
  else()
    # If we can't get the include directory from the target, use the source directory
    set(tinyobjloader_INCLUDE_DIR ${tinyobjloader_SOURCE_DIR})
    set(tinyobjloader_INCLUDE_DIRS ${tinyobjloader_INCLUDE_DIR})
  endif()
endif()

mark_as_advanced(tinyobjloader_INCLUDE_DIR tinyobjloader_LIBRARY)
