# Findtinygltf.cmake
#
# Finds the tinygltf library
#
# This will define the following variables
#
#    tinygltf_FOUND
#    tinygltf_INCLUDE_DIRS
#
# and the following imported targets
#
#    tinygltf::tinygltf
#

# First, try to find nlohmann_json
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
  include(FetchContent)
  message(STATUS "nlohmann_json not found, fetching v3.12.0 from GitHub...")
  FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0  # Use a specific tag for stability
  )
  FetchContent_MakeAvailable(nlohmann_json)
endif()

# Try to find tinygltf using standard find_package
find_path(tinygltf_INCLUDE_DIR
  NAMES tiny_gltf.h
  PATHS
    ${PC_tinygltf_INCLUDE_DIRS}
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
  PATH_SUFFIXES tinygltf include
)

# If not found, use FetchContent to download and build
if(NOT tinygltf_INCLUDE_DIR)
  # If not found, use FetchContent to download and build
  include(FetchContent)

  message(STATUS "tinygltf not found, fetching from GitHub...")
  FetchContent_Declare(
    tinygltf
    GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
    GIT_TAG v2.8.18  # Use a specific tag for stability
  )

  # Set policy to suppress the deprecation warning
  if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
  endif()

  # Populate the content but don't configure it yet
  FetchContent_GetProperties(tinygltf)
  if(NOT tinygltf_POPULATED)
    FetchContent_Populate(tinygltf)

    # Update the minimum required CMake version to avoid deprecation warning
    file(READ "${tinygltf_SOURCE_DIR}/CMakeLists.txt" TINYGLTF_CMAKE_CONTENT)
    string(REPLACE "cmake_minimum_required(VERSION 3.6)"
                   "cmake_minimum_required(VERSION 3.10)"
                   TINYGLTF_CMAKE_CONTENT "${TINYGLTF_CMAKE_CONTENT}")
    file(WRITE "${tinygltf_SOURCE_DIR}/CMakeLists.txt" "${TINYGLTF_CMAKE_CONTENT}")

    # Create a symbolic link to make nlohmann/json.hpp available
    if(EXISTS "${tinygltf_SOURCE_DIR}/json.hpp")
      file(MAKE_DIRECTORY "${tinygltf_SOURCE_DIR}/nlohmann")
      file(CREATE_LINK "${tinygltf_SOURCE_DIR}/json.hpp" "${tinygltf_SOURCE_DIR}/nlohmann/json.hpp" SYMBOLIC)
    endif()

    # Set tinygltf to header-only mode
    set(TINYGLTF_HEADER_ONLY ON CACHE BOOL "Use header only version" FORCE)
    set(TINYGLTF_INSTALL OFF CACHE BOOL "Do not install tinygltf" FORCE)

    # Add the subdirectory after modifying the CMakeLists.txt
    add_subdirectory(${tinygltf_SOURCE_DIR} ${tinygltf_BINARY_DIR})
  else()
    # If already populated, just make it available
    FetchContent_MakeAvailable(tinygltf)
  endif()

  # Get the include directory from the target
  get_target_property(tinygltf_INCLUDE_DIR tinygltf INTERFACE_INCLUDE_DIRECTORIES)
  if(NOT tinygltf_INCLUDE_DIR)
    # If we can't get the include directory from the target, use the source directory
    FetchContent_GetProperties(tinygltf SOURCE_DIR tinygltf_SOURCE_DIR)
    set(tinygltf_INCLUDE_DIR ${tinygltf_SOURCE_DIR})
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(tinygltf
  REQUIRED_VARS tinygltf_INCLUDE_DIR
)

if(tinygltf_FOUND)
  set(tinygltf_INCLUDE_DIRS ${tinygltf_INCLUDE_DIR})

  if(NOT TARGET tinygltf::tinygltf)
    add_library(tinygltf::tinygltf INTERFACE IMPORTED)
    set_target_properties(tinygltf::tinygltf PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${tinygltf_INCLUDE_DIRS}"
      INTERFACE_COMPILE_DEFINITIONS "TINYGLTF_IMPLEMENTATION;TINYGLTF_NO_EXTERNAL_IMAGE;TINYGLTF_NO_STB_IMAGE;TINYGLTF_NO_STB_IMAGE_WRITE"
    )
    if(TARGET nlohmann_json::nlohmann_json)
      target_link_libraries(tinygltf::tinygltf INTERFACE nlohmann_json::nlohmann_json)
    endif()
  endif()
elseif(TARGET tinygltf)
  # If find_package_handle_standard_args failed but we have a tinygltf target from FetchContent
  # Create an alias for the tinygltf target
  if(NOT TARGET tinygltf::tinygltf)
    add_library(tinygltf_wrapper INTERFACE)
    target_link_libraries(tinygltf_wrapper INTERFACE tinygltf)
    target_compile_definitions(tinygltf_wrapper INTERFACE
      TINYGLTF_IMPLEMENTATION
      TINYGLTF_NO_EXTERNAL_IMAGE
      TINYGLTF_NO_STB_IMAGE
      TINYGLTF_NO_STB_IMAGE_WRITE
    )
    if(TARGET nlohmann_json::nlohmann_json)
      target_link_libraries(tinygltf_wrapper INTERFACE nlohmann_json::nlohmann_json)
    endif()
    add_library(tinygltf::tinygltf ALIAS tinygltf_wrapper)
  endif()

  # Set variables to indicate that tinygltf was found
  set(tinygltf_FOUND TRUE)
  set(TINYGLTF_FOUND TRUE)

  # Set include directories
  get_target_property(tinygltf_INCLUDE_DIR tinygltf INTERFACE_INCLUDE_DIRECTORIES)
  if(tinygltf_INCLUDE_DIR)
    set(tinygltf_INCLUDE_DIRS ${tinygltf_INCLUDE_DIR})
  else()
    # If we can't get the include directory from the target, use the source directory
    FetchContent_GetProperties(tinygltf SOURCE_DIR tinygltf_SOURCE_DIR)
    set(tinygltf_INCLUDE_DIR ${tinygltf_SOURCE_DIR})
    set(tinygltf_INCLUDE_DIRS ${tinygltf_INCLUDE_DIR})

    # Explicitly set the include directory on the target
    if(TARGET tinygltf)
      set_target_properties(tinygltf PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${tinygltf_INCLUDE_DIR}"
      )
    endif()
endif()
endif()

mark_as_advanced(tinygltf_INCLUDE_DIR)
