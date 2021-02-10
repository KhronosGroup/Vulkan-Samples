<!--
    Copyright (c) 2021, Arm Limited and Contributors
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
-->

# Vulkan Samples Docker Image

This directory contains all the files required to build the `ghcr.io/KhronosGroup/vulkan-samples` docker image.

# Scripts

vk_convert.lex - An adaptation of a camal case to snake case converter originally written by [Rodrigo Bernardo](https://github.com/rodamber/camel-case-to-snake-case)
snake_case.py - A helper script to run the lexer on a subset of files
run-clang-tidy.py - A helper script that can be found in [clang-tools](https://clang.llvm.org/extra/doxygen/run-clang-tidy_8py_source.html)
check_copyright_headers.py - A script used to check that each file contains the correct up to date copyright information
clang_format.py - A helper script used to run clang format over a subset of files [git-clang-format](https://github.com/llvm-mirror/clang/blob/master/tools/clang-format/git-clang-format)