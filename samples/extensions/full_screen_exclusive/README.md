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

This code sample demonstrates how to incorporate Vulkan ```VK_EXT_full_screen_exclusive ``` extension. One shall notice
that, this extension provides a solution for full screen exclusion issue on Windows platform, which its application
window cannot correctly adapt a ture exclusive mode. Notice that, ```VK_EXT_full_screen_exclusive ```
is applicable on Windows platform alone.

## Introduction

This sample provides a detailed procedure, which activates full screen exclusive mode on Windows applications. Users may
choose application display mode from: 1) windowed, 2) borderless fullscreen, and 3) exclusive fullscreen
using ```keyboard input events```. As ```VK_EXT_full_screen_exclusive``` extension works specifically for Windows
platform, the mentioned feature only applies to the Windows platform. On other platforms, this sample simply renders a
triangle (e.g., ```hello_triangle```).

## *Reminder

This is an optional section.

It is commonly confusing when new users attempted to apply the full screen exclusive extension the very first time. One
must notice that, by configuring the ```swapchain create info```
using full screen exclusive extension variables or functions **DOES NOT** automatically set the application window to
full screen mode. In fact, it merely configures the ```swapchain``` to provide the correct ```swapchain images``` for
its application window. Hence, it is important to follow the correct procedure to activate the full screen exclusive
mode:

1) recreate the ```swapchain``` using ```full screen exclusive``` related features.
2) recreate the ```frame buffers``` with that ```swapchain``` created in the previous step.
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
active_instance_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
```

If application is running on a Windows platform, it can be detected by using:

```cpp 
#if defined(VK_USE_PLATFORM_WIN32_KHR)
   // Windows platform functions and extensions only
#endif
```

If the target application might run on platforms other than Windows, be certain to check for Windows in the build to
ensure those other platforms still compile.

Device extensions are added in ```init_device()```, where:

```cpp
active_device_extensions.push_back(VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME);
```

## Swapchain creation

Two variables are introduced during the ```swapchain``` creation process.
Where,```surface_full_screen_exclusive_info_EXT``` and ```surface_full_screen_exclusive_Win32_info_EXT```
are declared if a Windows platform is detected:

```cpp
VkSurfaceFullScreenExclusiveInfoEXT      surface_full_screen_exclusive_info_EXT{};
VkSurfaceFullScreenExclusiveWin32InfoEXT surface_full_screen_exclusive_Win32_info_EXT{};
```

Both variables are initialized using```initialize_windows()```:

```cpp
void FullScreenExclusive::initialize_windows()
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
	// The following variable has to be attached to the pNext of surface_full_screen_exclusive_info_EXT:
	surface_full_screen_exclusive_Win32_info_EXT.sType    = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_WIN32_INFO_EXT;
	surface_full_screen_exclusive_Win32_info_EXT.pNext    = nullptr;
	surface_full_screen_exclusive_Win32_info_EXT.hmonitor = MonitorFromWindow(HWND_applicationWindow, MONITOR_DEFAULTTONEAREST);

	surface_full_screen_exclusive_info_EXT.sType               = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT;
	surface_full_screen_exclusive_info_EXT.pNext               = &surface_full_screen_exclusive_Win32_info_EXT;
	surface_full_screen_exclusive_info_EXT.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_DEFAULT_EXT;        // Set the fullScreenExclusive stage to default when initializing
#endif
}
```

```sType``` of ```surface_full_screen_exclusive_Win32_info_EXT```
is ```VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_WIN32_INFO_EXT```
and the ```pNext``` is attached to a ```nullptr```. Meantime, the ```hmonitor``` in this sample, one can get it using
the following Windows command:

```cpp
MonitorFromWindow(HWND_applicationWindow, MONITOR_DEFAULTTONEAREST);
```

More details can be found in the link:
[MonitorFromWindow function (winuser.h)](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-monitorfromwindow)

```HWND_applicationWindow``` is a ```hwnd``` represents the application windows handle. Such handle is declared as a
private variable, where:

```cpp
HWND HWND_applicationWindow{}; 
```

one can get it using the following Windows command:

```cpp
HWND_applicationWindow = GetActiveWindow();
```

More details can be found in the link:
[GetActiveWindow  function (winuser.h)](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getactivewindow)

```sType``` of ```surface_full_screen_exclusive_info_EXT``` is
```VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT```, and its ```pNext``` is connected to
```surface_full_screen_exclusive_Win32_info_EXT```. To specify the display mode, the following ```Enums``` can be chosen
from, Where:

1. ```VK_FULL_SCREEN_EXCLUSIVE_DEFAULT_EXT```
2. ```VK_FULL_SCREEN_EXCLUSIVE_DISALLOWED_EXT```
3. ```VK_FULL_SCREEN_EXCLUSIVE_ALLOWED_EXT```
4. ```VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT```

The swapchain chain creation info in declared locally, its ```sType``` is
```VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR```, and in Windows platform, its ```pNext``` should be attached to
the ```surface_full_screen_exclusive_info_EXT```, otherwise, its ```pNext``` is connected to a ```nullptr```.

```cpp
VkSwapchainCreateInfoKHR info{};
info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;

#if defined(VK_USE_PLATFORM_WIN32_KHR)
    info.pNext = &surface_full_screen_exclusive_info_EXT;
#else
    info.pNext = nullptr;
