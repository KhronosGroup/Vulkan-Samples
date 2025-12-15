cmake_minimum_required(VERSION 3.29)

project(SimpleEngine VERSION 1.0.0 LANGUAGES CXX C)

# Option to enable/disable Vulkan C++20 module support for this standalone project
option(ENABLE_CPP20_MODULE "Enable C++ 20 module support for Vulkan in SimpleEngine" OFF)

# Enable C++ module dependency scanning only when modules are enabled
if(ENABLE_CPP20_MODULE)
    set(CMAKE_CXX_SCAN_FOR_MODULES ON)
endif()

# Add CMake module path for custom find modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/CMake")

# Find required packages
find_package (glfw3 REQUIRED)
find_package (glm REQUIRED)
find_package (Vulkan REQUIRED)
find_package (tinygltf REQUIRED)
find_package (KTX REQUIRED)
find_package (OpenAL REQUIRED)

if(ENABLE_CPP20_MODULE)
    # Set up Vulkan C++ module for this standalone project
    add_library(VulkanCppModule)
    add_library(Vulkan::cppm ALIAS VulkanCppModule)

    target_compile_definitions(VulkanCppModule
            PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
    )
    target_include_directories(VulkanCppModule
            PUBLIC
            "${Vulkan_INCLUDE_DIR}"
    )
    target_link_libraries(VulkanCppModule
            PUBLIC
            Vulkan::Vulkan
    )

    set_target_properties(VulkanCppModule PROPERTIES CXX_STANDARD 20)

    target_sources(VulkanCppModule
            PUBLIC
            FILE_SET cxx_modules TYPE CXX_MODULES
            BASE_DIRS
            "${Vulkan_INCLUDE_DIR}"
            FILES
            "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
    )

    # MSVC-specific options to improve module support
    if(MSVC)
        target_compile_options(VulkanCppModule PRIVATE
            /std:c++latest
            /permissive-
            /Zc:__cplusplus
            /EHsc
            /Zc:preprocessor
        )
    endif()
else()
    add_library(VulkanCppModule INTERFACE)
    add_library(Vulkan::cppm ALIAS VulkanCppModule)
    target_link_libraries(VulkanCppModule INTERFACE Vulkan::Vulkan)
    target_compile_definitions(VulkanCppModule
            INTERFACE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
    )
endif()



# Platform-specific settings
if(ANDROID)
    # Android-specific settings
    add_definitions(-DPLATFORM_ANDROID)
else()
    # Desktop-specific settings
    add_definitions(-DPLATFORM_DESKTOP)
endif()

# Shader compilation
# Find Slang shaders (exclude utility modules that are imported, not compiled standalone)
file(GLOB SLANG_SHADER_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.slang)
list(FILTER SLANG_SHADER_SOURCES EXCLUDE REGEX ".*/(common_types|pbr_utils|lighting_utils|tonemapping_utils)\\.slang$")

# Find slangc executable (optional)
find_program(SLANGC_EXECUTABLE slangc HINTS $ENV{VULKAN_SDK}/bin)

if(SLANGC_EXECUTABLE)
    # Ensure the output directory for compiled shaders exists
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders)

    # Compile Slang shaders using slangc
    foreach(SHADER ${SLANG_SHADER_SOURCES})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        get_filename_component(SHADER_NAME_WE ${SHADER_NAME} NAME_WE)
        string(REGEX REPLACE "\.slang$" "" OUTPUT_NAME ${SHADER_NAME})
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/${OUTPUT_NAME}.spv
            COMMAND ${SLANGC_EXECUTABLE} ${SHADER} -target spirv -profile spirv_1_4 -emit-spirv-directly -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/${OUTPUT_NAME}.spv
            DEPENDS ${SHADER}
            COMMENT "Compiling Slang shader ${SHADER_NAME} with slangc"
        )
        list(APPEND SHADER_SPVS ${CMAKE_CURRENT_BINARY_DIR}/shaders/${OUTPUT_NAME}.spv)
    endforeach()

    add_custom_target(shaders DEPENDS ${SHADER_SPVS})
else()
    message(STATUS "slangc not found. Skipping shader compilation step.")
    add_custom_target(shaders)
endif()

