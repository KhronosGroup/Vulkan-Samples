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

# Enabling AFBC in your Vulkan Application

## Overview

AFBC (Arm Frame Buffer Compression) is a real-time lossless compression algorithm found in Arm Mali GPUs, designed to tackle the ever-growing demand for higher resolution graphics. This format is applied to the framebuffers that are to be written to the GPU. This technology can offer bandwidth reductions of [up to 50%](https://developer.arm.com/technologies/graphics-technologies/arm-frame-buffer-compression).

The sample is geared towards demonstrating the bandwidth that you can save by toggling AFBC on and off and displaying a real-time graph of the external bandwidth. In this case we will be focusing on the swapchain images.

The Vulkan API allows the developer a level of control around how the `VkSwapchainKHR` is created and formatted. It is here where we want to ensure that it is created and formatted in the right way so that the subsequent `VkImage`'s that we query from it have AFBC appropriately applied.

It is important to note that from a device perspective to have AFBC enabled on Vulkan, you will need at least driver version `r16p0` and a `Mali G-51` or higher. To find out your GPU and driver version, open the [debug window](../../../docs/misc.md#Debug-Window) or follow the steps in this [article](../../../docs/misc.md#Driver-Version).

> Tested on: Samsung Galaxy S10, Huawei P30

## Enabling AFBC

AFBC is functionally transparent to the application and will be automatically applied on a per `VkImage` basis (provided multiple checks pass on various properties of your device and your images).

The driver will check the applications state along with the `VkImage` properties to determine if it will enable AFBC or just continue without it.
This section will detail the requirements.

`VkImage` requirements:

* `VkSampleCountFlagBits` must be `VK_SAMPLE_COUNT_1_BIT`
* `VkImageType` must be `VK_IMAGE_TYPE_2D`
* `VkImageTiling` must be `VK_IMAGE_TILING_OPTIMAL`
* `VkFormat` [supported list](#format-support)

In addition to this, your `VkImage` needs to adhere to the following flags:

* `VkImageUsageFlags` must not contain:
  * `VK_IMAGE_USAGE_STORAGE_BIT`
  * `VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT`
  * (Only for some devices with driver version `r16p0`) `VK_IMAGE_USAGE_TRANSFER_DST_BIT`
* `VkImageCreateFlags` must not contain:
  * `VK_IMAGE_CREATE_ALIAS_BIT`
  * `VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT`

## The AFBC Sample

The sample presents the user with Sponza, with a graph displaying bandwidth at the top and a configuration window at the bottom. There is a back-and-forth oscillating camera to disable any GPU optimisations for frames that are identical.

The configuration itself is simple. There is one checkbox (labelled "Enable AFBC") that will reload the swapchain when its value is changed.

![Sponza AFBC off](images/afbc_disabled.jpg)

Here the sample is in its default state: AFBC off. At the top of the screen there is a graph displaying the external write bandwidth (measured from `L2_EXT_WRITE_BEATS`).

It is currently setting the `VK_IMAGE_USAGE_STORAGE_BIT` flag in the `VkImageUsageFlags` for the swapchain images, causing the driver to skip over applying AFBC to the swapchain images.

When we enable the check box, the sample will reload the swapchain with the right usage flags to have the driver enable AFBC.

![Sponza AFBC on](images/afbc_enabled.jpg)

Here is the same scene as before, except AFBC is now enabled. The `VK_IMAGE_USAGE_STORAGE_BIT` flag is not being set and the swapchain is being created properly.

The bandwidth has dropped from 788.0 MiB/s to 528.6 MiB/s, this is approximately a 33% reduction.

You can also confirm these numbers in Streamline. Here are some screenshots:

![Streamline AFBC Off](images/streamline_disabled.png)
![Streamline AFBC On](images/streamline_enabled.png)

## Format Support

GPUs from Mali-G77 onwards support formats up to and including 32 bits per pixel regardless of color channel arrangement or sRBG.

Previous generations that support AFBC only support a subset of formats:

|             Formats             |
| :-----------------------------: |
| VK_FORMAT_R4G4B4A4_UNORM_PACK16 |
| VK_FORMAT_B4G4R4A4_UNORM_PACK16 |
|  VK_FORMAT_R5G6B5_UNORM_PACK16  |
| VK_FORMAT_R5G5B5A1_UNORM_PACK16 |
| VK_FORMAT_B5G5R5A1_UNORM_PACK16 |
| VK_FORMAT_A1R5G5B5_UNORM_PACK16 |
|     VK_FORMAT_B8G8R8_UNORM      |
|    VK_FORMAT_B8G8R8A8_UNORM     |
|     VK_FORMAT_B8G8R8A8_SRGB     |
|    VK_FORMAT_A8B8G8R8_UNORM     |
|     VK_FORMAT_A8B8G8R8_SRGB     |
|     VK_FORMAT_A8R8G8B8_SRGB     |
|   VK_FORMAT_B10G10R10A2_UNORM   |
|    VK_FORMAT_R4G4B4A4_UNORM     |
|     VK_FORMAT_R5G6B5_UNORM      |
|    VK_FORMAT_R5G5B5A1_UNORM     |
|       VK_FORMAT_R8_UNORM        |
|      VK_FORMAT_R8G8_UNORM       |
|     VK_FORMAT_R8G8B8_UNORM      |
|    VK_FORMAT_R8G8B8A8_UNORM     |
|     VK_FORMAT_R8G8B8A8_SRGB     |
|    VK_FORMAT_A8R8G8B8_UNORM     |
|   VK_FORMAT_R10G10B10A2_UNORM   |
|   VK_FORMAT_D24_UNORM_S8_UINT   |
|       VK_FORMAT_D16_UNORM       |
|      VK_FORMAT_D32_SFLOAT       |

## Further Reading

* [Arm Frame Buffer Compression](https://www.arm.com/why-arm/technologies/graphics-technologies/arm-frame-buffer-compression) - developer.arm.com

## Best practice summary

**Do**

* Ensure that your swapchain is created correctly as per the requirements of AFBC.
* Ensure that all textures are compressed into ASTC format.
* Avoid changing your image configuration at run-time (using `vkCmdCopyImage` with an invalid AFBC requirement) as it will trigger a decompression.
* Make sure you are resolving your images using `pResolveAttachments` when it comes to multisampling. Any `VkImage` with `SAMPLE_COUNT > 1` will not have AFBC applied to it.

**Don't**

* Use image usage flags, such as `VK_IMAGE_USAGE_STORAGE_BIT`, unless you really need it (i.e. for compute on a specific image).

**Impact**

* Having an incorrect configuration of your images will cause all your surface `VkImage`'s to be uncompressed, losing out on considerable system wide bandwidth reductions.

**Debugging**

* To test if AFBC is enabled or disabled, you can use a profiler such as Streamline and record the bandwidth values when AFBC is enabled or when AFBC is disabled.
