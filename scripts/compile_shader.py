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

SUPPORTED_SHADER_STAGES = [
    "vert",
    "tesc",
    "tese",
    "geom",
    "frag",
    "comp",
    "rchit",
    "rahit",
    "rmiss",
    "rint",
    "rcall",
    "rgen",
    "task", 
    "mesh"]

def deduce_hlsl_profile(stage):
    if stage == "vert":
        return "vs_6_0"
    elif stage == "tesc":
        return "hs_6_0"
    elif stage == "tese":
        return "ds_6_0"
    elif stage == "geom":
        return "gs_6_0"
    elif stage == "frag":
        return "ps_6_0"
    elif stage == "comp":
        return "cs_6_0"
    else:
        print("ERROR: unsupported shader stage")
        sys.exit(1)

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
    

def process_hlsl_shader(args, stage):
    res = subprocess.run([HLSL_COMPILER_EXECUTABLE, "-fspv-target-env=vulkan1.3", "-T", deduce_hlsl_profile(stage), "-E", "main", "-Fo", args.output_file, "-spirv", args.input_file])

    if res.returncode != 0:
        print("ERROR: shader compilation failed")
        sys.exit(1)

    # exit with success
    sys.exit(0)

def process_glsl_shader(args, stage):
    # compile the shader
    res = subprocess.run([GLSL_COMPILER_PATH, "-fshader-stage={}".format(stage), args.input_file, "--target-env=vulkan1.3", "-o", args.output_file])

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

     # get the file extension of the input file
    file = os.path.split(args.input_file)[1]
    parts = file.split(".")

    if len(parts) != 3:
        print("ERROR: input file name is not valid (must be <name>.<stage>.<langauge>)")
        sys.exit(1)

    stage = parts[1]
    langauge = parts[2]

    # check that the input file extension is supported
    if stage not in SUPPORTED_SHADER_STAGES:
        print("ERROR: input file extension not supported")
        sys.exit(1)

    if args.language != langauge:
        print("ERROR: input file extension does not match language")
        sys.exit(1)

    if args.language == "hlsl":
        process_hlsl_shader(args, stage)
    elif args.language == "glsl":
        process_glsl_shader(args, stage)
    else:
        print("ERROR: shader language not supported")
        sys.exit(1)

    # check that the output file was created
    if not os.path.isfile(args.output_file):
        print("ERROR: output file was not created")
        sys.exit(1)

    # validate the compiled shader
    validate_spirv_shader(args)