#endif
```

Rest of the ```swapchain create info``` are configured normally.

## *Application window modes

This is an optional section.

This section introduces how to configure application window modes using Windows commands. In this particular sample, a
private function ```update_application_window()``` is introduced, which, it switches the application window mode between
windowed and fullscreen:

```cpp
void FullScreenExclusive::update_application_window()
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
	if (application_window_status == ApplicationWindowMode::Fullscreen && isWindowed)
	{
		isWindowed = false;

		GetWindowPlacement(HWND_applicationWindow, &wpc);
		if (HWND_style == 0)
			HWND_style = GetWindowLong(HWND_applicationWindow, GWL_STYLE);
		if (HWND_style_previous == 0)
			HWND_style_previous = GetWindowLong(HWND_applicationWindow, GWL_EXSTYLE);

		long HWND_newStyle = HWND_style;
		HWND_newStyle &= ~WS_BORDER;
		HWND_newStyle &= ~WS_DLGFRAME;
		HWND_newStyle &= ~WS_THICKFRAME;

		long HWND_newStyle_previous = HWND_style_previous;
		HWND_newStyle_previous &= ~WS_EX_WINDOWEDGE;

		SetWindowLong(HWND_applicationWindow, GWL_STYLE, HWND_newStyle | WS_POPUP);
		SetWindowLong(HWND_applicationWindow, GWL_EXSTYLE, HWND_newStyle_previous | WS_EX_TOPMOST);
		ShowWindow(HWND_applicationWindow, SW_SHOWMAXIMIZED);
	}
	else if (application_window_status == ApplicationWindowMode::Windowed && !isWindowed)
	{
		isWindowed = true;

		SetWindowLong(HWND_applicationWindow, GWL_STYLE, HWND_style);
		SetWindowLong(HWND_applicationWindow, GWL_EXSTYLE, HWND_style_previous);
		ShowWindow(HWND_applicationWindow, SW_SHOWNORMAL);
		SetWindowPlacement(HWND_applicationWindow, &wpc);
	}
#endif
}
```

Variables appeared in the above function were declared and initialized as private variables, such that:

```cpp
HWND                  HWND_applicationWindow{};
bool                  isWindowed = true;                                                
WINDOWPLACEMENT       wpc{};                                                            
long                  HWND_style          = 0;
long                  HWND_style_previous = 0;
ApplicationWindowMode application_window_status = ApplicationWindowMode::Windowed; 
```

In addition, an ```Enum class``` is created to flag the current application window mode:

```cpp
enum class ApplicationWindowMode
{
    Windowed,
    Fullscreen
};
```

## Recreation

In case when a display mode is switched from one to another, ```recreate()``` will be called. This function idles
the ```logic device```, tears down the current ```frame buffers```, then recreates the new ```swapchain```
and ```frame buffers```, and reconfigures the application window mode:

```cpp
void FullScreenExclusive::recreate()
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
	// Check if there IS a device, if not don't do anything
	if (context.device != VK_NULL_HANDLE)
	{
		// Step: 0) idles the device, destroy/teardown the current swapchain and frame buffers.
		vkDeviceWaitIdle(context.device);        // pause the renderer
		teardown_frame_buffers(context);         // basically destroy everything swapchain related

		// Step: 1) recreate the swapchain with its properly selected FullscreenExclusive enum value
		init_swapchain(context);

		// Step: 2) recreate the frame buffers using the newly created swapchain
		init_frame_buffers(context);

		// Step: 3-1) update the window mode, corresponding to the FullscreenExclusive enum value
		update_application_window();

		// Step: 3-2) remember: ALWAYS change the application window mode BEFORE acquire the full screen exclusive mode!
		if (isFullScreenExclusive)
		{
			VkResult result = vkAcquireFullScreenExclusiveModeEXT(context.device, context.swapchain);
			if (result == VK_SUCCESS)
			{
				LOGI("vkAcquireFullScreenExclusiveModeEXT result: VK_SUCCESS!")
			}
		}
	}
#endif
}
```

## Input events

Display mode can be selected using keyboard input from F1 to F3, where:

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
            isRecreate                                                 = true;
            isFullScreenExclusive                                      = false;
            LOGI("Windowed Mode Detected!")
        }
        break;
    case vkb::KeyCode::F2:        // FullscreenExclusiveEXT = Allowed
        if (full_screen_status != SwapchainMode::BorderlessFullscreen)
        {
            full_screen_status                                         = SwapchainMode::BorderlessFullscreen;
            application_window_status                                  = ApplicationWindowMode::Fullscreen;
            surface_full_screen_exclusive_info_EXT.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_ALLOWED_EXT;
            isRecreate                                                 = true;
            isFullScreenExclusive                                      = false;
            LOGI("Borderless Fullscreen Mode Detected!")
        }
        break;
    case vkb::KeyCode::F3:        // FullscreenExclusiveEXT = Application Controlled
        if (full_screen_status != SwapchainMode::ExclusiveFullscreen)
        {
            full_screen_status                                         = SwapchainMode::ExclusiveFullscreen;
            application_window_status                                  = ApplicationWindowMode::Fullscreen;
            surface_full_screen_exclusive_info_EXT.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT;
            isRecreate                                                 = true;
            isFullScreenExclusive                                      = true;
            LOGI("Exclusive Fullscreen Mode Detected!")
        }
        break;
    default:        // FullscreenExclusiveEXT = Default
        isRecreate = false;
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