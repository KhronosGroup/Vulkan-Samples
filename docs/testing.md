<!--
- Copyright (c) 2019-2020, Arm Limited and Contributors
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

# Testing Guides

## Contents 
- [Testing Guides](#testing-guides)
  - [Contents](#contents)
  - [System Test](#system-test)
    - [Android](#android)
  - [Generate Sample Test](#generate-sample-test)
      - [To run](#to-run)

## System Test
In order for the script to work you will need to install and add to your Path:
* `Python 3.x`
* `imagemagick`
* `git`
* `cmake` 
* (Optional) `adb` if you plan to use Android

You will also need to have built the Vulkan Best Practices in 64 bit, with the CMake flag `VKB_BUILD_SAMPLES`, `VKB_BUILD_TESTS` and in addition to this install a working `.apk` onto a device if you plan on testing on Android.

**Before you begin a system test on Android, ensure that the device is held in landscape, and there isn't an instance of Vulkan Best Practice running already.**

1. From the root of the project: `cd tests/system_test`
2. To run: `python system_test.py -B <build dir> -C <Debug|Release>`  
2.1. e.g. `python system_test.py -Bbuild/windows -CRelease` (build path is relative to root)  
2.2. To target just testing on desktop, add a `-D` flag, or to target just Android, an `-A` flag. If no flag is specified it will run for both.  
2.3. To run a specific sub test(s), use the `-S` flag (e.g. `python system_test.py ... -S sponza bonza` runs sponza and bonza)  

### Android

We currently support FHD resolutions (2280x1080), if testing on another device or resolution the test may fail.

## Generate Sample Test

There is a test for the `generate_sample` script, to ensure that it generates a sample that builds within the project. 

#### To run
```
cd tests/generate_sample
python generate_sample_test.py
```

It will print out the result of the test