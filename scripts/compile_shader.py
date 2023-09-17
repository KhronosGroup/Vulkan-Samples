#!/usr/bin/env python

# Copyright 2023, Thomas Atkinson
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 the "License";
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import argparse
import os
from shutil import which
import sys
import subprocess

GLSL_COMPILER_EXECUTABLE = "glslc"
HLSL_COMPILER_EXECUTABLE = "dxc"
SPIRV_VALIDATOR_EXECUTABLE = "spirv-val"

SUPPORTED_GLSL_EXTENSIONS = [".vert", ".tesc", ".tese", ".geom", ".frag", ".comp", ".rchit", ".rahit", ".rmiss", ".rint", ".rcall", ".rgen", ".task", ".mesh"]

def make_dir_if_not_exists(file):
    dir = os.path.dirname(file)
    if not os.path.exists(dir):
        os.makedirs(dir)

def check_required_executables():
    execs = [GLSL_COMPILER_EXECUTABLE, HLSL_COMPILER_EXECUTABLE, SPIRV_VALIDATOR_EXECUTABLE]
    for exec in execs:
        if not which(exec):
            print("ERROR: {} executable not found".format(exec))
            sys.exit(1)

    global GLSL_COMPILER_PATH
    GLSL_COMPILER_PATH = which(GLSL_COMPILER_EXECUTABLE)

    global HLSL_COMPILER_PATH
    HLSL_COMPILER_PATH = which(HLSL_COMPILER_EXECUTABLE)

    global SPIRV_VALIDATOR_PATH
    SPIRV_VALIDATOR_PATH = which(SPIRV_VALIDATOR_EXECUTABLE)
    

def process_hlsl_shader(args):
    print("ERROR: HLSL shader compilation not supported - yet")
    sys.exit(1)

def process_glsl_shader(args):
    # get the file extension of the input file
    input_file_ext = os.path.splitext(args.input_file)[1]

    # check that the input file extension is supported
    if input_file_ext not in SUPPORTED_GLSL_EXTENSIONS:
        print("ERROR: input file extension not supported")
        sys.exit(1)

    # compile the shader
    res = subprocess.run([GLSL_COMPILER_PATH, args.input_file, "--target-env=vulkan1.3", "-o", args.output_file])

    if res.returncode != 0:
        print("ERROR: shader compilation failed")
        sys.exit(1)

    # exit with success
    sys.exit(0)

def validate_spirv_shader(args):
    res = subprocess.run([SPIRV_VALIDATOR_PATH, args.output_file])

    # check that the output file was created
    if res.returncode != 0:
        print("ERROR: SPIR-V shader validation failed")
        sys.exit(1)

    # exit with success
    sys.exit(0)


if __name__ == "__main__":
    check_required_executables()

    # parse the command line
    parser = argparse.ArgumentParser()
    parser.add_argument("input_file", help="path to the shader source file")
    parser.add_argument("output_file", help="path to the compiled shader file")
    parser.add_argument("--language", help="shader language", choices=["glsl", "hlsl"], default="glsl")
    args = parser.parse_args()
    
    # check that the input file exists
    if not os.path.isfile(args.input_file):
        print("ERROR: input file does not exist")
        sys.exit(1)

    make_dir_if_not_exists(args.output_file)

    if args.language == "hlsl":
        process_hlsl_shader(args)
    elif args.language == "glsl":
        process_glsl_shader(args)
    else:
        print("ERROR: shader language not supported")
        sys.exit(1)

    # check that the output file was created
    if not os.path.isfile(args.output_file):
        print("ERROR: output file was not created")
        sys.exit(1)

    # validate the compiled shader
    validate_spirv_shader(args)
