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
import sys
from shutil import which

from subprocess import check_output

# Get the file extension
def get_ext(file_path):
    file_name = os.path.basename(file_path)
    file_name, file_ext = os.path.splitext(file_name)
    return file_ext

class terminal_colors:
    SUCCESS = "\033[92m"
    INFO = "\033[94m"
    WARNING = "\033[33m"
    ERROR = "\033[91m"
    END = "\033[0m"

if __name__ == "__main__":
    argument_parser = argparse.ArgumentParser(
        description="Format C/C++ files using clang-format"
    )
    argument_parser.add_argument(
        "branch",
        type=str,
        default="main",
        nargs="?",
        help="Branch from which to compute the diff",
    )
    args = argument_parser.parse_args()

    if len(sys.argv) == 1:
        argument_parser.print_help(sys.stderr)
        sys.exit(1)

    files = None

    if not which("git"):
        print(terminal_colors.ERROR + "Missing git" + terminal_colors.END)
        sys.exit(1)

    if not which("clang-format"):
        print(terminal_colors.ERROR + "Missing clang-format" + terminal_colors.END)
        sys.exit(1)

    out = check_output(["git", "diff", args.branch, "--name-only"])

    check_files = [".h", ".hpp", ".cpp"]

    files = out.decode("utf-8").split("\n")
    files = [f for f in files if f and get_ext(f) in check_files]

    if files and len(files) > 0:
        print(terminal_colors.INFO + "Formatting files:" + terminal_colors.END)
        for f in files:
            print(terminal_colors.INFO + "  " + f + terminal_colors.END)
        print()

        for f in files:
            if os.path.isfile(f):
                check_output(["clang-format", "-i", f])
    else:
        print(terminal_colors.INFO + "No files to format" + terminal_colors.END)
        
        
        