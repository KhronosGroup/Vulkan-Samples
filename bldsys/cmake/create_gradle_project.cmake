#[[
 Copyright (c) 2019-2022, Arm Limited and Contributors

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

cmake_minimum_required(VERSION 3.12)

set(SCRIPT_DIR ${CMAKE_CURRENT_LIST_DIR})
set(ROOT_DIR ${SCRIPT_DIR}/../..)
set(GRADLE_BUILD_FILE ${SCRIPT_DIR}/template/gradle/build.gradle.in)
set(GRADLE_APP_BUILD_FILE ${SCRIPT_DIR}/template/gradle/app.build.gradle.in)
set(GRADLE_SETTINGS_FILE ${SCRIPT_DIR}/template/gradle/settings.gradle.in)
set(GRADLE_PROPERTIES_FILE ${SCRIPT_DIR}/template/gradle/gradle.properties.in)
set(GRADLE_LOCAL_PROPERTIES_FILE ${SCRIPT_DIR}/template/gradle/local.properties.in)
set(GRADLE_WRAPPER_PROPERTIES_FILE ${SCRIPT_DIR}/template/gradle/gradle-wrapper.properties.in)
set(GRADLE_WRAPPER_PROPERTIES_JAR ${SCRIPT_DIR}/template/gradle/gradle-wrapper.jar)
set(GRADLE_WRAPPER_SCRIPT_LINUX ${SCRIPT_DIR}/template/gradle/gradlew)
set(GRADLE_WRAPPER_SCRIPT_WIN ${SCRIPT_DIR}/template/gradle/gradlew.bat)
set(VALID_ABI "armeabi" "armeabi-v7a" "arm64-v8a" "x86" "x86_64")

include(${SCRIPT_DIR}/utils.cmake)

# script parameters
set(ANDROID_API 30 CACHE STRING "")
set(ANDROID_SDK "" CACHE STRING "")
set(ANDROID_MANIFEST "AndroidManifest.xml" CACHE STRING "")
set(ARCH_ABI "arm64-v8a;armeabi-v7a" CACHE STRING "")
set(ASSET_DIRS "assets" CACHE STRING "")
set(RES_DIRS "res" CACHE STRING "")
set(JAVA_DIRS "java" CACHE STRING "")
set(JNI_LIBS_DIRS "jni" CACHE STRING "")
set(NATIVE_SCRIPT "CMakeLists.txt" CACHE STRING "")
set(NATIVE_ARGUMENTS "ANDROID_TOOLCHAIN=clang;ANDROID_STL=c++_static;VKB_VALIDATION_LAYERS=OFF" CACHE STRING "")
set(OUTPUT_DIR "${ROOT_DIR}/build/android_gradle" CACHE PATH "")

# android sdk path
if(DEFINED ENV{ANDROID_HOME})
  if(NOT IS_ABSOLUTE $ENV{ANDROID_HOME})
	message(FATAL_ERROR "ANDROID_HOME must be absolute path")
  endif()
  set(ANDROID_SDK $ENV{ANDROID_HOME} CACHE STRING "" FORCE)
else()
	message(FATAL_ERROR "Can't find Android SDK, please define environment variable ANDROID_HOME")
endif()

if(NOT ANDROID_SDK OR NOT IS_ABSOLUTE ${ANDROID_SDK})
  message(FATAL_ERROR "Can't find required Android sdk path or its not absolute")
endif()

# minSdkVersion
set(MIN_SDK_VERSION "minSdk ${ANDROID_API}")

# manifest.srcFile
if(NOT IS_ABSOLUTE ${ANDROID_MANIFEST})
	set(ANDROID_MANIFEST ${CMAKE_SOURCE_DIR}/${ANDROID_MANIFEST})
endif()

if(EXISTS ${ANDROID_MANIFEST})
	file(RELATIVE_PATH ANDROID_MANIFEST ${OUTPUT_DIR}/app ${ANDROID_MANIFEST})
	set(MANIFEST_FILE "manifest.srcFile '${ANDROID_MANIFEST}'")
else()
	message(FATAL_ERROR "Manifest file does not exists at `${ANDROID_MANIFEST}`")
endif()

# ndk.abiFilters
set(ABI_LIST)

foreach(ABI_FILTER ${ARCH_ABI})
	if(${ABI_FILTER} IN_LIST VALID_ABI)
		list(APPEND ABI_LIST ${ABI_FILTER})
	else()
		message(STATUS "Filter abi is invalid `${ABI_FILTER}`")
	endif()
endforeach()

list(JOIN ABI_LIST "', '" ABI_LIST)

if(NOT ${ABI_LIST})
	set(NDK_ABI_FILTERS "abiFilters.addAll( '${ABI_LIST}' )")
else()
	message(FATAL_ERROR "Minimum one android arch abi required.")
endif()

# assets.srcDirs
set(ASSETS_LIST)

foreach(ASSET_DIR ${ASSET_DIRS})
	if(NOT IS_ABSOLUTE ${ASSET_DIR})
		set(ASSET_DIR ${CMAKE_SOURCE_DIR}/${ASSET_DIR})
	endif()

	if(IS_DIRECTORY ${ASSET_DIR})
		file(RELATIVE_PATH ASSET_DIR ${OUTPUT_DIR}/app ${ASSET_DIR})
		list(APPEND ASSETS_LIST ${ASSET_DIR})
	else()
		message(STATUS "Asset dir not exists at `${ASSET_DIR}`")
	endif()
endforeach()

list(JOIN ASSETS_LIST "', '" ASSETS_LIST)

if(NOT ${ASSETS_LIST})
	set(ASSETS_SRC_DIRS "assets.srcDirs += [ '${ASSETS_LIST}' ]")
