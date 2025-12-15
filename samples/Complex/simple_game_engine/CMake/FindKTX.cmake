# FindKTX.cmake
#
# Finds the KTX library
#
# This will define the following variables
#
#    KTX_FOUND
#    KTX_INCLUDE_DIRS
#    KTX_LIBRARIES
#
# and the following imported targets
#
#    KTX::ktx
#

# Check if we're on Linux - if so, we'll skip the search and directly use FetchContent
if(UNIX AND NOT APPLE)
  # On Linux, we assume KTX is not installed and proceed directly to fetching it
  set(KTX_FOUND FALSE)
else()
  # On non-Linux platforms, try to find KTX using pkg-config first
  find_package(PkgConfig QUIET)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_KTX QUIET ktx libktx ktx2 libktx2)
  endif()

  # Try to find KTX using standard find_package
  find_path(KTX_INCLUDE_DIR
    NAMES ktx.h
    PATH_SUFFIXES include ktx KTX ktx2 KTX2
    HINTS
      ${PC_KTX_INCLUDEDIR}
      /usr/include
      /usr/local/include
      $ENV{KTX_DIR}/include
      $ENV{VULKAN_SDK}/include
      ${CMAKE_SOURCE_DIR}/external/ktx/include
  )

  find_library(KTX_LIBRARY
    NAMES ktx ktx2 libktx libktx2
    PATH_SUFFIXES lib lib64
    HINTS
      ${PC_KTX_LIBDIR}
      /usr/lib
      /usr/lib64
      /usr/local/lib
      /usr/local/lib64
      $ENV{KTX_DIR}/lib
      $ENV{VULKAN_SDK}/lib
      ${CMAKE_SOURCE_DIR}/external/ktx/lib
  )

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(KTX
    REQUIRED_VARS KTX_INCLUDE_DIR KTX_LIBRARY
    FAIL_MESSAGE ""  # Suppress the error message to allow our fallback
  )

  # Debug output if KTX is not found (only on non-Linux platforms)
  if(NOT KTX_FOUND)
    message(STATUS "KTX include directory search paths: ${PC_KTX_INCLUDEDIR}, /usr/include, /usr/local/include, $ENV{KTX_DIR}/include, $ENV{VULKAN_SDK}/include, ${CMAKE_SOURCE_DIR}/external/ktx/include")
    message(STATUS "KTX library search paths: ${PC_KTX_LIBDIR}, /usr/lib, /usr/lib64, /usr/local/lib, /usr/local/lib64, $ENV{KTX_DIR}/lib, $ENV{VULKAN_SDK}/lib, ${CMAKE_SOURCE_DIR}/external/ktx/lib")
  endif()
endif()

if(KTX_FOUND)
  set(KTX_INCLUDE_DIRS ${KTX_INCLUDE_DIR})
  set(KTX_LIBRARIES ${KTX_LIBRARY})

  if(NOT TARGET KTX::ktx)
    add_library(KTX::ktx UNKNOWN IMPORTED)
    set_target_properties(KTX::ktx PROPERTIES
      IMPORTED_LOCATION "${KTX_LIBRARIES}"
      INTERFACE_INCLUDE_DIRECTORIES "${KTX_INCLUDE_DIRS}"
    )
  endif()
else()
  # If not found, use FetchContent to download and build
  include(FetchContent)

  # Only show the message on non-Linux platforms
  if(NOT (UNIX AND NOT APPLE))
    message(STATUS "KTX not found, fetching from GitHub...")
  endif()

  FetchContent_Declare(
    ktx
    GIT_REPOSITORY https://github.com/KhronosGroup/KTX-Software.git
    GIT_TAG v4.3.1  # Use a specific tag for stability
  )

  # Set options to minimize build time and dependencies
  set(KTX_FEATURE_TOOLS OFF CACHE BOOL "Build KTX tools" FORCE)
  set(KTX_FEATURE_DOC OFF CACHE BOOL "Build KTX documentation" FORCE)
  set(KTX_FEATURE_TESTS OFF CACHE BOOL "Build KTX tests" FORCE)

  FetchContent_MakeAvailable(ktx)

  # Create an alias to match the expected target name
  if(NOT TARGET KTX::ktx)
    add_library(KTX::ktx ALIAS ktx)
  endif()

  set(KTX_FOUND TRUE)
endif()
