vkb__register_headers(
    FOLDER
        glslang
    HEADERS
      shader_compiler.hpp
    OUTPUT
      GLSLANG_HEADERS
)

vkb__register_component(
  NAME glslang
  SRC
    src/glslang/shader_compiler.cpp
    src/glslang/default_resource_limits.cpp
  HEADERS ${GLSLANG_HEADERS}
  INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/glslang
  LINK_LIBS
    glslang
    SPIRV
    OGLCompiler
    OSDependent
    MachineIndependent
    vkb__vulkan__headers # we only require the interface definitions
)

vkb__register_tests(
    NAME "glslang_compiler_tests"
    SRC
      tests/glslang_shader_compiler_tests.cpp
    LIBS
      vkb__vfs
      vkb__glslang
      vkb__vulkan
    INCLUDE_DIRS
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

vkb__register_gpu_tests(
    NAME "glslang_compiler_gpu_tests"
    SRC
      tests/gpu/glslang_shader_compiler_gpu_tests.cpp
    LIBS
      vkb__vfs
      vkb__glslang
      vkb__vulkan
    INCLUDE_DIRS
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

# Spirv Reflect

vkb__register_headers(
    FOLDER
        spirv_reflect
    HEADERS
      shader_reflector.hpp
    OUTPUT
      REFLECT_HEADERS
)


vkb__register_component(
  NAME spirv_reflect
  SRC
    src/spirv_reflect/shader_reflector.cpp
  HEADERS ${REFLECT_HEADERS}
  INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  LINK_LIBS
    spirv-reflect-static
    vkb__vulkan__headers # we only require the interface definitions
)

vkb__register_tests(
    NAME "spirv_reflect_reflector_tests"
    SRC
      tests/spirv_reflect_shader_reflector_tests.cpp
    LIBS
      vkb__glslang
      vkb__vfs
      vkb__spirv_reflect
      vkb__vulkan
    INCLUDE_DIRS
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
)
