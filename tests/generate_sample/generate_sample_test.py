'''
Copyright (c) 2019-2023, Arm Limited and Contributors

SPDX-License-Identifier: Apache-2.0

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
'''

import os, platform, subprocess, shutil

generate_sample_script = "bldsys/scripts/generate_sample"
script_path            = os.path.dirname(os.path.realpath(__file__))
root_path              = os.path.join(script_path, "../../")

def add_sample(sample_name, template_name):
    generate_sample_path = os.path.join(root_path, generate_sample_script)
    if platform.system() == "Windows":
        generate_sample_path += ".bat"
    else:
        generate_sample_path += ".sh"

    arguments = [generate_sample_path, sample_name, "api", template_name]
    result = True
    try:
        subprocess.run(arguments, cwd=root_path)
    except FileNotFoundError:
        print("Error: Couldn't find generate sample script")
        result = False
    except:
        print("Error: Couldn't generate sample, script failed")
        result = False
    return result

def clear(platform):
    try:
        os.remove("build/" + platform + "/CMakeCache.txt")
    except OSError:
        pass

def rm_path(path):
    if os.listdir(path):
        shutil.rmtree(path)

def build():
    clear("windows")
    clear("linux")
    clear("mac")

    result = True
    generate_command = ""
    build_command = ""
    if platform.system() == "Windows":
        generate_command = "cmake -G\"Visual Studio 15 2017 Win64\" -H. -Bbuild/windows -DVKB_BUILD_SAMPLES=ON"
        build_command = "cmake --build build/windows --config Release --target vulkan_samples"
    elif platform.system() == "Linux":
        generate_command = "cmake -G \"Unix Makefiles\" -H. -Bbuild/linux -DCMAKE_BUILD_TYPE=Release -DVKB_BUILD_SAMPLES=ON"
        build_command = "cmake --build build/linux --config Release --target vulkan_samples"
    elif platform.system() == "Darwin":
        generate_command = "cmake -H. -Bbuild/mac -DCMAKE_BUILD_TYPE=Release -DVKB_BUILD_SAMPLES=ON"
        build_command = "cmake --build build/mac --config Release --target vulkan_samples"
    else:
        print("Error: Platform not supported")
        return False

    try:
        process = subprocess.run(generate_command, cwd=root_path, shell=True)
        process = subprocess.run(build_command, cwd=root_path, shell=True)
        if process.returncode != 0:
            return False
    except:
        print("Error: Build process failed")
        result = False

    return result

if __name__ == "__main__":
    result = False
    if(add_sample("SampleTest", "sample") and add_sample("SampleTestApi", "sample_api") and build()):
        result = True

    rm_path(os.path.join(root_path, "samples/api/sample_test"))
    rm_path(os.path.join(root_path, "samples/api/sample_test_api"))

    if result:
        print("Generate Sample Test: Passed")
        exit(0)
    else:
        print("Generate Sample Test: Failed")
        exit(1)

