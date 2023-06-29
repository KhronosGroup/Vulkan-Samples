#!/usr/bin/env python

# Copyright 2021-2023, Arm Limited and Contributors
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
from shutil import which

from subprocess import check_output

# Patterns to search for in the files - Order is important!
COPYRIGHT_PATTERNS = [
    re.compile(r"\bCopyright \(c\)[^a-zA-Z0-9]*\b\d{4}-\d{4}\b", re.IGNORECASE),
    re.compile(r"\bCopyright[^a-zA-Z0-9]*\b\d{4}-\d{4}\b", re.IGNORECASE),
    re.compile(r"\bCopyright \(c\)[^a-zA-Z0-9]*\b\d{4}\b", re.IGNORECASE),
    re.compile(r"\bCopyright[^a-zA-Z0-9]*\b\d{4}\b", re.IGNORECASE),
]

YEAR_PATTERN = re.compile(r"\b\d{4}\b")
YEAR_RANGE_PATTERN = re.compile(r"\b\d{4}-\d{4}\b")

EXCEPTION_FILE = ".copyrightignore"

class terminal_colors:
    SUCCESS = "\033[92m"
    INFO = "\033[94m"
    WARNING = "\033[33m"
    ERROR = "\033[91m"
    END = "\033[0m"

# Get the file extension
def get_ext(file_path):
    file_name = os.path.basename(file_path)
    file_name, file_ext = os.path.splitext(file_name)
    return file_ext

# Query files for the presence of a valid year in the header
def query_files(files):
    failed_queries = {}
    queries = {}
    
    print("\n lrm query_files")

    for filename in files:
        print("\n lrm filename %s", filename)
        with io.open(filename, "r+", encoding="utf-8") as f:
            queries[filename] = None
            file_contents = f.read()
            for query in COPYRIGHT_PATTERNS:
                try:
                    matches = re.findall(query, file_contents)
                    if len(matches) > 0:
                        queries[filename] = matches
                        break # Stop searching after the first match
                except re.error as error:
                    failed_queries[filename] = error

    return queries, failed_queries

# Check the files for the presence of a valid year in the header
def check_files(check_files):
    queries, failures = query_files(check_files)

    print("\n lrm check files")
    current_year = datetime.datetime.now().year

    missing = []
    outdated = {}

    for filename, tokens in queries.items():
        if not tokens:
            missing.append(filename)
            continue

        outdated[filename] = None

        for token in tokens:
            copyright_years = re.findall(YEAR_PATTERN, token)

            most_recent_year = int(copyright_years[-1]) if copyright_years else 0

            if most_recent_year != current_year:
                if not outdated[filename]:
                    outdated[filename] = []
                outdated[filename].append(token)

        outdated = {k: v for k, v in outdated.items() if v is not None}
    return missing, outdated, failures

def fix(file):
    queries, failures = query_files([file])

    current_year = datetime.datetime.now().year


    for filename, tokens in queries.items():
        if not tokens:
            continue

        for token in tokens:
            year_range = YEAR_RANGE_PATTERN.search(token)
            if year_range:
                year_range = year_range.group(0)
                start_year, end_year = year_range.split("-")
                if int(end_year) != current_year:
                    fixed_token = token.replace(year_range, start_year + "-" + str(current_year))
                    with io.open(filename, "r+", encoding="utf-8") as f:
                        file_contents = f.read()
                        file_contents = file_contents.replace(token, fixed_token)
                        f.seek(0)
                        f.write(file_contents)
                        f.truncate(
                            f.tell()
                        )
                        continue

            year = YEAR_PATTERN.search(token)
            if year:
                year = year.group(0)
                if int(year) != current_year:
                    fixed_token = token + "-" + str(current_year)
                    with io.open(filename, "r+", encoding="utf-8") as f:
                        file_contents = f.read()
                        file_contents = file_contents.replace(token, fixed_token)
                        f.seek(0)
                        f.write(file_contents)
                        f.truncate(
                            f.tell()
                        )  # Truncate the file to the current position of the file pointer

if __name__ == "__main__":
    argument_parser = argparse.ArgumentParser(
        description="Check that modified files include copyright headers with current year."
    )
    argument_parser.add_argument(
        "branch",
        type=str,
        default="main",
        nargs="?",
        help="Branch from which to compute the diff",
    )
    argument_parser.add_argument(
        "--fix",
        action="store_true",
        help="Fix the files that are missing the header",
        default=False,
    )
    args = argument_parser.parse_args()

    if len(sys.argv) == 1:
        argument_parser.print_help(sys.stderr)
        sys.exit(1)

    files = None

    if not which("git"):
        print(terminal_colors.ERROR + "Missing git" + terminal_colors.END)
        sys.exit(1)

    file_exceptions = [EXCEPTION_FILE]

    try:
        ignored = open(EXCEPTION_FILE).readlines()
        for file in ignored:
            file_exceptions.append(file.strip())
    except FileNotFoundError:
        pass

    out = check_output(["git", "diff", args.branch, "--name-only"])
 
    files = out.decode("utf-8").split("\n")

    files_to_check = list(
        filter(
            lambda x: os.path.isfile(x)
            and os.path.basename(x) not in file_exceptions
            and get_ext(x) not in file_exceptions
            and len(x) > 0,
            files,
        )
    )

    if files_to_check and len(files_to_check) > 0:
        missing, outdated, failures = check_files(files_to_check)

        if len(failures) > 0:
            print(terminal_colors.ERROR + "Failed to search:" + terminal_colors.END)

            for filename, error in failures.items():
                print(filename)

            print()

        if len(missing) > 0:
            print(terminal_colors.ERROR + "Missing copyright:" + terminal_colors.END)

            for filename in missing:
                print(filename)

            print()

        if len(outdated) > 0:
            print(terminal_colors.ERROR + "Outdated copyright:" + terminal_colors.END)

            for filename, tokens in outdated.items():
                if args.fix:
                    fix(filename)
                    print(terminal_colors.SUCCESS + "Fixed " + filename + terminal_colors.END)
                else:
                    print(filename)
                    for token in tokens:
                        print("\t", terminal_colors.WARNING + token + terminal_colors.END)

            print()

        print("\n=== Files Checked ===")
        for filename in files_to_check:
            print(terminal_colors.INFO + filename + terminal_colors.END)

        if len(outdated) > 0 or len(missing) > 0:
            sys.exit(-1)
        else:
            sys.exit(0)
    else:
        print(terminal_colors.INFO + "No files found" + terminal_colors.END)