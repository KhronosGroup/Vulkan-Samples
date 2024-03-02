# Copyright (c) 2024, Sascha Willems
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

# Walks trough all sub directories and compiles HLSL shaders to SPIR-V using DXC

import argparse
import os
import subprocess
import sys

parser = argparse.ArgumentParser(description='Compile all .hlsl shaders')
parser.add_argument('--dxc', type=str, help='path to DXC executable')
args = parser.parse_args()

def findDXC():
    def isExe(path):
        return os.path.isfile(path) and os.access(path, os.X_OK)

    if args.dxc != None and isExe(args.dxc):
        return args.dxc

    exe_name = "dxc"
    if os.name == "nt":
        exe_name += ".exe"

    for exe_dir in os.environ["PATH"].split(os.pathsep):
        full_path = os.path.join(exe_dir, exe_name)
        if isExe(full_path):
            return full_path

    sys.exit("Could not find DXC executable on PATH, and was not specified with --dxc")

dxc_path = findDXC()
dir_path = os.path.dirname(os.path.realpath(__file__))
dir_path = dir_path.replace('\\', '/')
for root, dirs, files in os.walk(dir_path):
    for file in files:
        if file.endswith(".hlsl"):
            hlsl_file = os.path.join(root, file)
            spv_out = hlsl_file + ".spv"
            spv_out = spv_out.replace('.hlsl.','.')

            target = ''
            profile = ''
            if(hlsl_file.find('.vert') != -1):
                profile = 'vs_6_1'
            elif(hlsl_file.find('.frag') != -1):
                profile = 'ps_6_4'
            elif(hlsl_file.find('.comp') != -1):
                profile = 'cs_6_1'
            elif(hlsl_file.find('.geom') != -1):
                profile = 'gs_6_1'
            elif(hlsl_file.find('.tesc') != -1):
                profile = 'hs_6_1'
            elif(hlsl_file.find('.tese') != -1):
                profile = 'ds_6_1'
            elif(hlsl_file.find('.rgen') != -1 or
				hlsl_file.find('.rchit') != -1 or
				hlsl_file.find('.rmiss') != -1):
                target='-fspv-target-env=vulkan1.2'
                profile = 'lib_6_3'
            elif(hlsl_file.find('.mesh') != -1):
                target='-fspv-target-env=vulkan1.2'
                profile = 'ms_6_6'

            if(profile == ''):
                continue;

            print('Compiling %s to %s' % (hlsl_file, spv_out))
            subprocess.check_output([
                dxc_path,
                '-spirv',
                '-T', profile,
                '-E', 'main',
                target,
                hlsl_file,
                '-Fo', spv_out])
