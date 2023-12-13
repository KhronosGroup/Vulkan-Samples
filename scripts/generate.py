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
from subprocess import check_output
import sys

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
ROOT_DIR = os.path.join(SCRIPT_DIR, "..")


class terminal_colors:
    SUCCESS = "\033[92m"
    INFO = "\033[94m"
    WARNING = "\033[33m"
    ERROR = "\033[91m"
    END = "\033[0m"


def generate_android_gradle_help(subparsers):
    parser = subparsers.add_parser(
        "android",
        help="Generate Android Gradle files",
    )

    parser.set_defaults(func=generate_android_gradle)

    parser.add_argument(
        "--output-dir",
        type=str,
        help="Relative Output Directory: <project_dir>/build/android_gradle",
        default=None,
    )


def generate_android_gradle(args):
    output_dir = (
        os.path.join(ROOT_DIR, args.output_dir)
        if args.output_dir
        else os.path.join(ROOT_DIR, "build", "android_gradle")
    )

    if not which("cmake"):
        print("Missing cmake")
        sys.exit(1)

    print(
        terminal_colors.INFO
        + "Generating Android Gradle files at "
        + output_dir
        + terminal_colors.END
    )

    check_output(
        [
            "cmake",
            "-DPROJECT_NAME=vulkan_samples",
            "-DANDROID_API=30",
            "-DARCH_ABI=arm64-v8a",
            "-DANDROID_MANIFEST={}".format(
                os.path.join(ROOT_DIR, "app", "android", "AndroidManifest.xml")
            ),
            "-DJAVA_DIRS={}".format(os.path.join(ROOT_DIR, "app", "android", "java")),
            "-DRES_DIRS={}".format(os.path.join(ROOT_DIR, "app", "android", "res")),
            "-DOUTPUT_DIR={}".format(output_dir),
            "-DASSET_DIRS=",
            "-DJNI_LIBS_DIRS=",
            "-DNATIVE_SCRIPT={}".format(os.path.join(ROOT_DIR, "CMakeLists.txt")),
            "-P",
            os.path.join(ROOT_DIR, "bldsys", "cmake", "create_gradle_project.cmake"),
        ]
    )

def generate_sample_help(subparsers, template):
    parser = subparsers.add_parser(
        template,
        help="Generate sample project",
    )

    parser.set_defaults(func=generate_sample(template))

    parser.add_argument(
        "--name",
        type=str,
        help="Name of the sample project",
        default="SampleTest",
    )

    parser.add_argument(
        "--category",
        type=str,
        help="Which category the sample project belongs to",
        default="api",
    )

    parser.add_argument(
        "--output-dir",
        type=str,
        help="Output Directory: <project_dir>/samples/<category>",
        default=None,
    )

def prompt_if_path_exists(output_dir, name):
    path = os.path.join(output_dir, name)

    if os.path.exists(path):
        yes = {'yes', 'y'}
        no = {'no', 'n', ''}
        print(terminal_colors.WARNING
              + f"The output directory {path} is not empty. Would you like to overwrite its content with the sample template? [y/N]"
              + terminal_colors.END)
        while True:
            choice = input().lower()
            if choice in yes:
                return True
            elif choice in no:
                return False
            else:
                print("Please respond with 'y' or 'n'")    
    return True

def generate_sample(template):
    def func(args):
        output_dir = (
            args.output_dir
            if args.output_dir
            else os.path.join(ROOT_DIR, "samples", args.category)
        )

        if not which("cmake"):
            print("Missing cmake")
            sys.exit(1)

        if not prompt_if_path_exists(output_dir, args.name):
            return 

        print(
            terminal_colors.INFO
            + "Generating sample project "
            + args.name
            + terminal_colors.END
        )

        check_output(
            [
                "cmake",
                "-DSAMPLE_NAME={}".format(args.name),
                "-DTEMPLATE_NAME={}".format(template),
                "-DOUTPUT_DIR={}".format(output_dir),
                "-P",
                os.path.join(ROOT_DIR, "bldsys", "cmake", "create_sample_project.cmake"),
            ]
        )

    return func


if __name__ == "__main__":
    argument_parser = argparse.ArgumentParser(description="Generate utility.")

    subparsers = argument_parser.add_subparsers(
        help="Commands",
    )

    generate_android_gradle_help(subparsers)
    generate_sample_help(subparsers, "sample")
    generate_sample_help(subparsers, "sample_api")

    args = argument_parser.parse_args()

    if len(sys.argv) == 1:
        argument_parser.print_help(sys.stderr)
        sys.exit(1)

    args.func(args)
