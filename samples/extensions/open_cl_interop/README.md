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

# Cross vendor OpenCL interoperability 

## Overview

Even though compute support in Vulkan is mandatory, there are still use-cases where the broader range of OpenCL's compute features may be required, e.g. for complex scientific computations or for re-using existing OpenCL kernels. For that both apis offer a set of vendor independent extensions that allow zero-copy sharing of objects known to both apis. Zero-copy means that both apis can access these objects without the need to duplicate and copy them between the apis. This allows for an efficient sharing of these objects between Vulkan and OpenCL.

This sample demonstrates that zero-copy sharing with an image that's updated using an OpenCL compute kernel and displayed as a texture on a quad inside Vulkan. To sync between the two apis the sample also makes use of shared semaphores.

To fully understand how this sample works it's advised to have experience with both Vulkan and OpenCL, esp. as both apis greatly differ in how things are set up.

## Required extensions

Both Vulkan and OpenCL offer extensions for so called external objects. An external object is something that can be referenced in multiple apis. In this sample we share images and semaphores, so we need to enable related extensions on both apis.

For **sharing the memory** backing up the image, in **Vulkan** we need to enable [```VK_KHR_external_memory_capabilities```](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_external_memory_capabilities.html) at instance level and [```VK_KHR_external_memory```](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_external_memory.html) at device level. We also need to enable specific extensions based on the platform we're running on. For Windows that's [```VK_KHR_external_memory_win32```](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_external_memory_win32.html) and for all Unix based platforms we need to enable [```VK_KHR_external_memory_fd```](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_external_memory_fd.html). The OpenCl equivalents to these extensions are [```cl_khr_external_memory```](https://registry.khronos.org/OpenCL/specs/3.0-unified/html/OpenCL_Ext.html#cl_khr_external_memory), ```cl_khr_external_memory_win32``` (Windows) and ```cl_khr_external_memory_opaque_fd``` (Unix based platforms).

For **sharing the semaphores** used to sync image access between the apis, in **Vulkan** we need enable [```VK_KHR_external_semaphore_capabilities```](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_external_semaphore_capabilities.html) at the instance level and [```VK_KHR_external_semaphore```](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_external_semaphore.html) at the device level. The platform specific extension to enable are [```VK_KHR_external_semaphore_win32```](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_external_semaphore_win32.html) for Windows and [```VK_KHR_external_semaphore_fd```](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_external_semaphore_fd.html) for Unix based platforms. The **OpenCL equivalents** to these are [```cl_khr_external_semaphore```](https://registry.khronos.org/OpenCL/specs/3.0-unified/html/OpenCL_Ext.html#cl_khr_external_semaphore), ```cl_khr_external_semaphore_win32``` (Windows) and ```cl_khr_external_semaphore_opaque_fd``` (Unix based platforms).

## A note on Windows security

On Windows we need to ensure read and write access to the shared memory for external handles (see [spec](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkExportMemoryWin32HandleInfoKHR.html#_description)). This requires setting up security attributes using the Windows API. To simplify this, the sample implements that in the ```WinSecurityAttributes``` class. This is then used in all places where we share memory on Windows.

## Creating and sharing the image

@todo: Describe get_vulkan_memory_handle

The sample will update the contents of an image with OpenCL and displays that on a quad with Vulkan. So we first need to setup that image (and it's memory) in Vulkan just as any other image with the appropriate usage flags:

```cpp
VkImageCreateInfo image_create_info = vkb::initializers::image_create_info();
image_create_info.imageType         = VK_IMAGE_TYPE_2D;
image_create_info.format            = VK_FORMAT_R8G8B8A8_UNORM;
image_create_info.mipLevels         = 1;
image_create_info.arrayLayers       = 1;
image_create_info.samples           = VK_SAMPLE_COUNT_1_BIT;
image_create_info.tiling            = VK_IMAGE_TILING_OPTIMAL;
image_create_info.extent            = {shared_image.width, shared_image.height, shared_image.depth};
image_create_info.usage             = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
```

And mark it as external using ```VkExternalMemoryImageCreateInfo``` in the ```pNext``` chain of the image create info structure , so other apis (in our case OpenCL) will be able to access it:

```cpp
VkExternalMemoryImageCreateInfo external_memory_image_info{};
external_memory_image_info.sType       = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
external_memory_image_info.handleTypes = external_handle_type;

image_create_info.pNext = &external_memory_image_info;
VK_CHECK(vkCreateImage(get_device().get_handle(), &image_create_info, nullptr, &shared_image.image));
```

Just like the required extensions, the ```handleTypes``` are also platform specific. We need to use ```VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR``` for Windows and ```VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR``` for Unix based platforms (which also includes Android).

We need to do the same with the memory backing up our image, as we also allocate it in the Vulkan part of our sample. We chain a ```VkExportMemoryAllocateInfoKHR``` structure into the memory allocation:

```cpp
VkExportMemoryAllocateInfoKHR export_memory_allocate_info{};
export_memory_allocate_info.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_KHR;
export_memory_allocate_info.handleTypes = external_handle_type;
#ifdef _WIN32
	export_memory_allocate_info.pNext = &export_memory_win32_handle_info;
#endif

VkMemoryAllocateInfo memory_allocate_info = vkb::initializers::memory_allocate_info();
memory_allocate_info.pNext                = &export_memory_allocate_info;
memory_allocate_info.allocationSize       = memory_requirements.size;
memory_allocate_info.memoryTypeIndex      = device->get_memory_type(memory_requirements.memoryTypeBits, 0);

VK_CHECK(vkAllocateMemory(device_handle, &memory_allocate_info, nullptr, &shared_image.memory));
VK_CHECK(vkBindImageMemory(device_handle, shared_image.image, shared_image.memory, 0));    
```

As noted earlier, on Windows we need to pass additional process security related information using the ```VkExportMemoryWin32HandleInfoKHR``` structure:

```cpp
#ifdef _WIN32
	WinSecurityAttributes            win_security_attributes;
	VkExportMemoryWin32HandleInfoKHR export_memory_win32_handle_info{};
	export_memory_win32_handle_info.sType       = VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_KHR;
	export_memory_win32_handle_info.pAttributes = &win_security_attributes;
	export_memory_win32_handle_info.dwAccess    = DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE;
	export_memory_allocate_info.pNext           = &export_memory_win32_handle_info;
#endif
```

Once we created the image along with it's memory in Vulkan, we **switch over to OpenCL** where we'll import the image. Note that the OpenCL api looks very different from Vulkan. OpenCL e.g. often uses zero terminated property lists instead of explicit structures.

First we import the image memory handle from Vulkan using ```get_vulkan_memory_handle``` and the platform specific handle type:

```cpp
	std::vector<cl_mem_properties> mem_properties;

#ifdef _WIN32
	HANDLE handle = get_vulkan_memory_handle(shared_image.memory);
	mem_properties.push_back((cl_mem_properties) CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_WIN32_KHR);
	mem_properties.push_back((cl_mem_properties) handle);
#else
	int fd = get_vulkan_memory_handle(shared_image.memory);
	mem_properties.push_back((cl_mem_properties) CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR);
	mem_properties.push_back((cl_mem_properties) fd);
#endif
	mem_properties.push_back((cl_mem_properties) CL_DEVICE_HANDLE_LIST_KHR);
	mem_properties.push_back((cl_mem_properties) opencl_objects.device_id);
	mem_properties.push_back((cl_mem_properties) CL_DEVICE_HANDLE_LIST_END_KHR);
	mem_properties.push_back(0);
```

And then create an OpenCL image using that handle:

```cpp
cl_image_format cl_img_fmt{};
cl_img_fmt.image_channel_order     = CL_RGBA;
cl_img_fmt.image_channel_data_type = CL_UNSIGNED_INT8;

cl_image_desc cl_img_desc{};
cl_img_desc.image_width       = shared_image.width;
cl_img_desc.image_height      = shared_image.height;
cl_img_desc.image_type        = CL_MEM_OBJECT_IMAGE2D;
cl_img_desc.image_slice_pitch = cl_img_desc.image_row_pitch * cl_img_desc.image_height;
cl_img_desc.num_mip_levels    = 1;
cl_img_desc.buffer            = nullptr;

int cl_result;
opencl_objects.image = clCreateImageWithProperties(opencl_objects.context,
                                                    mem_properties.data(),
                                                    CL_MEM_READ_WRITE,
                                                    &cl_img_fmt,
                                                    &cl_img_desc,
                                                    NULL,
                                                    &cl_result);
CL_CHECK(cl_result);
```

The interesting part here is:

```cpp
cl_img_desc.buffer            = nullptr;
```

This means that we don't allocate a buffer backing the image in OpenCL, but rather import it via the handle specified in the ```mem_properties``` property list. 

After the call to `clCreateImageWithProperties` we're ready to use the image in both apis.

## Creating and sharing semaphores

@todo: Describe get_vulkan_semaphore_handle

To sync work across Vulkan and OpenCL we'll be using semaphores. Once again we create these on the Vulkan side of our sample. Sharing them is very similar to sharing any other object like e.g. the image:

```cpp
VkExportSemaphoreCreateInfoKHR export_semaphore_create_info{};
export_semaphore_create_info.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO_KHR;

#ifdef _WIN32
WinSecurityAttributes               win_security_attributes;
VkExportSemaphoreWin32HandleInfoKHR export_semaphore_handle_info{};
export_semaphore_handle_info.sType       = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR;
export_semaphore_handle_info.pAttributes = &win_security_attributes;
export_semaphore_handle_info.dwAccess    = DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE;

export_semaphore_create_info.pNext       = &export_semaphore_handle_info;
export_semaphore_create_info.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
export_semaphore_create_info.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif

VkSemaphoreCreateInfo semaphore_create_info{};
semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
semaphore_create_info.pNext = &export_semaphore_create_info;

VK_CHECK(vkCreateSemaphore(device->get_handle(), &semaphore_create_info, nullptr, &cl_update_vk_semaphore));
VK_CHECK(vkCreateSemaphore(device->get_handle(), &semaphore_create_info, nullptr, &vk_update_cl_semaphore));
```

We once again select the handle type based on the platform we're compiling on and if it's a Windows system we set the required security access information before creating two semaphores with `vkCreateSemaphore`.

With the Vulkan part done, we again **switch over** to OpenCL, where we'll import the Vulkan semaphores:

```cpp
std::vector<cl_semaphore_properties_khr> semaphore_properties{
    (cl_semaphore_properties_khr) CL_SEMAPHORE_TYPE_KHR,
    (cl_semaphore_properties_khr) CL_SEMAPHORE_TYPE_BINARY_KHR,
    (cl_semaphore_properties_khr) CL_DEVICE_HANDLE_LIST_KHR,
    (cl_semaphore_properties_khr) opencl_objects.device_id,
    (cl_semaphore_properties_khr) CL_DEVICE_HANDLE_LIST_END_KHR,
};

// CL to VK semaphore

// We need to select the external handle type based on our target platform
#ifdef _WIN32
semaphore_properties.push_back((cl_semaphore_properties_khr) CL_SEMAPHORE_HANDLE_OPAQUE_WIN32_KHR);
HANDLE handle = get_vulkan_semaphore_handle(cl_update_vk_semaphore);
semaphore_properties.push_back((cl_semaphore_properties_khr) handle);
#else
semaphore_properties.push_back((cl_semaphore_properties_khr) CL_SEMAPHORE_HANDLE_OPAQUE_FD_KHR);
int fd = get_vulkan_semaphore_handle(cl_update_vk_semaphore);
semaphore_properties.push_back((cl_semaphore_properties_khr) fd);
#endif
semaphore_properties.push_back(0);

cl_int cl_result;

opencl_objects.cl_update_vk_semaphore = clCreateSemaphoreWithPropertiesKHR(opencl_objects.context, semaphore_properties.data(), &cl_result);
CL_CHECK(cl_result);

// Remove the last two entries so we can push the next handle and zero terminator to the properties list and re-use the other values
semaphore_properties.pop_back();
semaphore_properties.pop_back();

// VK to CL semaphore
// Code is the same, and not repeated here
...
```

## Sharing data between apis

@todo: Describe render(), esp. sync and acquire and release
