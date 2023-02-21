<!--
- Copyright (c) 2023, Holochip Corporation
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

This code sample demonstrates how to incorporate the Vulkan extension ```VK_EXT_full_screen_exclusive ```. This
extension provides a solution for the full screen exclusion issue on Windows 10 platform, that window 10 cannot
correctly adapt the ture exclusive mode. Notice that, ```VK_EXT_full_screen_exclusive ``` is applicable on Windows 10
platform alone.

## Introduction

This sample provides a detailed procedure to activate full screen exclusive mode on Windows 10 applications. Users can
switch display mode from: 1) windowed, 2) borderless fullscreen, and 3) exclusive fullscreen using keyboard inputs.

## *Reminder

This is an optional section.

One must notice that, by configuring the ```swapchain create info```
using full screen exclusive extension **DOES NOT** automatically set the application window to full screen mode. The
following procedure shows how to activate full screen exclusive mode correctly:

1) recreate the ```swapchain``` using ```full screen exclusive``` related features.
2) recreate the ```frame buffers``` with the new ```swapchain```.
3) configure the application window to **fullscreen mode** by using windowing SDK (e.g., ```Windows```
   or ```GLFW``` commands, etc.)
4) execute the ```acquire full screen exclusive EXT``` call.

## Full screen exclusive extension

To enable ```VK_EXT_full_screen_exclusive``` extension, the following instance and device extensions must be added:

1. Instance extension:```VK_KHR_WIN32_SURFACE_EXTENSION_NAME```
2. Instance extension:```VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME```
3. Instance extension:```VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME```
4. Instance extension:```VK_KHR_SURFACE_EXTENSION_NAME```
5. Device extension: ```VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME```

Where, instance extensions are added in ```init_instance()```:

```cpp
active_instance_extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
active_instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
active_instance_extensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
```

Device extensions are added in ```perpare()``` using ```init_device()```, due to its built-in extension availability
check, where:

```cpp
init_device({VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME});
```

## Swapchain creation

When creating the ```swapchain```, ```surface_full_screen_exclusive_info_EXT```
and ```surface_full_screen_exclusive_Win32_info_EXT``` are defined inside the function scope, where:

```cpp
VkSurfaceFullScreenExclusiveInfoEXT      surface_full_screen_exclusive_info_EXT{};
VkSurfaceFullScreenExclusiveWin32InfoEXT surface_full_screen_exclusive_Win32_info_EXT{};
```

Where, both variables are initialized using```initialize_windows()```:

```cpp
void FullScreenExclusive::initialize_windows()
{
	// The following variable has to be attached to the pNext of surface_full_screen_exclusive_info_EXT:
	surface_full_screen_exclusive_Win32_info_EXT.sType    = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_WIN32_INFO_EXT;
	surface_full_screen_exclusive_Win32_info_EXT.pNext    = nullptr;
	surface_full_screen_exclusive_Win32_info_EXT.hmonitor = MonitorFromWindow(HWND_application_window, MONITOR_DEFAULTTONEAREST);

	surface_full_screen_exclusive_info_EXT.sType               = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT;
	surface_full_screen_exclusive_info_EXT.pNext               = &surface_full_screen_exclusive_Win32_info_EXT;
	surface_full_screen_exclusive_info_EXT.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_DEFAULT_EXT;        // Set the fullScreenExclusive stage to default when initializing

	// Windows placement should only be registered ONCE:
	GetWindowPlacement(HWND_application_window, &wpc);
}
```

The ```sType``` of ```surface_full_screen_exclusive_Win32_info_EXT```
is ```VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_WIN32_INFO_EXT```
and its ```pNext``` is connected to a ```nullptr```. One can get its ```hmonitor``` using the following Windows command:

```cpp
MonitorFromWindow(HWND_application_window, MONITOR_DEFAULTTONEAREST);
```

More details can be found in the link:
[MonitorFromWindow function (winuser.h)](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-monitorfromwindow)

