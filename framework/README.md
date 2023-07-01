<!--
- Copyright (c) 2023, Sascha Willems
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

# Vulkan Samples Framework <!-- omit in toc -->

This folder contains the base framework used by the samples. It offers sample base classes, encapsulates common functionality for e.g. loading assets (images, models, shaders), wraps common Vulkan objects and implements frequently used concepts like a caches and a scene graph. The framework also implements platform support for Windows, Linux, MacOS and Android.

It can also be used as a guideline for writing advanced Vulkan applications.

Before trying to implement common functions, consider checking if the framework doesn't already provides what you are looking for.

## Sample base classes

The framework provides two different sample base classes. When [creating new samples](../scripts/README.md), you can choose between one of them:

### High level base sample class

This base class abstracts away most of the Vulkan API and as such makes heavy use of the Vulkan object wrapper classes of the framework. Writing samples with the base class is less verbose.

See [vulkan_sample.h](./vulkan_sample.h) and [vulkan_sample.cpp](./vulkan_sample.cpp).

### API sample base class

This base class uses less abstraction, letting you work more explicitly with the api. 

See [api_vulkan_sample.h](./api_vulkan_sample.h) and [vulkan_sample.cpp](./api_vulkan_sample.cpp).

### Support for Vulkan-Hpp

While the framework itself primarily uses the C-Interface for Vulkan, both the high level and the API sample base class also come with [Vulkan-Hpp](https://github.com/KhronosGroup/Vulkan-Hpp) variants, letting you write samples using the C++ Vulkan language bindings instead.

See [hpp_vulkan_sample.h](./hpp_vulkan_sample.h) / [hpp_vulkan_sample.cpp](./hpp_vulkan_sample.cpp) and [hpp_api_vulkan_sample.h](./hpp_api_vulkan_sample.h) / [hpp_vulkan_sample.cpp](./hpp_api_vulkan_sample.cpp).

## Supported asset types

The base framework implements loaders for the following asset types:

- [glTF](https://www.khronos.org/gltf/) for 3D scenes
- [KTX](https://www.khronos.org/ktx/) for loading texture images
- GLSL for shaders