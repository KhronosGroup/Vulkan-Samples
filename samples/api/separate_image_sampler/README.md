<!--
- Copyright (c) 2021, Sascha Willems
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

# Separating samplers and images

This tutorial, along with the accompanying example code, shows how to separate samplers and images in a Vulkan application. Opposite to combined image and samplers, this allows the application to freely mix an arbitrary set of samplers and images in the shader.

In the sample code, a single image and multiple samplers with different options will be created. The sampler to be used for sampling the image can then be selected at runtime. As image and sampler objects are separated, this only requires selecting a different descriptor at runtime.

## In the application

From the application's point of view, images and samplers are always created separately. Access to the image is done via the image's `VkImageView`. Samplers are created using a `VkSampler` object, specifying how an image will be sampled.

The difference between separating and combining them starts at the descriptor level, which defines how the shader accesses the samplers and images.

A separate setup uses a descriptor of type `VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE` for the sampled image, and a `VK_DESCRIPTOR_TYPE_SAMPLER` for the sampler, separating the image and sampler object:

```cpp
// Image info only references the image
VkDescriptorImageInfo image_info{};
image_info.imageView   = texture.image->get_vk_image_view().get_handle();
image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

// Sampled image descriptor
VkWriteDescriptorSet image_write_descriptor_set{};
image_write_descriptor_set.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
image_write_descriptor_set.dstSet          = base_descriptor_set;
image_write_descriptor_set.dstBinding      = 1;
image_write_descriptor_set.descriptorCount = 1;
image_write_descriptor_set.descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
image_write_descriptor_set.pImageInfo      = &image_info;

// One set for the sampled image
std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
	...
	// Binding 1 : Fragment shader sampled image
	image_write_descriptor_set};
vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);
```

For this sample, we then create two samplers with different filtering options:

```cpp
// Sets for each of the sampler
descriptor_set_alloc_info.pSetLayouts = &sampler_descriptor_set_layout;
for (size_t i = 0; i < sampler_descriptor_sets.size(); i++)
{
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &descriptor_set_alloc_info, &sampler_descriptor_sets[i]));

	// Descriptor info only references the sampler
	VkDescriptorImageInfo sampler_info{};
	sampler_info.sampler = samplers[i];

	VkWriteDescriptorSet sampler_write_descriptor_set{};
	sampler_write_descriptor_set.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	sampler_write_descriptor_set.dstSet          = sampler_descriptor_sets[i];
	sampler_write_descriptor_set.dstBinding      = 0;
	sampler_write_descriptor_set.descriptorCount = 1;
	sampler_write_descriptor_set.descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER;
	sampler_write_descriptor_set.pImageInfo      = &sampler_info;

	vkUpdateDescriptorSets(get_device().get_handle(), 1, &sampler_write_descriptor_set, 0, nullptr);
}
```

At draw-time, the descriptor containing the sampled image is bound to set 0 and the descriptor for the currently selected sampler is bound to set 1:

```cpp
// Base descriptor with the image to be sampled in set 0
vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &base_descriptor_set, 0, nullptr);
// Descriptor for the selected sampler in set 1
vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 1, 1, &sampler_descriptor_sets[selected_sampler], 0, nullptr);
...
vkCmdDrawIndexed(draw_cmd_buffers[i], index_count, 1, 0, 0, 0);
``` 

## In the shader

With the above setup, the shader interface for the fragment shader also separates the sampler and image as two distinct uniforms:

```glsl
layout (set = 0, binding = 1) uniform texture2D _texture;
layout (set = 1, binding = 0) uniform sampler _sampler;
```

To sample from the image referenced by `_texture`, with the currently set sampler in '_sampler', we create a sampled image in the fragment shader at runtime using the `sampler2D` function.

```glsl
void main() 
{
    vec4 color = texture(sampler2D(_texture, _sampler), inUV);
}
```

## Comparison with combined image samplers

For reference, a combined image and sampler setup would differ for both the application and the shader. The app would use a single descriptor of type `VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER`, and set both image and sampler related values in the descriptor:

```cpp
// Descriptor info references image and sampler
VkDescriptorImageInfo image_info;
image_info.imageView   = texture.view;
image_info.sampler     = texture.sampler;
image_info.imageLayout = texture.image_layout;

VkWriteDescriptorSet image_write_descriptor_set{};
image_write_descriptor_set.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
image_write_descriptor_set.dstSet          = descriptor_set;
image_write_descriptor_set.dstBinding      = 0;
image_write_descriptor_set.descriptorCount = 1;
image_write_descriptor_set.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
image_write_descriptor_set.pImageInfo      = &image_info;        
```

The shader interface only uses one uniform for accessing the combined image and sampler and also doesn't construct a `sampler2D` at runtime:

```glsl
layout (binding = 1) uniform sampler2D _combined_image;

void main() 
{
    vec4 color = texture(_combined_image, inUV);
}
```

Compared to the separated setup, changing a sampler in this setup would either require creating multiple descriptors with each image/sampler combination or rebuilding the descriptor.