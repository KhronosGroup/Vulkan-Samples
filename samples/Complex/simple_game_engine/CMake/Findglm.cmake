# Findglm.cmake
#
# Finds the GLM library
#
# This will define the following variables
#
#    glm_FOUND
#    glm_INCLUDE_DIRS
#
# and the following imported targets
#
#    glm::glm
#

# Try to find the package using pkg-config first
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_glm QUIET glm)
endif()

# Find the include directory
find_path(glm_INCLUDE_DIR
  NAMES glm/glm.hpp
  PATHS
    ${PC_glm_INCLUDE_DIRS}
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
  PATH_SUFFIXES glm
)

# If the include directory wasn't found, use FetchContent to download and build
if(NOT glm_INCLUDE_DIR)
  # If not found, use FetchContent to download and build
  include(FetchContent)

  message(STATUS "GLM not found, fetching from GitHub...")
  FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 0.9.9.8  # Use a specific tag for stability
  )

  # Define a function to update the CMake minimum required version
  function(update_glm_cmake_version)
    # Get the source directory
    FetchContent_GetProperties(glm SOURCE_DIR glm_SOURCE_DIR)

    # Update the minimum required CMake version
    file(READ "${glm_SOURCE_DIR}/CMakeLists.txt" GLM_CMAKE_CONTENT)
    string(REPLACE "cmake_minimum_required(VERSION 3.2"
                   "cmake_minimum_required(VERSION 3.5"
                   GLM_CMAKE_CONTENT "${GLM_CMAKE_CONTENT}")
    file(WRITE "${glm_SOURCE_DIR}/CMakeLists.txt" "${GLM_CMAKE_CONTENT}")
  endfunction()

  # Set policy to suppress the deprecation warning
  if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
  endif()

  # First, declare and populate the content
  FetchContent_GetProperties(glm)
  if(NOT glm_POPULATED)
    FetchContent_Populate(glm)
    # Update the CMake version before making it available
    update_glm_cmake_version()
  endif()

  # Now make it available (this will process the CMakeLists.txt)
  FetchContent_MakeAvailable(glm)

  # Get the include directory from the target
  if(TARGET glm)
    get_target_property(glm_INCLUDE_DIR glm INTERFACE_INCLUDE_DIRECTORIES)
    if(NOT glm_INCLUDE_DIR)
      # If we can't get the include directory from the target, use the source directory
      set(glm_INCLUDE_DIR ${glm_SOURCE_DIR})
    endif()
  else()
    # GLM might not create a target, so use the source directory
    set(glm_INCLUDE_DIR ${glm_SOURCE_DIR})
  endif()
endif()

# Set the variables
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(glm 
  REQUIRED_VARS glm_INCLUDE_DIR
)

if(glm_FOUND)
  set(glm_INCLUDE_DIRS ${glm_INCLUDE_DIR})

  # Create an imported target
  if(NOT TARGET glm::glm)
    add_library(glm::glm INTERFACE IMPORTED)
    set_target_properties(glm::glm PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${glm_INCLUDE_DIRS}"
    )
  endif()
elseif(TARGET glm)
  # If find_package_handle_standard_args failed but we have a glm target from FetchContent
  # Create an alias for the glm target
  if(NOT TARGET glm::glm)
    add_library(glm::glm ALIAS glm)
  endif()

  # Set variables to indicate that glm was found
  set(glm_FOUND TRUE)
  set(GLM_FOUND TRUE)

  # Set include directories
  get_target_property(glm_INCLUDE_DIR glm INTERFACE_INCLUDE_DIRECTORIES)
  if(glm_INCLUDE_DIR)
    set(glm_INCLUDE_DIRS ${glm_INCLUDE_DIR})
  else()
    # If we can't get the include directory from the target, use the source directory
    set(glm_INCLUDE_DIR ${glm_SOURCE_DIR})
    set(glm_INCLUDE_DIRS ${glm_INCLUDE_DIR})
  endif()
endif()

mark_as_advanced(glm_INCLUDE_DIR)
