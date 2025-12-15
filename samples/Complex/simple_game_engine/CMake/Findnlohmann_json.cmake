# Findnlohmann_json.cmake
#
# Finds the nlohmann_json library
#
# This will define the following variables
#
#    nlohmann_json_FOUND
#    nlohmann_json_INCLUDE_DIRS
#
# and the following imported targets
#
#    nlohmann_json::nlohmann_json
#

# Try to find the package using pkg-config first
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_nlohmann_json QUIET nlohmann_json)
endif()

# Find the include directory
find_path(nlohmann_json_INCLUDE_DIR
  NAMES nlohmann/json.hpp json.hpp
  PATHS
    ${PC_nlohmann_json_INCLUDE_DIRS}
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
  PATH_SUFFIXES nlohmann json
)

# If the include directory wasn't found, use FetchContent to download and build
if(NOT nlohmann_json_INCLUDE_DIR)
  # If not found, use FetchContent to download and build
  include(FetchContent)

  message(STATUS "nlohmann_json not found, fetching from GitHub...")
  FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0  # Use a specific tag for stability
  )

  # Set policy to suppress the deprecation warning
  if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
  endif()

  # Populate the content but don't configure it yet
  FetchContent_GetProperties(nlohmann_json)
  if(NOT nlohmann_json_POPULATED)
    FetchContent_Populate(nlohmann_json)

    if(ANDROID)
    # Update the minimum required CMake version before including the CMakeLists.txt
    file(READ "${nlohmann_json_SOURCE_DIR}/CMakeLists.txt" NLOHMANN_JSON_CMAKE_CONTENT)
    string(REPLACE "cmake_minimum_required(VERSION 3.1"
                   "cmake_minimum_required(VERSION 3.10"
                   NLOHMANN_JSON_CMAKE_CONTENT "${NLOHMANN_JSON_CMAKE_CONTENT}")
    string(REPLACE "cmake_minimum_required(VERSION 3.2"
                   "cmake_minimum_required(VERSION 3.10"
                   NLOHMANN_JSON_CMAKE_CONTENT "${NLOHMANN_JSON_CMAKE_CONTENT}")
    string(REPLACE "cmake_minimum_required(VERSION 3.3"
                   "cmake_minimum_required(VERSION 3.10"
                   NLOHMANN_JSON_CMAKE_CONTENT "${NLOHMANN_JSON_CMAKE_CONTENT}")
    string(REPLACE "cmake_minimum_required(VERSION 3.4"
                   "cmake_minimum_required(VERSION 3.10"
                   NLOHMANN_JSON_CMAKE_CONTENT "${NLOHMANN_JSON_CMAKE_CONTENT}")
    string(REPLACE "cmake_minimum_required(VERSION 3.5"
                   "cmake_minimum_required(VERSION 3.10"
                   NLOHMANN_JSON_CMAKE_CONTENT "${NLOHMANN_JSON_CMAKE_CONTENT}")
    string(REPLACE "cmake_minimum_required(VERSION 3.6"
                   "cmake_minimum_required(VERSION 3.10"
                   NLOHMANN_JSON_CMAKE_CONTENT "${NLOHMANN_JSON_CMAKE_CONTENT}")
    string(REPLACE "cmake_minimum_required(VERSION 3.7"
                   "cmake_minimum_required(VERSION 3.10"
                   NLOHMANN_JSON_CMAKE_CONTENT "${NLOHMANN_JSON_CMAKE_CONTENT}")
    string(REPLACE "cmake_minimum_required(VERSION 3.8"
                   "cmake_minimum_required(VERSION 3.10"
                   NLOHMANN_JSON_CMAKE_CONTENT "${NLOHMANN_JSON_CMAKE_CONTENT}")
    string(REPLACE "cmake_minimum_required(VERSION 3.9"
                   "cmake_minimum_required(VERSION 3.10"
                   NLOHMANN_JSON_CMAKE_CONTENT "${NLOHMANN_JSON_CMAKE_CONTENT}")
    file(WRITE "${nlohmann_json_SOURCE_DIR}/CMakeLists.txt" "${NLOHMANN_JSON_CMAKE_CONTENT}")
    endif()

    # Now add the subdirectory manually
    add_subdirectory(${nlohmann_json_SOURCE_DIR} ${nlohmann_json_BINARY_DIR})
  else()
    # If already populated, just make it available
    FetchContent_MakeAvailable(nlohmann_json)
  endif()

  # Get the include directory from the target
  if(TARGET nlohmann_json)
    get_target_property(nlohmann_json_INCLUDE_DIR nlohmann_json INTERFACE_INCLUDE_DIRECTORIES)
    if(NOT nlohmann_json_INCLUDE_DIR)
      # If we can't get the include directory from the target, use the source directory
      set(nlohmann_json_INCLUDE_DIR ${nlohmann_json_SOURCE_DIR}/include)
    endif()
  else()
    # nlohmann_json might not create a target, so use the source directory
    set(nlohmann_json_INCLUDE_DIR ${nlohmann_json_SOURCE_DIR}/include)
  endif()
endif()

# Set the variables
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(nlohmann_json
  REQUIRED_VARS nlohmann_json_INCLUDE_DIR
)

if(nlohmann_json_FOUND)
  set(nlohmann_json_INCLUDE_DIRS ${nlohmann_json_INCLUDE_DIR})

  # Create an imported target
  if(NOT TARGET nlohmann_json::nlohmann_json)
    add_library(nlohmann_json::nlohmann_json INTERFACE IMPORTED)
    set_target_properties(nlohmann_json::nlohmann_json PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${nlohmann_json_INCLUDE_DIRS}"
    )
  endif()
elseif(TARGET nlohmann_json)
  # If find_package_handle_standard_args failed but we have a nlohmann_json target from FetchContent
  # Create an alias for the nlohmann_json target
  if(NOT TARGET nlohmann_json::nlohmann_json)
    add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
  endif()

  # Set variables to indicate that nlohmann_json was found
  set(nlohmann_json_FOUND TRUE)
  set(NLOHMANN_JSON_FOUND TRUE)

  # Set include directories
  get_target_property(nlohmann_json_INCLUDE_DIR nlohmann_json INTERFACE_INCLUDE_DIRECTORIES)
  if(nlohmann_json_INCLUDE_DIR)
    set(nlohmann_json_INCLUDE_DIRS ${nlohmann_json_INCLUDE_DIR})
  else()
    # If we can't get the include directory from the target, use the source directory
    set(nlohmann_json_INCLUDE_DIR ${nlohmann_json_SOURCE_DIR}/include)
    set(nlohmann_json_INCLUDE_DIRS ${nlohmann_json_INCLUDE_DIR})
  endif()
endif()

mark_as_advanced(nlohmann_json_INCLUDE_DIR)
