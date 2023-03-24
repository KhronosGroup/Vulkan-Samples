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

# Descriptor buffers

## Overview

Binding and managing descriptors in Vulkan can become pretty complex, both for the application and the driver. With the [```VK_EXT_descriptor_buffer```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VK_ext_descriptor_buffer) extension, this interface is simplified and maps more directly to how hardware sees descriptors. It also simplifies the programming model, as you no longer have to create descriptor pool upfront.

This sample shows how to use that extension by rendering multiple objects with different uniform buffers and images using the new interface of creating and binding descriptors.

## Deprecated descriptor bindings

Creating and binding descriptors in Vulkan requires different steps and function calls.

After all, descriptors are just memory, and something like a `VkDescriptorPool` was an abstract concept that didn't actually map to hardware. On most implementations `vkCreateDescriptorPool` did nothing more than just a memory allocation. Same for `vkAllocateDescriptorSets`, which in the end is also just some sort of memory allocation, while `vkUpdateDescriptorSets` did some memory copies for the descriptors to that buffer.

With the streamlined descriptor setup from `VK_EXT_descriptor_buffer`, the api now maps more closely to this and removes the need for the following functions:

- vkCreateDescriptorPool
- vkAllocateDescriptorSets
- vkUpdateDescriptorSets
- vkCmdBindDescriptorSets

Other concepts of Vulkan's descriptor logic like descriptor set layouts and pipeline layouts are still used and not deprecated.


## The new way

The `VK_EXT_descriptor_buffer` replaces all of this with **resource descriptor buffer**. These store descriptors in a way that the GPU can directly read them from such a buffer. The application simply puts them into those buffers. That buffer is then bound at command buffer recording time similar to other buffer types.

To make the following code easier to understand, let's take a look at the interfaces of our shaders:

```glsl
// Vertex shader
layout (set = 0, binding = 0) uniform UBOScene {
	mat4 projection;
	mat4 view;
} uboCamera;

layout (set = 1, binding = 0) uniform UBOModel {
	mat4 local;
} uboModel;

// Fragment shader
layout (set = 2, binding = 0) uniform sampler2D samplerColorMap;
```

We use three descriptor sets, each with one binding.

### Creating the descriptor buffers

Descriptors are now stored in and accessed from memory, so instead of having to use the old approach of creating dedicated Vulkan objects, we create buffers that will store descriptors instead.

The extension introduces two different types of descriptors: Resource descriptors for buffers (uniform buffers, shader storage buffers) and sampler/combined image sampler descriptors. In this sample we'll be using both types, so we create two different buffers.

As is usual in Vulkan, implementations have different size and alignment requirements. So to calculate the actual buffer sizes required to store the descriptors, we need to get the sizes that match our descriptor set layouts first:

```cpp
std::array<VkDeviceSize, 2> descriptorLayoutSizes{};

// Buffers 
vkGetDescriptorSetLayoutSizeEXT(get_device().get_handle(), descriptor_set_layout_buffer, &descriptorLayoutSizes[0]);

// Combined image samplers
vkGetDescriptorSetLayoutSizeEXT(get_device().get_handle(), descriptor_set_layout_image, &descriptorLayoutSizes[1]);
```

Creating the resource descriptors for the uniform buffers using the `VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT` usage flag:

```cpp
resource_descriptor_buffer =
	std::make_unique<vkb::core::Buffer>(get_device(),
										(static_cast<uint32_t>(cubes.size()) + 1) * descriptorLayoutSizes[0],
										VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
										VMA_MEMORY_USAGE_CPU_TO_GPU);
```

Creating the combined image sampler descriptors by additionally adding the `VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT` usage flag:

```cpp
image_descriptor_buffer =
	std::make_unique<vkb::core::Buffer>(get_device(),
										static_cast<uint32_t>(cubes.size()) * descriptorLayoutSizes[1],
										VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
										VMA_MEMORY_USAGE_CPU_TO_GPU);
```

### Putting the descriptors into the buffers

After creating the appropriate buffers we now put the actual descriptors into those buffers, making them accessible to the GPU. This is done with the `vkGetDescriptorEXT` function.

The sample uses one global uniform buffer that stores the scene matrices, one uniform buffer per object displayed and one combined image sampler per object.

As usual we need to adhere to the size and alignment requirements of the implementation defined in `VkPhysicalDeviceDescriptorBufferPropertiesEXT`. At the start of the example we fetch that information into `descriptor_buffer_properties` so we can access it in the following code:

```cpp
VkPhysicalDeviceProperties2KHR device_properties{};
descriptor_buffer_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT;
device_properties.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
device_properties.pNext            = &descriptor_buffer_properties;
vkGetPhysicalDeviceProperties2KHR(get_device().get_gpu().get_handle(), &device_properties);
```

For resource descriptor buffers, we can simply put buffer device addresses into it. No need for descriptors, as the GPU only needs to know the address of the buffers to access them:

