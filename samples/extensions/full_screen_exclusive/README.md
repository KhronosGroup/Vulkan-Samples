<!--
- Copyright (c) 2022, Holochip Corporation
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

# Full Screen Exclusive
## Overview
This code sample demonstrates how to incorporate Vulkan full screen exclusive extension.
## Full screen exclusive extension
In order to enable usage of full screen exclusive extension features, instance extensions and device extensions were introduced in initiation functions of the FullScreenExclusive class. Where, inside the function scope of init_instance():
```cpp
active_instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
active_instance_extensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
active_instance_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
```
If application is running on a Windows platform, it can be detected per usage of:
```defined(VK_USE_PLATFORM_WIN32_KHR)```
Meantime, due to the introduction of a series of full screen exclusive related functions, a Boolean is introduced as well, where:
```isWin32 = true;```
This Boolean variable is set true per detection of application running on a Windows platform. In addition, device extensions are added in addition to the required extensions inside the init_device() function, where:
```cpp
if (isWin32)
{   
    active_device_extensions.push_back(VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME);
}
```
## Swapchain
Full Screen Exclusive related features will be added in the swapchain creation. Where, a new variable called ```surface_full_screen_exclusive_info_EXT``` is created locally within  the function scope. It is in format of ```VkSurfaceFullScreenExclusiveInfoEXT```, and its sType is initialized as ```VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT```.
Based on the full screen exclusive mode selection, following Enums can be chosen for the ```surface_full_screen_exclusive_info_EXT.fullScreenExclusive```. Where, a) ```VK_FULL_SCREEN_EXCLUSIVE_DEFAULT_EXT```, b) ```VK_FULL_SCREEN_EXCLUSIVE_DESALLOWED_EXT```, c) ```VK_FULL_SCREEN_EXCLUSIVE_ALLOWED_EXT```, d) ```VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT```.
While initializing the swapchain chain creation information in format of ```VkSwapchainCreateInforKHR```, reference of ```surface_full_screen_exclusive_info_EXT``` must be passed to the pNext of the swapchain creation information. Where, in case of application running on a Window platform is detected, the following codes are introduced: 
```cpp
VkSwapchainCreateInfoKHR info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};        // initialize the swapchain create info without adding pNext info
// if Windows platform application:
if (isWin32)
{
   VkSurfaceFullScreenExclusiveInfoEXT surface_full_screen_exclusive_info_EXT{VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT};
   // this is to execute the full screen related variables based on its chosen status
   switch (full_screen_status)
   {
      case 0:        // default
         surface_full_screen_exclusive_info_EXT.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_DEFAULT_EXT;
         break;
      case 1:        // disallowed
         surface_full_screen_exclusive_info_EXT.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_DISALLOWED_EXT;
         break;
      case 2:        // allowed
         surface_full_screen_exclusive_info_EXT.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_ALLOWED_EXT;
         break;
      case 3:        // full screen exclusive combo with acquire and release
         surface_full_screen_exclusive_info_EXT.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT;
         break;
      default:        // this case won't usually happen but just to set everything to be default just in case
         surface_full_screen_exclusive_info_EXT.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_DEFAULT_EXT;
         break;
   }
   info.pNext = &surface_full_screen_exclusive_info_EXT;        // syncing the full screen exclusive info.
}
```
Notice that, the ```sType``` of a ```VkSwapchainCreateInfoKHR``` variable is initialized to be ```VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR```. Rest of the swapchain create information are configured normally.
Furthermore, a ```recreate()``` function is introduced per change of full screen exclusive mode. Which it pauses the renderer, teardown the previous frame buffer, and reinitiate a new swapchain and a new frame buffer.  
```cpp
void FullScreenExclusive::recreate()
{
   // Check if there IS a device, if not don't do anything
   if (context.device != VK_NULL_HANDLE)
   {
      vkDeviceWaitIdle(context.device);        // pause the renderer
      teardown_frame_buffers(context);         // basically destroy everything swapchain related
      init_swapchain(context);                 // recreate swapchain
      init_frame_buffers(context);             // sync the recreated swapchain to the frame buffer
      if (isFullScreenExclusive)
      {
         VK_CHECK(vkAcquireFullScreenExclusiveModeEXT(context.device, context.swapchain));
      }
   }
}
```
## Input events
The input event function is overridden in order to help selected the desired full screen exclusive mode. Where, display modes can be selected using keyboard input from F1 to F4, representing 1) default, 2) disallowed, 3) allowed, and 4) application controlled. Which is reflected in the following code: 
```cpp
switch (key_button.get_code())
{
   case vkb::KeyCode::F1:        // Default
      if (full_screen_status != 0)
      {
         full_screen_status    = 0;
         isRecreate            = true;
         isFullScreenExclusive = false;
      }
      break;
   case vkb::KeyCode::F2:        // Disallowed
      if (full_screen_status != 1)
      {
         full_screen_status    = 1;
         isRecreate            = true;
         isFullScreenExclusive = false;
      }
      break;
   case vkb::KeyCode::F3:        // Allowed
      if (full_screen_status != 2)
      {
         full_screen_status    = 2;
         isRecreate            = true;
         isFullScreenExclusive = false;
      }
      break;
      // App decided full screen exclusive
   case vkb::KeyCode::F4:
      if (full_screen_status != 3)
      {
         full_screen_status    = 3;
         isRecreate            = true;
         isFullScreenExclusive = true;
      }
      break;
   default:
      break;
}
```
Meantime, per proper input events, swapchain recreation will be triggered, where: 
```cpp
if (isRecreate)
{
   FullScreenExclusive::recreate();
}
```