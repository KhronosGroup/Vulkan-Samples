<!--
- Copyright (c) 2019-2021, Arm Limited and Contributors
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

# Build Guides <!-- omit in toc -->

# Contents <!-- omit in toc -->

- [CMake Options](#cmake-options)
  - [VKB_<sample_name>](#vkb_sample_name)
  - [VKB_SYMLINKS](#vkb_symlinks)
  - [VKB_ENTRYPOINTS](#vkb_entrypoints)
  - [VKB_VALIDATION_LAYERS](#vkb_validation_layers)
  - [VKB_VALIDATION_LAYERS_GPU_ASSISTED](#vkb_validation_layers_gpu_assisted)
  - [VKB_WARNINGS_AS_ERRORS](#vkb_warnings_as_errors)
- [3D models](#3d-models)
- [Performance data](#performance-data)
- [Windows](#windows)
  - [Dependencies](#dependencies)
  - [Build with CMake](#build-with-cmake)
- [Linux](#linux)
  - [Dependencies](#dependencies-1)
  - [Build with CMake](#build-with-cmake-1)
- [macOS](#macos)
  - [Dependencies](#dependencies-2)
  - [Build with CMake](#build-with-cmake-2)
- [Android](#android)
  - [Dependencies](#dependencies-3)
  - [Build with Gradle](#build-with-gradle)
  - [Build with CMake](#build-with-cmake-3)
- [Building Individual Samples](#building-individual-samples)
  - [CMake](#cmake)
  - [Visual Studio](#visual-studio)

# CMake Options

The following options are used to change the build configuration

#### VKB\_<sample_name>

Choose whether to include a sample at build time.

- `ON` - Build Sample
- `OFF` - Exclude Sample

**Default:** `ON`

#### VKB_BUILD_SAMPLES

Choose whether to build the samples.

- `ON` - Build All Samples
- `OFF` - Skip building Samples

**Default:** `ON`

#### VKB_BUILD_TESTS

Choose whether to build the tests

- `ON` - Build All Tests
- `OFF` - Skip building Tests

**Default:** `OFF`

#### VKB_SYMLINKS
Rather than changing the working directory inside the IDE, `VKB_SYMLINKS` will enable symlink creation pointing to the root directory which exposes the assets and outputs folders to the samples.

**Default:** `OFF`

#### VKB_ENTRYPOINTS

Generate a build project for each application so that they can be run separately

**Default:** `OFF`

#### VKB_VALIDATION_LAYERS

Enable Validation Layers

**Default:** `OFF`

#### VKB_VALIDATION_LAYERS_GPU_ASSISTED

Enable GPU assisted Validation Layers, used primarily for VK_EXT_descriptor_indexing.

**Default:** `OFF`

#### VKB_WARNINGS_AS_ERRORS

Treat all warnings as errors

**Default:** `ON`

# 3D models

Most of the samples require 3D models downloaded from <https://github.com/KhronosGroup/Vulkan-Samples-Assets>.
That repository is referenced as a git submodule by this project
so if you followed the clone instructions in the [project readme](../README.md)
you will already have the models locally under `./assets/`.

On Android, Gradle will run CMake which will sync assets to the device if there has been a change.

However, to sync them manually you may run the following command to ensure up to date assets are on the device:

```
adb push --sync assets /sdcard/Android/data/com.khronos.vulkan_samples/files/
adb push --sync shaders /sdcard/Android/data/com.khronos.vulkan_samples/files/
```

# Performance data

In order for performance data to be displayed, profiling needs to be enabled on the device. Some devices may disable it by default.

Profiling can be enabled via adb:

```
adb shell setprop security.perf_harden 0
```

> Performance data is captured using HWCPipe.
> For details on this project and how to integrate it in your pipeline,
> visit: https://github.com/ARM-software/HWCPipe

# Windows

## Dependencies

- CMake v3.10+ (known to work with 3.10.2 and 3.19.3)
- Python 3
- Visual Studio 2017 or above
- [clang-format-8](#clang-format-and-visual-studio)
- [CMake Options](#cmake-options)
- [3D models](#3d-models)

## Clang Format and Visual Studio

Visual Studio comes with `clang-format-6` which is incompatible with some of the styles we use in our `.clang-format` file. It is recommended to point to a `clang-format-8.exe` binary within the in-built clang formatter, or disable it and use a third party extension that is more up to date.

Go to the [LLVM downloads page](http://releases.llvm.org/download.html) to get clang.

## Build with CMake

> Please make sure, when running any sample, that you either:
>
> - Enable [Developer Mode](https://docs.microsoft.com/en-us/windows/uwp/get-started/enable-your-device-for-development "Microsoft Tutorial to Enable Developer Mode 'docs.microsoft.com'")
> - Run Command Prompt or Visual Studio as administrator

`Step 1.` The following command will generate the VS project

```
cmake -G"Visual Studio 15 2017 Win64" -S . -Bbuild/windows
```

(Prior to CMake v3.13)
```
cmake -G"Visual Studio 15 2017 Win64" -H. -Bbuild/windows
```

`Step 2.` Build the Visual Studio project

```
cmake --build build/windows --config Release --target vulkan_samples
```

`Step 3.` Run the **Vulkan Samples** application

```
build\windows\app\bin\Release\AMD64\vulkan_samples.exe
```

# Linux

## Dependencies

- CMake v3.10+ (known to work with 3.10.2)
- C++14 Compiler (tested on GCC 7.3)
- [CMake Options](#cmake-options)
- [3D models](#3d-models)

```
sudo apt-get install cmake g++ xorg-dev libglu1-mesa-dev
```

## Build with CMake

`Step 1.` The following command will generate the project

```
cmake -G "Unix Makefiles" -H. -Bbuild/linux -DCMAKE_BUILD_TYPE=Release
```

`Step 2.` Build the project

```
cmake --build build/linux --config Release --target vulkan_samples -- -j4
```

`Step 3.` Run the **Vulkan Samples** application to display the help message

```
./build/linux/app/bin/Release/x86_64/vulkan_samples --help
```

# macOS

## Dependencies

- CMake v3.10+ (known to work with 3.10.2; Apple Silicon requires at least 3.19.2)
- XCode v12 for Apple Silicon
- Command Line Tools (CLT) for Xcode `xcode-select --install`
- [Vulkan SDK](https://vulkan.lunarg.com/doc/sdk/latest/mac/getting_started.html) `./install_vulkan.py`
- [CMake Options](#cmake-options)
- [3D models](#3d-models)

## Build with CMake

`Step 1.` The following command will generate the project

```
cmake -H. -Bbuild/mac -DCMAKE_BUILD_TYPE=Release
```

`Step 2.` Build the project

```
cmake --build build/mac --config Release --target vulkan_samples -- -j4
```

`Step 3.` Run the **Vulkan Samples** application to display the help message

```
./build/mac/app/bin/Release/x86_64/vulkan_samples --help
```


# Android

## Dependencies

For all dependencies set the following environment variables.

- CMake v3.10+
- JDK 8+ `JAVA_HOME=<SYSTEM_DIR>/java`
- Android NDK r18+ `ANDROID_NDK_ROOT=<WORK_DIR>/android-ndk`
- Android SDK `ANDROID_HOME=<WORK_DIR>/android-sdk`
- Gradle 5+ `GRADLE_HOME=<WORK_DIR>/gradle`
- [CMake Options](#cmake-options)
- [3D models](#3d-models)
- [Performance data](#performance-data)

## Build with Gradle

### Generate the gradle project 

Use the provided script for the platform you are building on by running the following command:

#### Windows <!-- omit in toc -->

```
bldsys\scripts\generate_android_gradle.bat
```

#### Linux <!-- omit in toc -->

```
./bldsys/scripts/generate_android_gradle.sh
```

A new folder will be created in the root directory at `build\android_gradle`

### Build the project

> Prefer a release build for better performance unless you want to actively debug the application.

```
cd build/android_gradle
```

For a release build:

```
gradle assembleRelease
``` 

For a debug build:

```
gradle assembleDebug
``` 

### Install the apk on the device

You can now install the apk on a connected device using the Android Debug Bridge:

For a release build:

```
adb install build/outputs/apk/release/vulkan_samples-release.apk
```
For a debug build:

```
adb install build/outputs/apk/debug/vulkan_samples-debug.apk
```

## Build with Android Studio

> Alternatively, you may import the `build/android_gradle` folder in Android Studio and run the project from here

If you are using a newer version of cmake then 3.13, you might get this error:

> Execution failed for task ':externalNativeBuildDebug'.
Expected output file at \<PATH> for target \<sample> but there was none

In this case, update the version of the gradle plugin in "bldsys/cmake/template/gradle/build.gradle.in" to 3.5.0, remove the content of build folder and repeat the build process from Step 1. This is known to work with Gradle 6.3 and NDK 20.0.55

If you are using Android Studio, you can simply do these changes after importing the `build/android_gradle` folder, opening File->Project Structure, and doing the following changes:
On the Project tab, change the Android Gradle Plugin version to 3.5.0 and the Gradle version to 6.3.(this also requires NDK 20.0.55)

# Building Individual Samples

`Step 1.` When generating cmake set the `VKB_ENTRYPOINTS` flag to `ON`

```
cmake -B"build" -H"." -DVKB_ENTRYPOINTS=ON
```

### CMake

`Step 2.` Set the target to `<samplename>_app`. For example:

```
cmake --build build --config Debug --target afbc_app
```

### Visual Studio

`Step 2.` In the Solution Explorer there now is an Entrypoints folder. Select the `<sample>_app` you would like to run and set it as the `Start Up Project`