endif()

# res.srcDirs
set(RES_LIST)

foreach(RES_DIR ${RES_DIRS})
	if(NOT IS_ABSOLUTE ${RES_DIR})
		set(RES_DIR ${CMAKE_SOURCE_DIR}/${RES_DIR})
	endif()

	if(IS_DIRECTORY ${RES_DIR})
		file(RELATIVE_PATH RES_DIR ${OUTPUT_DIR}/app ${RES_DIR})
		list(APPEND RES_LIST ${RES_DIR})
	else()
		message(STATUS "Resource dir not exists at `${RES_DIR}`")
	endif()
endforeach()

list(JOIN RES_LIST "', '" RES_LIST)

if(NOT ${RES_LIST})
	set(RES_SRC_DIRS "res.srcDirs += [ '${RES_LIST}' ]")
endif()

# java.srcDirs
set(JAVA_LIST)

foreach(JAVA_DIR ${JAVA_DIRS})
	if(NOT IS_ABSOLUTE ${JAVA_DIR})
		set(JAVA_DIR ${CMAKE_SOURCE_DIR}/${JAVA_DIR})
	endif()

	if(IS_DIRECTORY ${JAVA_DIR})
		file(RELATIVE_PATH JAVA_DIR ${OUTPUT_DIR}/app ${JAVA_DIR})
		list(APPEND JAVA_LIST ${JAVA_DIR})
	else()
		message(STATUS "Java dir not exists at `${JAVA_DIR}`")
	endif()
endforeach()

list(JOIN JAVA_LIST "', '" JAVA_LIST)

if(NOT ${JAVA_LIST})
	set(JAVA_SRC_DIRS "java.srcDirs += [ '${JAVA_LIST}' ]")
endif()

# jniLibs.srcDirs
set(JNI_LIBS_DIR_LIST)

foreach(JNI_LIBS_DIR ${JNI_LIBS_DIRS})
	if(NOT IS_ABSOLUTE ${JNI_LIBS_DIR})
		set(JNI_LIBS_DIR ${CMAKE_SOURCE_DIR}/${JNI_LIBS_DIR})
	endif()

	if(IS_DIRECTORY ${JNI_LIBS_DIR})
		file(RELATIVE_PATH JNI_LIBS_DIR ${OUTPUT_DIR}/app ${JNI_LIBS_DIR})
		list(APPEND JNI_LIBS_DIR_LIST ${JNI_LIBS_DIR})
	else()
		message(STATUS "JNI lib dir not exists at `${JNI_LIBS_DIR}`")
	endif()
endforeach()

list(JOIN JNI_LIBS_DIR_LIST "', '" JNI_LIBS_DIR_LIST)

if(NOT ${JNI_LIBS_DIR_LIST})
	set(JNI_LIBS_SRC_DIRS "jniLibs.srcDirs += [ '${JNI_LIBS_DIR_LIST}' ]")
endif()

# cmake.path
if(NOT IS_ABSOLUTE ${NATIVE_SCRIPT})
	set(NATIVE_SCRIPT ${CMAKE_SOURCE_DIR}/${NATIVE_SCRIPT})
endif()

if(EXISTS ${NATIVE_SCRIPT})
	file(RELATIVE_PATH NATIVE_SCRIPT_TMP ${OUTPUT_DIR}/app ${NATIVE_SCRIPT})

	set(CMAKE_PATH "cmake {\n\t\t\tpath '${NATIVE_SCRIPT_TMP}'\n\t\t\tbuildStagingDirectory \'build-native\'\n\t\t} ")
endif()

# cmake.arguments
set(ARGS_LIST)

foreach(NATIVE_ARG ${NATIVE_ARGUMENTS})
	list(APPEND ARGS_LIST "-D${NATIVE_ARG}")
endforeach()

list(JOIN ARGS_LIST "', '" ARGS_LIST)

if(NOT ${ARGS_LIST} AND EXISTS ${NATIVE_SCRIPT})
	set(CMAKE_ARGUMENTS "cmake {\n\t\t\t\t${NDK_ABI_FILTERS}\n\t\t\t\targuments '${ARGS_LIST}'\n\t\t\t}")
endif()

file(MAKE_DIRECTORY ${OUTPUT_DIR}/gradle/wrapper)
file(MAKE_DIRECTORY ${OUTPUT_DIR}/app)
configure_file(${GRADLE_BUILD_FILE} ${OUTPUT_DIR}/build.gradle)
configure_file(${GRADLE_APP_BUILD_FILE} ${OUTPUT_DIR}/app/build.gradle)
configure_file(${GRADLE_SETTINGS_FILE} ${OUTPUT_DIR}/settings.gradle)
configure_file(${GRADLE_PROPERTIES_FILE} ${OUTPUT_DIR}/gradle.properties)
configure_file(${GRADLE_LOCAL_PROPERTIES_FILE} ${OUTPUT_DIR}/local.properties)
configure_file(${GRADLE_WRAPPER_PROPERTIES_FILE} ${OUTPUT_DIR}/gradle/wrapper/gradle-wrapper.properties)
file(COPY ${GRADLE_WRAPPER_PROPERTIES_JAR} DESTINATION ${OUTPUT_DIR}/gradle/wrapper)
file(COPY ${GRADLE_WRAPPER_SCRIPT_LINUX} DESTINATION ${OUTPUT_DIR})
file(COPY ${GRADLE_WRAPPER_SCRIPT_WIN} DESTINATION ${OUTPUT_DIR})

message(STATUS "Android Gradle Project (With Native Support) generated at:\n\t${OUTPUT_DIR}")
