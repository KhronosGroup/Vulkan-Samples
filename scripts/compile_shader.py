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
import hashlib
import json
import os
from shutil import which
import sys
import subprocess
from typing import Dict, List

PARENT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

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

def load_variants(variants_file) -> List[List[str]]:
    variants_json = {}
    with open(variants_file, "r") as f:
        contents = f.read()
        variants_json = json.loads(contents)

    if not "variants" in variants_json or not isinstance(variants_json["variants"], list):
        return []
    
    variants = []
    for variant in variants_json["variants"]:
        if not isinstance(variant["defines"], list):
            continue
        variants.append(variant["defines"])

    return variants
    

def process_hlsl_shader(input_file, output_file, stage) -> bool:
    res = subprocess.run([HLSL_COMPILER_EXECUTABLE, "-fspv-target-env=vulkan1.3", "-T", deduce_hlsl_profile(stage), "-E", "main", "-Fo", output_file, "-spirv", input_file])

    if res.returncode != 0:
        print("ERROR: shader compilation failed")
        return False
    return True

def process_glsl_shader(input_file, output_file, stage) -> bool:
    # compile the shader
    res = subprocess.run([GLSL_COMPILER_PATH, "-fshader-stage={}".format(stage), input_file, "--target-env=vulkan1.3", "-o", output_file])

    if res.returncode != 0:
        print("ERROR: shader compilation failed")
        return False
    return True

def validate_spirv_shader(spirv_file) -> bool:
    res = subprocess.run([SPIRV_VALIDATOR_PATH, spirv_file])

    # check that the output file was created
    if res.returncode != 0:
        print("ERROR: SPIR-V shader validation failed")
        return False
    return True

def merge_atlas_file(atlas_file, atlas) -> bool:
    if not os.path.isfile(atlas_file):
        make_dir_if_not_exists(atlas_file)
        with open(atlas_file, "w") as f:
            f.write(json.dumps(atlas, indent=4))
        return True

    atlas_json = {}
    with open(atlas_file, "r") as f:
        contents = f.read()
        try:
            atlas_json = json.loads(contents)
        except:
            atlas_json = {}

    if not isinstance(atlas_json, dict):
        print("ERROR: atlas file is not valid")
        return False
    
    for key in atlas:
        atlas_json[key] = atlas[key]
    
    with open(atlas_file, "w") as f:
        f.write(json.dumps(atlas_json, indent=4))
    
    return True


if __name__ == "__main__":
    check_required_executables()

    # parse the command line
    parser = argparse.ArgumentParser()
    parser.add_argument("input_file", help="path to the shader source file")
    parser.add_argument("output_file", help="path to the compiled shader file")
    parser.add_argument("--language", help="shader language", choices=["glsl", "hlsl"], default="glsl", required=True)
    parser.add_argument("--variants", help="path to the shader variants file", required=True)
    parser.add_argument("--atlas", help="path to the atlas file", required=True)
    args = parser.parse_args()

    if not os.path.isfile(args.variants):
        print("ERROR: shader variants file does not exist")
        sys.exit(1)
    
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

    variants = load_variants(args.variants)

    variant_entry = {}

    # remove PARENT_DIR from the input file path

    rel_input_file = os.path.relpath(args.input_file, PARENT_DIR)
    rel_output_file = os.path.relpath(args.output_file, PARENT_DIR)


    for defines in variants:
        define_hash = hashlib.sha256("".join(defines).encode('utf-8')).hexdigest()
        output_file = rel_output_file.replace(".spv", ".{}.spv".format(define_hash))

        result = False
        if args.language == "hlsl":
            result = process_hlsl_shader(rel_input_file, output_file, stage)
        elif args.language == "glsl":
            result = process_glsl_shader(rel_input_file, output_file, stage)
        else:
            print("ERROR: unsupported shader language")
            pass

        if not result:
            pass
            
        # check that the output file was created
        if not os.path.isfile(output_file):
            print("ERROR: output file was not created")
            pass

        # validate the compiled shader
        if not validate_spirv_shader(output_file):
            print("ERROR: SPIR-V shader validation failed")
            pass

        variant_entry[define_hash] = {
            "defines": defines,
            "file": output_file
        }

    atlas_entry = {
        "variants": variant_entry
    }

    atlas = {
        rel_input_file: atlas_entry
    }

    # append atlas file 
    if not merge_atlas_file(args.atlas, atlas):
        sys.exit(1)
