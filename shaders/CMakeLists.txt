file(GLOB_RECURSE GLSL_SHADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.glsl)

foreach(SHADER_FILE ${GLSL_SHADER_FILES})
    string(REPLACE ${CMAKE_CURRENT_SOURCE_DIR}/ "" SHADER_NAME ${SHADER_FILE})
    string(REPLACE "/" "_" SHADER_NAME ${SHADER_NAME})
    string(REPLACE "." "_" SHADER_NAME ${SHADER_NAME})
    string(REPLACE "\\" "_" SHADER_NAME ${SHADER_NAME})
    
    configure_shader(
        NAME ${SHADER_NAME}
        SOURCE ${SHADER_FILE}
        OUTPUT ${SHADER_NAME}.spv
    )
endforeach()