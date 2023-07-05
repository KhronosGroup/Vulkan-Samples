<!--
- Copyright (c) 2019-2023, Arm Limited and Contributors
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

## Contents <!-- omit in toc -->

- [CMake Options](#cmake-options)
  - [VKB\_\<sample\_name\>](#vkb_sample_name)
  - [VKB\_BUILD\_SAMPLES](#vkb_build_samples)
  - [VKB\_BUILD\_TESTS](#vkb_build_tests)
  - [VKB\_VALIDATION\_LAYERS](#vkb_validation_layers)
  - [VKB\_VALIDATION\_LAYERS\_GPU\_ASSISTED](#vkb_validation_layers_gpu_assisted)
  - [VKB\_VULKAN\_DEBUG](#vkb_vulkan_debug)
  - [VKB\_WARNINGS\_AS\_ERRORS](#vkb_warnings_as_errors)
- [Quality Assurance](#quality-assurance)
- [3D models](#3d-models)
- [Performance data](#performance-data)
- [Windows](#windows)
  - [Dependencies](#dependencies)
  - [Clang Format and Visual Studio](#clang-format-and-visual-studio)
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
    - [Generate the gradle project](#generate-the-gradle-project)
    - [Install dependencies](#install-dependencies)
    - [Build the project](#build-the-project)
    - [Install the apk on the device](#install-the-apk-on-the-device)
  - [Build with Android Studio](#build-with-android-studio)

## CMake Options

The following options are used to change the build configuration

### VKB\_<sample_name>

Choose whether to include a sample at build time.

- `ON` - Build Sample
- `OFF` - Exclude Sample

**Default:** `ON`

### VKB_BUILD_SAMPLES

Choose whether to build the samples.

- `ON` - Build All Samples
- `OFF` - Skip building Samples

**Default:** `ON`

### VKB_BUILD_TESTS

Choose whether to build the tests

- `ON` - Build All Tests
- `OFF` - Skip building Tests

**Default:** `OFF`

### VKB_VALIDATION_LAYERS

Enable Validation Layers

**Default:** `OFF`

### VKB_VALIDATION_LAYERS_GPU_ASSISTED

Enable GPU assisted Validation Layers, used primarily for VK_EXT_descriptor_indexing.

**Default:** `OFF`

### VKB_VULKAN_DEBUG

Enable VK_EXT_debug_utils or VK_EXT_debug_marker, if supported.
This enables debug names for Vulkan objects, and markers/labels in command buffers.  
See the [debug utils sample](samples/extensions/debug_utils/debug_utils_tutorial.md) for more information.

**Default:** `ON`

### VKB_WARNINGS_AS_ERRORS

Treat all warnings as errors

**Default:** `ON`

## Quality Assurance

We use a small set of tools to provide a level of quality to the project. These tools are part of our CI/CD process. If your local environment does not have the same versions of the tools we use in the CI you may see some errors or warnings pop-up when pushing.

For up-to date version information please see the repositories for the individual tools

- Doxygen [Doxygen Repository](https://github.com/KhronosGroupActions/doxygen)
- Clang Format / Clang Tidy [Clang Tools Repository](https://github.com/KhronosGroupActions/clang-tools)
- Snake Case Check [Snake Case Check Repository](https://github.com/KhronosGroupActions/snake-case-check)
- Android NDK [Android NDK Repository](https://github.com/KhronosGroupActions/android-ndk-build)

## 3D models

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

## Performance data

In order for performance data to be displayed, profiling needs to be enabled on the device. Some devices may disable it by default.

Profiling can be enabled via adb:

```
adb shell setprop security.perf_harden 0
```

> Performance data is captured using HWCPipe.
> For details on this project and how to integrate it in your pipeline,
> visit: https://github.com/ARM-software/HWCPipe

## Windows

### Dependencies

- CMake v3.12+
- Python 3
- Visual Studio 2017 or above
- [CMake Options](#cmake-options)
- [3D models](#3d-models)

### Clang Format and Visual Studio

Visual Studio comes with `clang-format-6` which is incompatible with some of the styles we use in our `.clang-format` file. It is recommended to point to a `clang-format-9.exe` binary within the in-built clang formatter, or disable it and use a third party extension that is more up to date.

Go to the [LLVM downloads page](http://releases.llvm.org/download.html) to get clang.

### Build with CMake

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

(New in CMake v3.14. Visual Studio 2019 must be installed)
```
 cmake -G "Visual Studio 16 2019" -A x64 -S . -Bbuild/windows
```

(New in CMake v3.21. Visual Studio 2022 must be installed)
```
 cmake -G "Visual Studio 17 2022" -A x64 -S . -Bbuild/windows
```

`Step 2.` Build the Visual Studio project

```
cmake --build build/windows --config Release --target vulkan_samples
```

`Step 3.` Run the **Vulkan Samples** application

```
build\windows\app\bin\Release\AMD64\vulkan_samples.exe
```

## Linux

### Dependencies

- CMake v3.12+
- C++14 Compiler
- [CMake Options](#cmake-options)
- [3D models](#3d-models)

```
sudo apt-get install cmake g++ xorg-dev libglu1-mesa-dev
```

### Build with CMake

`Step 1.` The following command will generate the project

```
cmake -G "Unix Makefiles" -H. -Bbuild/linux -DCMAKE_BUILD_TYPE=Release
```

`Step 2.` Build the project

```
cmake --build build/linux --config Release --target vulkan_samples -j$(nproc)
```

`Step 3.` Run the **Vulkan Samples** application to display the help message

```
./build/linux/app/bin/Release/x86_64/vulkan_samples --help
```

## macOS

### Dependencies

- CMake v3.12+ (Apple Silicon requires at least 3.19.2)
- XCode v12 for Apple Silicon
- Command Line Tools (CLT) for Xcode `xcode-select --install`
- [Vulkan SDK](https://vulkan.lunarg.com/doc/sdk/latest/mac/getting_started.html) `./install_vulkan.py`
- [CMake Options](#cmake-options)
- [3D models](#3d-models)

### Build with CMake

`Step 1.` The following command will generate the project

```
cmake -H. -Bbuild/mac -DCMAKE_BUILD_TYPE=Release
```

`Step 2.` Build the project

```
cmake --build build/mac --config Release --target vulkan_samples -j4
```

`Step 3.` Run the **Vulkan Samples** application to display the help message

```
./build/mac/app/bin/Release/x86_64/vulkan_samples --help
```


## Android

### Dependencies

For all dependencies set the following environment variables:

- JDK 8+ `JAVA_HOME=<SYSTEM_DIR>/java`
- Android SDK `ANDROID_HOME=<WORK_DIR>/android-sdk`
- CMake v3.16+
- Android NDK r23+ `ANDROID_NDK_HOME=<WORK_DIR>/android-ndk`
- [CMake Options](#cmake-options)
- [3D models](#3d-models)
- [Performance data](#performance-data)

> We use this environment in the CI [Android NDK Repository](https://github.com/KhronosGroupActions/android-ndk-build)

It is highly recommended to install [Android Studio](https://d.android.com/studio) to build, run and trace the sample project.
Android Studio uses the following plugins/tools to build samples:

- Android Gradle Plugin
- CMake Plugin, which installs and uses Ninja 
- NDK

Their versions are configured in the [build.gradle.in](https://github.com/KhronosGroup/Vulkan-Samples/blob/master/bldsys/cmake/template/gradle/build.gradle.in) and [app.build.gradle.in files](https://github.com/KhronosGroup/Vulkan-Samples/blob/master/bldsys/cmake/template/gradle/app.build.gradle.in); when updating these versions, refer to [the official documentation for the recommended combinations](https://developer.android.com/studio/projects/install-ndk#default-ndk-per-agp).


### Build with Gradle

#### Generate the gradle project 

To generate the gradle project, run the following command:

```
./scripts/generate.py android
```

A new folder will be created in the root directory at `build\android_gradle`

#### Install dependencies

[Android Gradle Plugin](https://d.android.com/reference/tools/gradle-api) (used by Android Studio) may not auto install dependencies. You will need to install them if they have not been installed:

- Find the configured versions in `build/android_gradle/app/build.gradle`, or its template file [`bldsys/camke/template/gradle/app.build.gradle.in`](https://github.com/KhronosGroup/Vulkan-Samples/blob/master/bldsys/cmake/template/gradle/app.build.gradle.in)
- [Install them with Android Studio](https://d.android.com/studio/projects/install-ndk) or [sdkmanager command line tool](https://d.android.com/studio/projects/configure-agp-ndk?language=agp4-1#command-line). For example, to install AGP port CMake 3.22.1 and NDK version 25.1.8937393 on Linux, do the following:
```
   yes | ${your-sdk}/cmdline-tools/latest/bin/sdkmanager --licenses
   ${your-sdk}/cmdline-tools/latest/bin/sdkmanager --install "ndk;25.1.8937393" --channel=3
   ${your-sdk}/cmdline-tools/latest/bin/sdkmanager --install "cmake;3.22.1" --channel=3
```

#### Build the project


```
cd build/android_gradle
```

> Prefer a release build for better performance unless you want to actively debug the application.

For a release build:

```
gradle assembleRelease
``` 

For a debug build:

```
gradle assembleDebug
``` 

#### Install the apk on the device

You can now install the apk on a connected device using the Android Debug Bridge:

For a release build:

```
adb install app/build/outputs/apk/release/vulkan_samples-release.apk
```
For a debug build:

```
adb install app/build/outputs/apk/debug/vulkan_samples-debug.apk
```


### Build with Android Studio

With [Android Studio](https://d.android.com/studio) you can open the `build/android_gradle/build.gradle` project, compile and run the project from here. The lastest Android Studio release is recommended.

If you have agreed with the licenses previously on your development system, Android Studio will automatically install, at the start up time, CMake and NDK with the version configured in your `build/android-gradle/build.gradle`. Otherwise (or if the installation failed), you need to install the required CMake and NDK manually, refer to [the official instructions](https://d.android.com/studio/projects/install-ndk) for the detailed steps. The default installed locations are:

- $SDK-ROOT-DIR/ndk/$ndkVersion for NDK.
- $SDK-ROOT-DIR/cmake/$cmake-version for CMake.

Android Studio will use the above default locations without any environment variable requirement; if you want to use the same NDK and CMake versions for other purpose, you can simply configure your environment variables to these locations. If you do set up the NDK and CMake environment variables, Android Studio will use them instead of the default locations.