```HWND_applicationWindow``` is a ```hwnd``` windows handle, and is declared as follow:

```cpp
HWND HWND_application_window{}; 
```

one can get it using the following Windows command:

```cpp
HWND_application_window = GetActiveWindow();
```

More details can be found in the link:
[GetActiveWindow  function (winuser.h)](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getactivewindow)

The ```sType``` of the ```surface_full_screen_exclusive_info_EXT``` is
```VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT```, and its ```pNext``` is connected to
```surface_full_screen_exclusive_Win32_info_EXT```. Its ```display mode``` must be selected from one of the following
enums:
from, Where:

1. ```VK_FULL_SCREEN_EXCLUSIVE_DEFAULT_EXT```
2. ```VK_FULL_SCREEN_EXCLUSIVE_DISALLOWED_EXT```
3. ```VK_FULL_SCREEN_EXCLUSIVE_ALLOWED_EXT```
4. ```VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT```

The swapchain chain creation info in declared locally, its ```sType``` is
```VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR```. and its ```pNext``` should be attached to
the ```surface_full_screen_exclusive_info_EXT```.

```cpp
VkSwapchainCreateInfoKHR info{};
info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
info.pNext = &surface_full_screen_exclusive_info_EXT;
```

## *Application window modes

This is an optional section.

In this sample, function ```update_application_window()``` is created to help switch the application from windowed and
fullscreen mode, where:

```cpp
void FullScreenExclusive::update_application_window()
{
	if (application_window_status == ApplicationWindowMode::Fullscreen && is_windowed)        // check if it is already in fullscreen, if is, then do nothing
	{
		is_windowed = false;

		if (HWND_style == 0)
			HWND_style = GetWindowLong(HWND_application_window, GWL_STYLE);
		if (HWND_extended_style == 0)
			HWND_extended_style = GetWindowLong(HWND_application_window, GWL_EXSTYLE);

		long HWND_newStyle          = HWND_style & (~WS_BORDER) & (~WS_DLGFRAME) & (~WS_THICKFRAME);
		long HWND_extended_newStyle = HWND_extended_style & (~WS_EX_WINDOWEDGE);

		SetWindowLong(HWND_application_window, GWL_STYLE, HWND_newStyle | static_cast<long>(WS_POPUP));
		SetWindowLong(HWND_application_window, GWL_EXSTYLE, HWND_extended_newStyle | WS_EX_TOPMOST);

		ShowWindow(HWND_application_window, SW_SHOWMAXIMIZED);
	}
	else if (application_window_status == ApplicationWindowMode::Windowed && !is_windowed)        // check if it is already "windowed", if is, then do nothing
	{
		is_windowed = true;

		SetWindowLong(HWND_application_window, GWL_STYLE, HWND_style);
		SetWindowLong(HWND_application_window, GWL_EXSTYLE, HWND_extended_style);
		ShowWindow(HWND_application_window, SW_SHOWNORMAL);
		SetWindowPlacement(HWND_application_window, &wpc);
	}
}
```

## Recreation

```recreate()``` will be called, when display mode changes. ```recreate()``` idles the ```logic device```, tears down
the current ```frame buffers```, then creates a new ```swapchain```
based on the selected display mode:

```cpp
void FullScreenExclusive::recreate()
{
	// Check if there IS a device, if not don't do anything
	if (context.device != VK_NULL_HANDLE)
	{
		// Step: 0) Idle the device, destroy/teardown the current swapchain and frame buffers.
		vkDeviceWaitIdle(context.device);        // pause the renderer
		teardown_framebuffers();                 // basically destroy everything swapchain related

		// Step: 1) recreate the swapchain with its properly selected FullscreenExclusive enum value
		init_swapchain();

		// Step: 2) recreate the frame buffers using the newly created swapchain
		init_framebuffers();

		// Step: 3-1) update the window mode, corresponding to the FullscreenExclusive enum value
		update_application_window();

		// Step: 3-2) remember: ALWAYS change the application window mode BEFORE acquire the full screen exclusive mode!
		if (is_full_screen_exclusive)
		{
			VkResult result = vkAcquireFullScreenExclusiveModeEXT(context.device, context.swapchain);
			LOGI("vkAcquireFullScreenExclusiveModeEXT: " + vkb::to_string(result))        // it would be necessary to learn the result on an unsuccessful attempt
		}
	}
}
```

