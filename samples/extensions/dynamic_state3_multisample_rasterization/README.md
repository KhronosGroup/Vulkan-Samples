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

# Overview

This sample demonstrates one of the functionalities of VK_EXT_extended_dynamic_state3 related to rasterization samples.
The extension can be used to dynamically change sampling without any need to restart the application.
image:image/image.png[]

# Enabling the extension

To be able to use this extension in Vulkan API:
1) At least Vulkan API version 1.1 must be supported.
2) Set of `VkPhysicalDeviceExtendedDynamicStateFeaturesEXT`, `VkPhysicalDeviceExtendedDynamicState2FeaturesEXT`, `VkPhysicalDeviceExtendedDynamicState3FeaturesEXT`
must be added to the feature chain of `VkPhysicalDeviceProperties2` and `VkPhysicalDeviceExtendedDynamicState3PropertiesEXT` must be added to `VkPhysicalDeviceProperties2`.

# Using the extension

To use the extension:
1) `VK_DYNAMIC_STATE_RASTERIZATION_SAMPLES_EXT` must be added to `VkPipelineDynamicStateCreateInfo`.
2) Method `void vkCmdSetRasterizationSamplesEXT(VkCommandBuffer commandBuffer, VkSampleCountFlagBits rasterizationSamples)` should be called between
`vkCmdBeginRenderPass` and `vkCmdEndRenderPass`.

# Resources

https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_extended_dynamic_state3.html
https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCmdSetRasterizationSamplesEXT.html
