<!--
- Copyright (c) 2019, Arm Limited and Contributors
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

# Contributing

Contributions are encouraged! If you have a sample that demonstrates:
- Vulkan API Usage
- Vulkan Extension Usage
- Vulkan Best Practices and/or Performance Guidance
- Unique Vulkan use case or project

then consider contributing to the Vulkan samples repository.
However, before you start, check out the requirements and guidelines below.
Following these guidelines can help ensure that your contribution ends up being approved by reviewers and most importantly becomes a valuable addition to the Vulkan Samples repository.

## Repository Structure

| folder                  | description                                                           |
|-------------------------|-----------------------------------------------------------------------|
| `/samples/api/`         | folder containing samples that demonstrate API usage                  |
| `/samples/extensions/`  | folder containing samples that demonstrate API Extensions usage       |
| `/samples/performance/` | folder containing samples that demonstrate performance best-practices |
| `/shaders/`             | folder containing shaders used by the samples                         |
| `/assets/`              | GIT sub-module with models, scenes and fonts                          |
| `/third_party/`         | folder with commonly used external libraries                          |


## Creating a sample

Follow [this guide](docs/create_sample.md) to create a dummy sample and associated build files automatically.
This new sample will be based on the framework of wrapper classes that provide an optimized and convenient system to manage Vulkan objects.

## General Requirements

- Sample folder and description:
  - Each sample must be placed in a separate sub-folder within `/samples/api/`, `/samples/performance/` or `/samples/extensions/`.
  - Each sample should use a short folder name, using snake_case, that best describes the sample.
  - Each sample must be well-documented and ideally include a tutorial file in the root of the sample's folder with a detailed explanation of the sample and a 'best-practice' summary if applicable.
    Any images used in the tutorial should be stored in an images/ sub-folder in the sample folder.

- Vendor samples:
  - By default each sample is assumed to run on all supported platforms. Otherwise note any platform restrictions in the sample's documentation.
  - If a sample is vendor-specific (i.e. only runs on certain hardware) please add a `TAG` with the vendor's name in the sample's `CMakeLists.txt`.

- Framework:
  - Make use of the available framework whenever possible.
  - Do not introduce any new wrapper code. If what you need is not already a part of the framework, please extend it rather than introduce anything new.
  - Alternatively you may use raw Vulkan API calls.

- Code and assets:
  - Single source file samples with minimal build complexity are encouraged to make porting to different platforms easier.
  - Compiling the sample with the highest warning level and warnings-as-errors (-Wall -Wextra -Werror, or /Wall /WX) is highly recommended.
  - Shaders are saved in the `/shaders/` folder, in a separate sub-folder with the same name as the sample sub-folder in `/samples/api/`, `/samples/performance/` or `/samples/extensions/`.
  - Assets should be saved in [vulkan-samples-assets](https://github.com/KhronosGroup/Vulkan-Samples-Assets).

- License:
  - Samples are licensed under the [LICENSE](LICENSE) file in the root folder.
  - The current Contributor License Agreement (CLA) only allows samples to be licensed under the Apache 2.0 license.
  - Every source code file must have a Copyright notice and license at the top of the file as described below.
  - Assets must have their own license.

- Third party libraries:
  - A sample may not depend on a separate installation of a third party library.
  - Any third party library that is used needs to be available under a compatible open source license i.e. MIT or Apache 2.0.
  - Any third party library that is used must be included as a sub-module in the `/third_party/` folder.

## Copyright Notice and License Template

To apply the Apache 2.0 License to your work, attach the following boilerplate notice, with
the fields enclosed by brackets "[]" replaced with your own identifying information for
the copyright year or years. *Don't include the brackets!* The text should be enclosed in the appropriate comment
syntax for the file format. We also recommend that a file or class name and description
of purpose be included on the same "printed page" as the copyright notice for easier
identification within third-party archives.
When contributing to an existing file you may add a new copyright year line under the existing ones.

    Copyright [yyyy] [name of copyright owner]

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

## Code Style

A common code style like, for instance, the one described by the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
is recommended. A sample must consistently apply a single code style.

A `.clang-format` file is included with this repository, please use `clang-format -style=file` to verify the code style. Ensure that you are using clang-format 8.

## Procedure for Contributing

  1. Fork the KhronosGroup/Vulkan-Samples repository.
  2. Add the contribution to the new fork (see [Creating a sample](#creating-a-sample)).
  3. Make sure the above requirements are met.
  4. Make sure the sample is in compliance with the Vulkan specification.
  5. Make sure the sample code builds and runs on Windows, Linux, macOS and Android. If you cannot verify on all these target platforms, please note platform restrictions in the sample's README.
  6. Verify the sample against a recent version of the Vulkan validation layers, either built from source or from the most recent available Vulkan SDK.
  7. Submit a pull request for the contribution, including electronically signing the Khronos Contributor License Agreement (CLA) for the repository using CLA-Assistant.

## Code Reviews

All submissions, including submissions by project members, are subject to a code review by the Khronos Membership.
GitHub pull requests are used to facilitate the review process, please submit a pull request with your contribution ready for review.
For more information on the review process visit this [link](https://github.com/KhronosGroup/Vulkan-Samples/wiki/Review-Process).

## Maintenance

Once a new sample is merged the author is expected to maintain it whenever possible.
Otherwise they should identify a new maintainer that has agreed to take on the responsibility.