## Input events

Display mode can be switched using keyboard input from F1 to F3, where:

1. F1: Windowed with ```VK_FULL_SCREEN_EXCLUSIVE_DISALLOWED_EXT``` and ```ApplicationWindowMode::Windowed```.
2. F2: Borderless Fullscreen with ```VK_FULL_SCREEN_EXCLUSIVE_ALLOWED_EXT```
   and ```ApplicationWindowMode::Fullscreen```.
3. F3: Exclusive Fullscreen with ```VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT```
   and ```ApplicationWindowMode::Fullscreen```.

```cpp
switch (key_button.get_code())
{
    case vkb::KeyCode::F1:        // FullscreenExclusiveEXT = Disallowed
        if (full_screen_status != SwapchainMode::Windowed)
        {
            full_screen_status                                         = SwapchainMode::Windowed;
            application_window_status                                  = ApplicationWindowMode::Windowed;
            surface_full_screen_exclusive_info_EXT.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_DISALLOWED_EXT;
            needs_recreate                                             = true;
            
            // If it is switched from the exclusive full screen mode, then release full screen first
            if (is_full_screen_exclusive)
            {
                VkResult result = vkReleaseFullScreenExclusiveModeEXT(context.device, context.swapchain);
                if (result != VK_SUCCESS)
                {
                    LOGI("vkReleaseFullScreenExclusiveModeEXT: " + vkb::to_string(result))
                }
            }
            
            is_full_screen_exclusive                                   = false;
            LOGI("Windowed Mode Detected!")
        }
        break;
    case vkb::KeyCode::F2:        // FullscreenExclusiveEXT = Allowed
        if (full_screen_status != SwapchainMode::BorderlessFullscreen)
        {
            full_screen_status                                         = SwapchainMode::BorderlessFullscreen;
            application_window_status                                  = ApplicationWindowMode::Fullscreen;
            surface_full_screen_exclusive_info_EXT.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_ALLOWED_EXT;
            needs_recreate                                             = true;

            // If it is switched from the exclusive full screen mode, then release full screen first
            if (is_full_screen_exclusive)
            {
                VkResult result = vkReleaseFullScreenExclusiveModeEXT(context.device, context.swapchain);
                if (result != VK_SUCCESS)
                {
                    LOGI("vkReleaseFullScreenExclusiveModeEXT: " + vkb::to_string(result))
                }
            }

            is_full_screen_exclusive                                   = false;
            LOGI("Borderless Fullscreen Mode Detected!")
        }
        break;
    case vkb::KeyCode::F3:        // FullscreenExclusiveEXT = Application Controlled
        if (full_screen_status != SwapchainMode::ExclusiveFullscreen)
        {
            full_screen_status                                         = SwapchainMode::ExclusiveFullscreen;
            application_window_status                                  = ApplicationWindowMode::Fullscreen;
            surface_full_screen_exclusive_info_EXT.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT;
            needs_recreate                                             = true;
            is_full_screen_exclusive                                   = true;
            LOGI("Exclusive Fullscreen Mode Detected!")
        }
        break;
    default:        // FullscreenExclusiveEXT = Default
        needs_recreate = false;
        break;
}
```

an ```Enum class``` is declared for the Swapchain mode, where:

```cpp
enum class SwapchainMode
{
    Default,
    Windowed,
    BorderlessFullscreen,
    ExclusiveFullscreen
};
```

And swapchain will be recreated if the display mode changes, where:

```cpp
if (isRecreate)
{
   FullScreenExclusive::recreate();
}
```