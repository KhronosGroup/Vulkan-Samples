<!--
- Copyright (c) 2023, Thomas Atkinson
-
- SPDX-License-Identifier: Apache-2.0
-
- Licensed under the Apache License, Version 2.0 the "License";
- you may not use this file except in compliance with the License.
- You may obtain a copy of the License at
-
-     http://www.apache.org/licenses/LICENSE-2.0
-
- Unless required by applicable law or agreed to in writing, software
- distributed under the License is distributed on an "AS IS" BASIS,
- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
- See the License for the specific language governing permissions and
- limitations under the License.
-
-->

# Utility Scripts <!-- omit in toc -->

A series of helpers to make life easier.

## Generate

Helps to generate new files for the project.

### Generate Sample

All flags are optional. Setting --name is advised. If --output-dir is not set, the sample will be generated in the samples directory filed under the specified category.

```bash
./scripts/generate.py sample --name <SampleName> --category <category> --output-dir <output_dir>
```

Running the above line will generate the following files:

```bash
samples/category/my_sample/CMakeLists.txt
samples/category/my_sample/sample_name.cpp
samples/category/my_sample/sample_name.h
```

A new class will also be generated

```cpp
class SampleName ... {
...
};
```

### Generate Android Project

```bash
./scripts/generate.py android
./scripts/generate.py android --output-dir build/<another_folder_name>
```

## Clang Format

When called from the root of the repository, this script will run clang-format on all files in the repository that have been altered in the `git diff`

```bash
./scripts/clang-format.py <brand_to_diff>
```

## Copyright Headers

When called from the root of the repository, this script will check all files in the repository that have been altered in the `git diff` to ensure they have the correct license header.

```bash
./scripts/copyright.py <branch_to_diff>
```

This is similar to the copyright CI check except when run with `--fix` this script will update the license headers in all files in the repository that have been altered in the `git diff`.

```bash
./scripts/copyright.py <branch_to_diff> --fix
```