<!--
- Copyright (c) 2023, Mobica Limited
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

# Color write enable
## Overview
This sample demonstrates how to use the `VK_EXT_color_write_enable` extension. This extension allows to toggle the output color attachments using a pipeline dynamic state. It allows the program to prepare an additional framebuffer populated with the data from a defined color blend attachment which can be blended dynamically to the final scene.

The final results are comparable to those obtained with `vkCmdSetColorWriteMaskEXT`, but it does not require the GPU driver to support `VK_EXT_extended_dynamic_state3`.

## How to use in Vulkan
To use this feature, the device extension VK_EXT_COLOR_WRITE_ENABLE_EXTENSION_NAME has to be enabled.
Support of this feature can be queried by extending the struct VkPhysicalDeviceFeatures2 in the vkGetPhysicalDeviceFeatures2 call by a VkPhysicalDeviceColorWriteEnableFeaturesEXT struct.
`VkPipelineColorWriteCreateInfoEXT` contains an array of Boolean values that serve as toggles for the corresponding `VkPipelineColorBlendAttachmentState`. This array can be overwritten dynamically with the `vkCmdSetColorWriteEnableEXT` function. 

Two subpasses are performed in the sample. In the first subpass, three attachments are used. Each attachment has only one color component bit enabled - R, G and B. A triangle is drawn on each of them separately.
The second subpass combines three images created in the previous pass. Checkboxes are used to toggle the `vkCmdSetColorWriteEnableEXT` function disabling each attachment. As a result of its disabling, the value of a given channel is set as the value of that channel in the background color. Sliders are used to set the background color.

## The sample
The sample shows how to setup an application to work with this extension:
- How to enable the extension.
- How to set up multiple color attachments in the color blend state.
- How to set up the render subpass and framebuffers for multiple color attachments.
- How to write a fragment shader with multiple outputs.



## Documentation links
[https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_color_write_enable.html](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_color_write_enable.html)

[https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSubpassDescription.html](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSubpassDescription.html)

[https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkFramebufferCreateInfo.html](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkFramebufferCreateInfo.html)

## See also
[https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCmdSetColorWriteMaskEXT.html](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCmdSetColorWriteMaskEXT.html)

