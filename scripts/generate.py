#!/usr/bin/env python

# Copyright 2023-2026, Thomas Atkinson
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

    parser.add_argument(
        "--readme-description",
        type=str,
        default=None,
        help="One-line description to put under the new entry in the category README (if omitted, you will be prompted)",
    )

def prompt_if_path_exists(output_dir, name):
    # Match the folder naming performed by the CMake script
    path = os.path.join(output_dir, slugify(name))

    if os.path.exists(path):
        yes = {'yes', 'y'}
        no = {'no', 'n', ''}
        print(terminal_colors.WARNING
              + "The output directory {path} is not empty. Would you like to overwrite its content with the sample template? [y/N]"
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

        # Pick template (with fallback if HPP template is not available)
        template_dir = os.path.join(ROOT_DIR, "bldsys", "cmake", "template", template)
        template_to_use = template
        if not os.path.isdir(template_dir):
            if template == "sample_hpp":
                print(
                    terminal_colors.WARNING
                    + "HPP template 'sample_hpp' not found, falling back to 'sample_api'. The generated sample will not be Vulkan-Hpp based unless you provide the template."
                    + terminal_colors.END
                )
                template_to_use = "sample_api"
            else:
                print(terminal_colors.ERROR + "Template '{template}' not found at {template_dir}" + terminal_colors.END)
                sys.exit(1)

        check_output(
            [
                "cmake",
                "-DSAMPLE_NAME={}".format(args.name),
                "-DTEMPLATE_NAME={}".format(template_to_use),
                "-DOUTPUT_DIR={}".format(output_dir),
                "-P",
                os.path.join(ROOT_DIR, "bldsys", "cmake", "create_sample_project.cmake"),
            ]
        )

        # After sample creation, mandatorily update navigation and docs
        sample_dir_name = slugify(args.name)
        category = args.category
        try:
            add_entry_to_antora_nav(category, sample_dir_name, template_to_use)
        except Exception as e:
            print(
                terminal_colors.ERROR
                + "Failed to update Antora nav.adoc: {e}"
                + terminal_colors.END
            )
            sys.exit(1)

        try:
            ensure_category_readme_entry(category, sample_dir_name, template_to_use, args)
        except Exception as e:
            print(
                terminal_colors.ERROR
                + "Failed to update category README: {e}"
                + terminal_colors.END
            )
            sys.exit(1)

    return func


def slugify(name: str) -> str:
    """
    Convert SampleName (or arbitrary string) to the repository's sample folder/file
    format (snake_case lowercase), matching the CMake conversion logic.
    """
    import re

    # Insert underscore before capitals and lower the result
    result = re.sub(r"(.)([A-Z][a-z]+)", r"\1_\2", name)
    result = re.sub(r"([a-z0-9])([A-Z])", r"\1_\2", result)
    return result.replace(" ", "_").lower()


def add_entry_to_antora_nav(category: str, sample_dir_name: str, template: str) -> None:
    """
    Adds an entry to antora/modules/ROOT/nav.adoc for the given sample.
    Tries to follow existing conventions. If the sample appears to be a Vulkan-Hpp
    sample (name or template), it will try to nest it under the corresponding base sample.
    """
    nav_path = os.path.join(ROOT_DIR, "antora", "modules", "ROOT", "nav.adoc")
    if not os.path.isfile(nav_path):
        raise FileNotFoundError("nav.adoc not found at {nav_path}")

    with open(nav_path, "r", encoding="utf-8") as f:
        lines = f.readlines()

    # Early exit if entry already exists
    target_ref = "xref:samples/{category}/{sample_dir_name}/README.adoc"
    if any(target_ref in line for line in lines):
        print(terminal_colors.INFO + "Antora nav already contains an entry for this sample, skipping." + terminal_colors.END)
        return

    # Determine if this is an HPP sample
    is_hpp = template.endswith("hpp") or sample_dir_name.startswith("hpp_")

    # Build a human-friendly title
    title_base = sample_dir_name.replace("_", " ").strip().title()
    if is_hpp:
        nav_title = "{title_base} (Vulkan-Hpp)"
    else:
        nav_title = title_base

    # Find the category block header
    cat_header = "* xref:samples/{category}/README.adoc"
    try:
        cat_index = next(i for i, l in enumerate(lines) if l.strip().startswith(cat_header))
    except StopIteration:
        raise RuntimeError("Could not find category section for '{category}' in nav.adoc")

    # Find insertion range within this category (until next top-level '* xref:' or EOF)
    insert_start = cat_index + 1
    insert_end = len(lines)
    for i in range(insert_start, len(lines)):
        stripped = lines[i].lstrip()
        # Detect the next top-level category header: a single '* ' at start
        if stripped.startswith("* xref:") and lines[i].startswith("* "):
            insert_end = i
            break

    # Try to nest HPP entries under their base sample if possible
    insert_line = None
    if is_hpp:
        base = sample_dir_name[len("hpp_") :] if sample_dir_name.startswith("hpp_") else sample_dir_name
        base_match = "xref:samples/{category}/{base}/README.adoc"
        for i in range(insert_start, insert_end):
            if base_match in lines[i]:
                # Find last contiguous '***' after this base to keep grouping
                j = i + 1
                last_hpp = i
                while j < insert_end and lines[j].lstrip().startswith("*** "):
                    last_hpp = j
                    j += 1
                insert_line = last_hpp + 1
                break

    # Compose the new entry line, default to level '**'
    new_entry = "** {target_ref}[{nav_title}]\n"
    if is_hpp and insert_line is not None:
        new_entry = "*** {target_ref}[{nav_title}]\n"

    if insert_line is None:
        insert_line = insert_end

    lines.insert(insert_line, new_entry)

    with open(nav_path, "w", encoding="utf-8") as f:
        f.writelines(lines)

    print(terminal_colors.SUCCESS + "Updated Antora nav: added {'nested ' if new_entry.startswith('***') else ''}entry for {sample_dir_name} under '{category}'." + terminal_colors.END)


def ensure_category_readme_entry(category: str, sample_dir_name: str, template: str, args) -> None:
    """
    Ensures a section exists in samples/<category>/README.adoc for the new sample.
    Prompts the user for a one-line description if not provided via --readme-description.
    """
    readme_path = os.path.join(ROOT_DIR, "samples", category, "README.adoc")
    if not os.path.isfile(readme_path):
        raise FileNotFoundError("Category README not found at {readme_path}")

    with open(readme_path, "r", encoding="utf-8") as f:
        content = f.read()

    # Determine the xref variable used by this category README
    var_map = {
        "api": "api_samplespath",
        "extensions": "extension_samplespath",
        "performance": "performance_samplespath",
        "general": "general_samplespath",
        "tooling": "tooling_samplespath",
    }
    xref_var = var_map.get(category, None)
    if not xref_var:
        raise RuntimeError("Unsupported or unknown category '{category}' for README update.")

    target_ref = "xref:./{{{xref_var}}}{sample_dir_name}/README.adoc"
    if target_ref in content:
        print(terminal_colors.INFO + "Category README already contains an entry for this sample, skipping." + terminal_colors.END)
        return

    # Title for the entry
    title_base = sample_dir_name.replace("_", " ").strip().title()
    is_hpp = template.endswith("hpp") or sample_dir_name.startswith("hpp_")
    title = "HPP {title_base}" if is_hpp else title_base

    # Determine description
    desc = args.readme_description
    if desc is None:
        try:
            user_in = input(
                terminal_colors.INFO
                + "Enter a one-line description for the category README entry (leave empty to use a placeholder):\n> "
                + terminal_colors.END
            ).strip()
        except Exception:
            user_in = ""
        desc = user_in if user_in else "TODO: Add a description for {title}."

    # Build the new section to append at the end of README
    new_section = (
        "\n=== {target_ref}[{title}]\n\n{desc}\n\n"
    )

    with open(readme_path, "a", encoding="utf-8") as f:
        f.write(new_section)

    print(terminal_colors.SUCCESS + "Updated {category}/README.adoc with a new entry for {sample_dir_name}." + terminal_colors.END)


if __name__ == "__main__":
    argument_parser = argparse.ArgumentParser(description="Generate utility.")

    subparsers = argument_parser.add_subparsers(
        help="Commands",
    )

    generate_android_gradle_help(subparsers)
    generate_sample_help(subparsers, "sample")
    generate_sample_help(subparsers, "sample_api")
    # Add support for generating a Vulkan-Hpp based sample template if provided
    # by the CMake templates (bldsys/cmake/template/sample_hpp)
    generate_sample_help(subparsers, "sample_hpp")

    args = argument_parser.parse_args()

    if len(sys.argv) == 1:
        argument_parser.print_help(sys.stderr)
        sys.exit(1)

    args.func(args)
