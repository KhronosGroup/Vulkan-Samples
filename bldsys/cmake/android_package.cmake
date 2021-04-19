#[[
 Copyright (c) 2019, Arm Limited and Contributors

 SPDX-License-Identifier: Apache-2.0

 Licensed under the Apache License, Version 2.0 the "License";
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

 ]]

set(SCRIPT_DIR ${CMAKE_CURRENT_LIST_DIR})

function(add_android_package_project)
    set(options)  
    set(oneValueArgs NAME JAVA_DIR RES_DIR MANIFEST_FILE)
    set(multiValueArgs DEPENDS ASSET_DIRS)

    cmake_parse_arguments(PARSE_ARGV 0 TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}")
    
    find_package(Gradle REQUIRED 4.10)
    
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME})
    
    project(${TARGET_NAME})
    
    set(GRADLE_ANDROID_SCRIPT ${SCRIPT_DIR}/create_gradle_project.cmake)
    
    if(NOT EXISTS ${GRADLE_ANDROID_SCRIPT})
        message(FATAL_ERROR "No gradle android script exists at `${GRADLE_ANDROID_SCRIPT}`")
    endif()
       
    set(GRADLE_CONFIG_FILE ${SCRIPT_DIR}/template/gradle/build.gradle.in)
    
    if(NOT EXISTS ${GRADLE_CONFIG_FILE})
        message(FATAL_ERROR "No build.gradle.in template file exists at `${GRADLE_CONFIG_FILE}`")
    endif()
    
    set(JNI_LIBS_DIRS)
    
    foreach(TARGET_DEPEND ${TARGET_DEPENDS})
        if(TARGET ${TARGET_DEPEND})
            get_target_property(BINARY_DIR ${TARGET_DEPEND} BINARY_DIR)
        
            list(APPEND JNI_LIBS_DIRS ${BINARY_DIR}/lib/${CMAKE_BUILD_TYPE})
        endif()
    endforeach()
           
    add_custom_command(
        OUTPUT
            gen.gradle.stamp
        DEPENDS
            ${GRADLE_CONFIG_FILE}
            ${GRADLE_ANDROID_SCRIPT}
        COMMAND
            ${CMAKE_COMMAND} 
                -DPROJECT_NAME=${CMAKE_PROJECT_NAME}
                -DANDROID_API=${CMAKE_ANDROID_API}
                -DARCH_ABI=${CMAKE_ANDROID_ARCH_ABI}
                -DANDROID_MANIFEST=${TARGET_MANIFEST_FILE}
                -DJAVA_DIRS=${TARGET_JAVA_DIR}
                -DRES_DIRS=${TARGET_RES_DIR}
                -DJNI_LIBS_DIRS=${JNI_LIBS_DIRS}
                -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME} 
                -DASSET_DIRS=${TARGET_ASSET_DIRS}
                -P ${GRADLE_ANDROID_SCRIPT}
        COMMAND
            ${CMAKE_COMMAND} -E touch gen.gradle.stamp
        COMMENT
            "Generate android gradle project"
        WORKING_DIRECTORY
            ${TARGET_NAME}
        VERBATIM)
        
    add_custom_command(
        OUTPUT
            bld.gradle.stamp
        DEPENDS
            gen.gradle.stamp
        COMMAND
            ${GRADLE_EXECUTABLE} assemble --no-daemon
        COMMAND
            ${CMAKE_COMMAND} -E touch bld.gradle.stamp
        COMMENT
            "Build android gradle project"
        WORKING_DIRECTORY
            ${TARGET_NAME}
        VERBATIM)    
        
    add_custom_target(${TARGET_NAME} ALL 
        DEPENDS
            bld.gradle.stamp
            ${TARGET_DEPENDS})

endfunction()

function(android_sync_folder)
    set(options)
    set(oneValueArgs PATH)
    set(multiValueArgs)

    cmake_parse_arguments(PARSE_ARGV 0 TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}")
    get_filename_component(FOLDER_NAME "${TARGET_PATH}" NAME)

    set(SYNC_COMMAND ${CMAKE_COMMAND}
            -DCMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}
            -DFOLDER_DIR=${TARGET_PATH}/.
            -DDEVICE_DIR=/sdcard/com.khronos.${PROJECT_NAME}/${FOLDER_NAME}/
            -P "${SCRIPT_DIR}/android_sync_folder.cmake")

    add_custom_target(
            sync.${FOLDER_NAME}.stamp
            COMMAND
            ${SYNC_COMMAND}
            COMMENT
            "Update ${FOLDER_NAME} in external storage"
            VERBATIM)

    add_dependencies(${PROJECT_NAME} sync.${FOLDER_NAME}.stamp)
endfunction()
