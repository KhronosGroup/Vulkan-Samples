# Copyright (c) 2022, Arm Limited and Contributors
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 the "License";
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

add_library(samples_launcher_lib STATIC src/config.cpp)
target_link_libraries(samples_launcher_lib PUBLIC vkb__platform vkb__encoding)
target_include_directories(samples_launcher_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

vkb__register_tests(
    NAME samples_launcher_test
    SRC
        tests/config.test.cpp
        tests/config_marshaler.test.cpp
    LIBS
        samples_launcher_lib
)

# samples launcher executable
add_executable(samples_launcher src/main.cpp)
target_link_libraries(samples_launcher PUBLIC samples_launcher_lib)

# compile sample information
get_property(DESCRIPTORS GLOBAL PROPERTY SAMPLE_DESCRIPTORS)
list(JOIN DESCRIPTORS "," JSON_ARRAY)
FILE(WRITE "${CMAKE_CURRENT_BINARY_DIR}/samples.json" "{\"samples\": [${JSON_ARRAY}]}")

# copy samples json to binary output dir
add_custom_command(
    TARGET samples_launcher
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_BINARY_DIR}/samples.json"
        "$<TARGET_FILE_DIR:samples_launcher>/samples.json"
)