# Findstb.cmake
#
# Finds the stb library (specifically stb_image.h)
#
# This will define the following variables
#
#    stb_FOUND
#    stb_INCLUDE_DIRS
#
# and the following imported targets
#
#    stb::stb
#

# Try to find the package using pkg-config first
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_stb QUIET stb)
endif()

# Find the include directory
find_path(stb_INCLUDE_DIR
  NAMES stb_image.h
  PATHS
    ${PC_stb_INCLUDE_DIRS}
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
  PATH_SUFFIXES stb
)

# If the include directory wasn't found, use FetchContent to download and build
if(NOT stb_INCLUDE_DIR)
  # If not found, use FetchContent to download and build
  include(FetchContent)

  message(STATUS "stb_image.h not found, fetching from GitHub...")
  FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master  # stb doesn't use version tags, so we use master
  )

  # Set policy to suppress the deprecation warning
  if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
  endif()

  # Populate the content
  FetchContent_GetProperties(stb)
  if(NOT stb_POPULATED)
    FetchContent_Populate(stb)
  endif()

  # stb is a header-only library with no CMakeLists.txt, so we just need to set the include directory
  set(stb_INCLUDE_DIR ${stb_SOURCE_DIR})
endif()

# Set the variables
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(stb 
  REQUIRED_VARS stb_INCLUDE_DIR
)

if(stb_FOUND)
  set(stb_INCLUDE_DIRS ${stb_INCLUDE_DIR})

  # Create an imported target
  if(NOT TARGET stb::stb)
    add_library(stb::stb INTERFACE IMPORTED)
    set_target_properties(stb::stb PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${stb_INCLUDE_DIRS}"
    )
  endif()
endif()

mark_as_advanced(stb_INCLUDE_DIR)