# Source files
set(SOURCES
    main.cpp
    engine.cpp
    scene_loading.cpp
    platform.cpp
    renderer_core.cpp
    renderer_rendering.cpp
    renderer_pipelines.cpp
    renderer_compute.cpp
    renderer_utils.cpp
    renderer_resources.cpp
    renderer_ray_query.cpp
    memory_pool.cpp
    resource_manager.cpp
    entity.cpp
    component.cpp
    transform_component.cpp
    mesh_component.cpp
    camera_component.cpp
    animation_component.cpp
    model_loader.cpp
    audio_system.cpp
    physics_system.cpp
    imgui_system.cpp
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    vulkan_device.cpp
    pipeline.cpp
    descriptor_manager.cpp
    renderdoc_debug_system.cpp
    mikktspace.c
)

# Create executable
add_executable(SimpleEngine ${SOURCES})
add_dependencies(SimpleEngine shaders)
set_target_properties (SimpleEngine PROPERTIES CXX_STANDARD 20)

# Enable required defines for GLM experimental extensions and MSVC math constants
target_compile_definitions(SimpleEngine PRIVATE
    GLM_ENABLE_EXPERIMENTAL
    _USE_MATH_DEFINES
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
)

# Link libraries
# Prefer the Vulkan C++ module target when available (configured at the parent level),
# but fall back to the standard Vulkan library otherwise.
if(TARGET Vulkan::cppm)
    target_link_libraries(SimpleEngine PRIVATE Vulkan::cppm)
else()
    target_link_libraries(SimpleEngine PRIVATE Vulkan::Vulkan)
endif()

target_link_libraries(SimpleEngine PRIVATE
    glm::glm
    tinygltf::tinygltf
    KTX::ktx
    OpenAL::OpenAL
)

if(NOT ANDROID)
    target_link_libraries(SimpleEngine PRIVATE glfw)
endif()

# Windows/MSVC portability and build settings
if(MSVC)
    # Avoid Windows.h macro pollution and CRT warnings; improve conformance and build perf
    target_compile_definitions(SimpleEngine PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        _CRT_SECURE_NO_WARNINGS
    )
    target_compile_options(SimpleEngine PRIVATE
        /permissive-
        /Zc:__cplusplus
        /EHsc
        /W3
        /MP
        /bigobj
    )
    # Crash reporter uses Dbghelp; pragma should suffice, but make it explicit for clarity
    target_link_libraries(SimpleEngine PRIVATE Dbghelp)
elseif(WIN32)
    # Non-MSVC Windows toolchains (e.g., MinGW)
    target_compile_definitions(SimpleEngine PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        _CRT_SECURE_NO_WARNINGS
    )
endif()

# Copy model and texture files if they exist
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/models)
    add_custom_command(TARGET SimpleEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/models ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/models
        COMMENT "Copying models to output directory"
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/textures)
    add_custom_command(TARGET SimpleEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/textures ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/textures
        COMMENT "Copying textures to output directory"
    )
endif()

# Add packaging configuration
include(CPack)

# Set package properties
set(CPACK_PACKAGE_NAME "SimpleEngine")
set(CPACK_PACKAGE_VENDOR "SimpleEngine Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A simple game engine built with Vulkan")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "SimpleEngine")

# Set platform-specific package generators
if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
    set(CPACK_NSIS_PACKAGE_NAME "SimpleEngine")
    set(CPACK_NSIS_DISPLAY_NAME "SimpleEngine")
    set(CPACK_NSIS_HELP_LINK "https://github.com/yourusername/SimpleEngine")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/yourusername/SimpleEngine")
    set(CPACK_NSIS_CONTACT "your.email@example.com")
    set(CPACK_NSIS_MODIFY_PATH ON)
elseif(APPLE)
    set(CPACK_GENERATOR "ZIP;DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "SimpleEngine")
    set(CPACK_DMG_FORMAT "UDBZ")
else()
    set(CPACK_GENERATOR "ZIP;DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Your Name <your.email@example.com>")
    set(CPACK_DEBIAN_PACKAGE_SECTION "games")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libvulkan1, libglfw3, libglm-dev, libktx-dev")
endif()

# Include binary and resource directories in the package
install(TARGETS SimpleEngine DESTINATION bin)
if(SLANGC_EXECUTABLE)
    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders DESTINATION share/SimpleEngine)
endif()

# Install models and textures if they exist
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/models)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/models DESTINATION share/SimpleEngine)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/textures)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/textures DESTINATION share/SimpleEngine)
endif()

# Install README if it exists
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/README.md DESTINATION share/SimpleEngine)
endif()
