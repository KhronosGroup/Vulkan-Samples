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

# Creating a new sample <!-- omit in toc -->

This document will explain how to create the necessary files to start working on your own sample.

- [Creating](#create)
- [Configuring](#configure)

## Create
To create a new sample, run the `generate_sample` script that exists within `bldsys/scripts`. There is a batch script for Windows, and a shell script for Unix based systems. 

#### Usage

```
generate_sample <sample_id> <category> [<create_path>]
```

* `<sample_id>` is the id tag that your sample will have (e.g. '`my_new_sample`')
* `<category>` is the category this sample will be placed into (e.g. '`performance`')
* `<create_path>` is optional, for deciding *where* your sample gets created. This should generally be left blank as the script will automatically place your sample its category folder.

#### Example

```
generate_sample my_sample category
```

Running the above line will generate the following files:

```
samples/category/my_sample/CMakeLists.txt
samples/category/my_sample/my_sample.cpp
samples/category/my_sample/my_sample.h
```

## Configure
To configure how your sample will appear, you can modify the `CMakeLists.txt` within the generated directory:
* `NAME` the string that will appear as the title of the sample in the sample list, and the header in the GUI for the sample
* `DESCRIPTION` the string that will appear under the samples title in the sample list

To change the order of the samples (or place your sample in a specific place), modify the `ORDER_LIST` list inside `samples/CMakeLists.txt`. Just place a string of your `<sample_id>` where you would like it to be placed relative to the other samples.

If you would like to show different configurations of your sample during batch mode, you will need to insert these configurations in the constructor of your sample (inside `samples/category/my_sample.cpp`).

e.g. `get_configuration().insert<vkb::IntSetting>(0, my_sample_value, 3);`
