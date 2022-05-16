<!--
- Copyright (c) 2022, Holochip
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

# Vulkan Portability Extension

## Overview

This tutorial, along with the accompanying example code, demonstrates the use of the [VK_KHR_portability_subset](https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/vkspec.html#VK_KHR_portability_subset) extension. 
When the VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR is set in the ```Instance``` class, Vulkan will consider 
devices that aren't fully conformant such as [MoltenVk](https://github.com/KhronosGroup/MoltenVK) to be identified 
as a conformant implementation.  When this happens, use the VkPhysicalDevicePortabilitySubsetPropertiesKHR extension 
with the [vkGetPhysicalDeviceFeatures2](https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/vkspec.html#vkGetPhysicalDeviceFeatures2)
as detailed below to get the list of supported/unsupported features.

This tutorial along with the accompanying code also demonstrates the use of the [VkPhysicalDevicePortabilitySubsetPropertiesKHR](https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/vkspec.html#VkPhysicalDevicePortabilitySubsetPropertiesKHR) which is currently a beta khronos extension.  This extension 
provides a list of supported and unsupported parts of Vulkan on a non-conformant Vulkan instance.  Build with 
VK_ENABLE_BETA_EXTENSIONS set to enable this.

## Setup

**Note**: Enabling the extension globally is done inside the framework, see the ```Instance``` class in [instance.
cpp](../../../framework/core/instance.cpp) for details.  To enable the extension for all samples, build with 
VKB_ENABLE_PORTABILITY defined.

Enabling the functionality for the portability subset is done by adding the extension to the list of extensions to 
enable at instance level. The device instance can also be used to generate the subset of portability enabled device 
items.
As with all extensions, this is optional, and you should check if the extension is present before enabling it.

```cpp
uint32_t instance_extension_count;
VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, nullptr));

std::vector<VkExtensionProperties> available_instance_extensions(instance_extension_count);
VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, available_instance_extensions.data()));

bool debug_utils = false;
for (auto &available_extension : available_instance_extensions)
{
    if (strcmp(available_extension.extensionName, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME) == 0)
    {
        debug_utils = true;
        extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
    }
}
```

### Asking the device for the list of supported features

NB: VkPhysicalDevicePortabilitySubsetFeaturesKHR is currently a beta extension and will only compile with the 
VK_ENABLE_BETA_EXTENSIONS definition set.
```cpp
VkPhysicalDevicePortabilitySubsetFeaturesKHR portability_features{};
portability_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PORTABILITY_SUBSET_FEATURES_KHR;
```

We then pass this to the ```pNext``` member of our Physical Device Features creation structure, then call the 
vkGetPhysicalDeviceFeatures2 function, the structure will populate and can be queried:

```cpp
VkPhysicalDeviceFeatures2 device_features{};
device_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
device_features.pNext = &portability_features;
vkGetPhysicalDeviceFeatures2(get_device().get_gpu().get_handle(), &device_features);
```
