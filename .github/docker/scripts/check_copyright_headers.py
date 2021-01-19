#!/usr/bin/env python

# Copyright (c) 2021, Arm Limited and Contributors
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
import re
import sys
import datetime
import io
from distutils.spawn import find_executable

from subprocess import check_output

copyright_pattern = re.compile(
    r"\bCopyright \(c\)[^a-zA-Z]*\b\d{4}\b",
    re.IGNORECASE)

year_pattern = re.compile(r"\b\d{4}\b")

file_exceptions = [
    ".copyrightignore"
]


class terminal_colors:
    INFO = '\033[94m'
    WARNING = '\033[33m'
    ERROR = '\033[91m'
    END = '\033[0m'


def get_ext(file_path):
    file_name = os.path.basename(file_path)
    file_name, file_ext = os.path.splitext(file_name)

    return file_ext


def query_files(query, files):
    failed_queries = {}
    queries = {}

    for filename in files:
        with io.open(filename, "r+", encoding="utf-8") as f:
            try:
                matches = re.findall(query, f.read())
                queries[filename] = matches if len(matches) > 0 else None
            except re.error as error:
                failed_queries[filename] = error

    return queries, failed_queries


def check_files(check_files):
    queries, failures = query_files(copyright_pattern, check_files)

    current_year = datetime.datetime.now().year

    missing = []
    outdated = {}

    for filename, tokens in queries.items():

        if not tokens:
            missing.append(filename)
            continue

        outdated[filename] = None

        for token in tokens:
            copyright_years = re.findall(year_pattern, token)

            most_recent_year = int(
                copyright_years[-1]) if copyright_years else 0

            if (most_recent_year != current_year):
                if not outdated[filename]:
                    outdated[filename] = []
                outdated[filename].append(token)

        outdated = {k: v for k, v in outdated.items() if v is not None}

    if len(failures) > 0:
        print(terminal_colors.ERROR +
              "Failed to search:" + terminal_colors.END)

        for filename, error in failures.items():
            print(filename)

        print()

    if len(missing) > 0:
        print(terminal_colors.ERROR +
              "Missing copyright:" + terminal_colors.END)

        for filename in missing:
            print(filename)

        print()

    if len(outdated) > 0:
        print(terminal_colors.ERROR +
              "Outdated copyright:" + terminal_colors.END)

        for filename, tokens in outdated.items():
            print(filename)
            for token in tokens:
                print("\t", terminal_colors.WARNING +
                      token + terminal_colors.END)

        print()

    print("\n=== Files Checked ===")
    for filename in check_files:
        print(terminal_colors.INFO + filename + terminal_colors.END)

    if len(outdated) > 0 or len(missing) > 0:
        sys.exit(-1)
    else:
        sys.exit(0)


if __name__ == "__main__":
    argument_parser = argparse.ArgumentParser(
        description="Check that modified files include copyright headers with current year.")
    argument_parser.add_argument(
        "branch", type=str, help="Branch from which to compute the diff")
    args = argument_parser.parse_args()

    files = None

    if not find_executable("git"):
        print(terminal_colors.ERROR + "Missing git" + terminal_colors.END)
        sys.exit(1)

    try:
        ignored = open(".copyrightignore").readlines()
        for file in ignored:
            file_exceptions.append(file.strip())
    except FileNotFoundError:
        pass

    out = check_output(["git", "diff", args.branch,
                        "--name-only"])

    files = out.decode('utf-8').split("\n")

    if files:
        file_to_check = list(filter(lambda x: os.path.isfile(x) and os.path.basename(
            x) not in file_exceptions and get_ext(x) not in file_exceptions and len(x) > 0, files))

        check_files(file_to_check)