```cpp
buf_ptr = (char *) resource_descriptor_buffer->get_data();

// Global matrices uniform buffer
VkDescriptorAddressInfoEXT addr_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT};
addr_info.address                    = uniform_buffers.scene->get_device_address();
addr_info.range                      = uniform_buffers.scene->get_size();
addr_info.format                     = VK_FORMAT_UNDEFINED;

VkDescriptorGetInfoEXT buffer_descriptor_info{VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT};
buffer_descriptor_info.type                = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
buffer_descriptor_info.data.pUniformBuffer = &addr_info;
vkGetDescriptorEXT(get_device().get_handle(), &buffer_descriptor_info, descriptor_buffer_properties.uniformBufferDescriptorSize, buf_ptr);

// Per-cube uniform buffers
buf_ptr += alignment;
for (size_t i = 0; i < cubes.size(); i++)
{
	VkDescriptorAddressInfoEXT cube_addr_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT};
	cube_addr_info.address                    = cubes[i].uniform_buffer->get_device_address();
	cube_addr_info.range                      = cubes[i].uniform_buffer->get_size();
	cube_addr_info.format                     = VK_FORMAT_UNDEFINED;

	buffer_descriptor_info.data.pUniformBuffer = &cube_addr_info;
	vkGetDescriptorEXT(get_device().get_handle(), &buffer_descriptor_info, descriptor_buffer_properties.uniformBufferDescriptorSize, buf_ptr);
	buf_ptr += alignment;
}
```

For combined image samplers (or samplers alone) we can't use buffer device addresses as the implementation needs more information, so we have to put actual descriptors into the buffer instead:

```cpp
const VkDeviceSize alignment = descriptor_buffer_properties.descriptorBufferOffsetAlignment;

// For combined images we need to put descriptors into the descriptor buffers
char *buf_ptr = (char *) image_descriptor_buffer->get_data();
for (size_t i = 0; i < cubes.size(); i++)
{
	VkDescriptorImageInfo image_descriptor = create_descriptor(cubes[i].texture);

	VkDescriptorGetInfoEXT image_descriptor_info{VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT};
	image_descriptor_info.type                       = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	image_descriptor_info.data.pCombinedImageSampler = &image_descriptor;
	vkGetDescriptorEXT(get_device().get_handle(), &image_descriptor_info, descriptor_buffer_properties.combinedImageSamplerDescriptorSize, buf_ptr + i * alignment);
}
```

### Binding the buffers

As noted earlier, we no longer bind descriptor sets using `vkCmdBindDescriptorSets` but instead use `vkCmdBindDescriptorBuffersEXT` to bind the (resource) descriptor buffers and then use `vkCmdSetDescriptorBufferOffsetsEXT` to index into that buffer for the next draw:

```cpp
// Descriptor buffer bindings
// Binding 0 = uniform buffer
VkDescriptorBufferBindingInfoEXT descriptor_buffer_binding_info[2]{};
descriptor_buffer_binding_info[0].sType   = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
descriptor_buffer_binding_info[0].address = resource_descriptor_buffer->get_device_address();
descriptor_buffer_binding_info[0].usage   = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;
// Binding 1 = Image
descriptor_buffer_binding_info[1].sType   = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
descriptor_buffer_binding_info[1].address = image_descriptor_buffer->get_device_address();
descriptor_buffer_binding_info[1].usage   = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;
vkCmdBindDescriptorBuffersEXT(draw_cmd_buffers[i], 2, descriptor_buffer_binding_info);

uint32_t     buffer_index_ubo = 0;
VkDeviceSize alignment        = descriptor_buffer_properties.descriptorBufferOffsetAlignment;
VkDeviceSize buffer_offset    = 0;

// Global Matrices (set 0)
vkCmdSetDescriptorBufferOffsetsEXT(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &buffer_index_ubo, &buffer_offset);

// Set and offset into descriptor for each model
for (size_t j = 0; j < cubes.size(); j++)
{
    // Uniform buffer (set 1)
    // Model ubos start at offset * 1 (slot 0 is global matrices)
    buffer_offset = (j + 1) * alignment;
    vkCmdSetDescriptorBufferOffsetsEXT(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 1, 1, &buffer_index_ubo, &buffer_offset);
    // Image (set 2)
    uint32_t buffer_index_image = 1;
    buffer_offset               = j * alignment;
    vkCmdSetDescriptorBufferOffsetsEXT(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 2, 1, &buffer_index_image, &buffer_offset);
    draw_model(models.cube, draw_cmd_buffers[i]);
}
```

In detail and in reference to our shader interface:

Earlier on, we did put the device address for the global matrices uniform buffer at the beginning to the resource descriptor buffer. So we set it to point at `buffer_offset = 0` for set 0:

```cpp
vkCmdSetDescriptorBufferOffsetsEXT(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &buffer_index_ubo, &buffer_offset);
```

We then loop through all cubes displayed in the example and let the descriptor buffer point at the next device address using the alignment of the implementation for set 1:

```cpp
vkCmdSetDescriptorBufferOffsetsEXT(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 1, 1, &buffer_index_ubo, &buffer_offset);
```

With an alignment of 16 (see `VkPhysicalDeviceDescriptorBufferPropertiesEXT`) the device address for the uniform buffer for the first cube would start at byte 16 in the resource descriptor buffer, the device address for the second cube's uniform buffer would start at byte 32. 

The descriptor buffer containing the descriptors for our combined image samples is bound to set 2:

```cpp
vkCmdSetDescriptorBufferOffsetsEXT(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 2, 1, &buffer_index_image, &buffer_offset);
```

## What about the shaders?

With descriptor set and pipeline layouts, Vulkan decouples the shader interfaces from the application. And since we don't change these but only the way how we provide descriptors to the GPU, **no changes to the shaders are required**.