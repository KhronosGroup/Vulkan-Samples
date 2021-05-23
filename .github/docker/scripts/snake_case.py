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
import sys
import subprocess
import os
import re

argv = sys.argv[1:]
try:
    idx = argv.index('--')
except ValueError:
    dash_dash = []
else:
    dash_dash = argv[idx:]
    argv = argv[:idx]

#basic cmd
parser = argparse.ArgumentParser(description="Git Snake Case", formatter_class=argparse.RawDescriptionHelpFormatter)
parser.add_argument('--diff', action='store_true', help='print a diff instead of applying the changes')
parser.add_argument('args', nargs='*', metavar='<commit>', help='revision from which to compute the diff')
opts = parser.parse_args(argv)
branch = opts.args[0]

def main():
    extensions = [".cpp", ".h"]
    files = list()
    out, err = run("git", "diff", branch, "--name-only")
    cpp = out.split("\n")
    for ext in extensions:       
        files = files + list(filter(lambda x: x[-len(ext):] == ext, cpp))
    
    warnings = dict()
    for file in files:
        warnings[file] = list([""])
        print("Processing diff of: " + file)
        out, err = run("git", "diff", "-U0", branch, file)
        lines = out.split("\n")
        warningindex = 0
        for line in lines:
            try:
                testcase = line.strip()[0]
                if(testcase == "@"):
                    warningindex += 1
                    warnings[file].append(str(line + "\n"))
                else:
                    warnings[file][warningindex] = warnings[file][warningindex] + line + "\n"
            except:
                continue
    log = ""
    for file in warnings:
        firstChange = True
        for line in warnings[file]:
            try:
                if(line[0] == "@"):
                    change = line.split("\n")
                    hasChange = False
                    line_num = change[0] + "\n"
                    before = ""
                    after = ""
                    for line in change:
                        try:
                            if(line.strip()[0] == "+"):
                                comparison = process_string(line)
                                if(comparison != line):
                                    hasChange = True
                                    before += "+" + line[1:].strip() + "\n"
                                    after += "?" + comparison[1:].strip() + "\n"
                        except:
                            continue
                    if(hasChange and firstChange):
                        firstChange = False
                        log += "FILE: " + file + "\n"
                    if(hasChange):
                        log += "\033[0;33mWARNING:\n" + line_num + before + after
            except:
                continue
                            


    log = format_report(log)
    print(log)
    print("\n")
    print("\033[0;37m=== Files Checked ===")
    for file in files:
        print(file)

def format_report(report):
    report = report.split("\n")
    for i, line in enumerate(report):
        if(len(line) > 0):
            test = line.strip()
            if(test[0] == "-"):
                report[i] = "\033[0;31m" + line
            elif(test[0] == "+"):
                report[i] = "\033[0;32m" + line
            elif(test[0] == "@"):
                report[i] = "\033[0;36m" + line
            elif(test[0] == "?"):
                report[i] = "\033[0;33m" + line
            else:
                report[i] = "\033[0;37m" + line
    return "\n".join(report)

def process_string(string):
    p = subprocess.Popen(["a.out"], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    p.stdin.write(string)
    out = p.communicate()[0]
    out = convert_string(out)
    return out


def run(*args):
    p = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = p.communicate()
    stdout = convert_string(stdout)
    stderr = convert_string(stderr)

    return stdout, stderr

def to_string(bytes_input):
    if isinstance(bytes_input, str):
        return bytes_input
    return bytes_input.encode('utf-8')

def convert_string(bytes_input):
    try:
        return to_string(bytes_input.decode('utf-8'))
    except AttributeError: # 'str' object has no attribute 'decode'.
        return str(bytes_input)
    except UnicodeError:
        return str(bytes_input)

if __name__ == '__main__':
    main()
