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

This code sample demonstrates how to incorporate Vulkan ```full screen exclusive``` extension. One shall notice that,
this extension provides a solution for ```full screen exclusion``` issue on ```Windows``` platform, that
the ```application window``` cannot correctly adapt a ture ```exclusive```mode. Therefore, all this extension variables
and related methods are applicable on ```Windows``` platform only.

## Introduction

This sample provides a detailed procedure, which approaches to activate the full screen exclusive feature
on ```Windows```
platform. Furthermore, users may switch between ```windowed```, ```borderless fullscreen```,
and ```exclusive fullscreen``` modes using ```keyboard input events```. As ```full screen exclusive``` extension works
specifically for ```Windows``` platform, thus the mentioned feature only applies to this sample running on
the ```Windows``` platform. On other platforms, this sample simply renders a triangle (e.g., ```hello_triangle```
sample).

## *Reminder

This is an optional section.

It is commonly confusing when new users attempted to apply the ```full screen exclusive```
extension the very first time. One must notice that, by configuring the ```swapchain create info```
using ```full screen exclusive``` extension variables or functionalities **DOES NOT** automatically set
the ```application window``` to full screen exclusive mode. In fact, it merely configures the ```swapchain``` to provide
the specified feature for an ```application window``` which is already on full screen mode. Hence, it is important to
notice that, the correct approach to activate the ```full screen feature``` is listed as follows:

1) recreate the ```swapchain``` using ```full screen exclusive``` related features.
2) recreate the ```frame buffers``` with the ```swapchain``` created in the previous procedure.
3) configure the ```application window``` to ```fullscreen mode``` by using windowing SDK (e.g., ```Windows```
   or ```GLFW``` commands, etc.)
4) execute the ```acquire full screen exclusive EXT``` call.

## Full screen exclusive extension

Enable the ```full screen exclusive``` extension, and doing so requires the following instance and device extensions
listed as follows:

1. Instance extension:```VK_KHR_WIN32_SURFACE_EXTENSION_NAME```
2. Instance extension:```VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME```
3. Instance extension:```VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME```
4. Instance extension:```VK_KHR_SURFACE_EXTENSION_NAME```
5. Device extension: ```VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME```

Where, inside the function scope of ```init_instance()```:

```cpp
active_instance_extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
active_instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
active_instance_extensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
active_instance_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
```

If application is running on a ```Windows``` platform, it can be detected by using:

```cpp 
#if defined(VK_USE_PLATFORM_WIN32_KHR)
   // Windows platform functions and extensions only
#endif
```

If the target application might run on platforms other than ```Windows```, be certain to check for ```Windows``` in the
build to ensure those other platforms still compile.

In addition, device extensions are added in addition to the required extensions in the function scope
of ```init_device()```, where:

```cpp
active_device_extensions.push_back(VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME);
```

## Swapchain creation

```full screen exclusive``` extension features are required to be enabled during the ```swapchain``` creation process.
Where, an extension variable called ```surface_full_screen_exclusive_info_EXT``` is declared as a private class variable
if ```Windows``` platform is detected. In addition, ```surface_full_screen_exclusive_Win32_info_EXT``` is declared as
well, in order to support the ```full screen exclusive``` feature. Such that:

```cpp
VkSurfaceFullScreenExclusiveInfoEXT      surface_full_screen_exclusive_info_EXT{};
VkSurfaceFullScreenExclusiveWin32InfoEXT surface_full_screen_exclusive_Win32_info_EXT{};
```

Both variables are initialized in the function scope of ```initialize_windows()```, where:

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

One shall notice that, the ```sType``` of ```surface_full_screen_exclusive_Win32_info_EXT```
is ```VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_WIN32_INFO_EXT```
and its ```pNext``` is attached to a ```nullptr```. Meantime, the ```hmonitor``` must be attached to the hmonitor
variable representing the monitor is currently operating at, and in this sample, one may simply get it from using the
following ```Windows``` command:

```cpp
MonitorFromWindow(HWND_applicationWindow, MONITOR_DEFAULTTONEAREST);
```

More details can be found in the link:
[MonitorFromWindow function (winuser.h)](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-monitorfromwindow)

Where, ```HWND_applicationWindow``` is a ```hwnd``` represents the application windows ```handle```. Such handle is
declared as a private class variable as well:

```cpp
HWND HWND_applicationWindow{}; 
```

And it can be easily get by using the following ```Windows``` command while initializing the application:

```cpp
HWND_applicationWindow = GetActiveWindow();
```

More details can be found in the link:
[GetActiveWindow  function (winuser.h)](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getactivewindow)

The variable ```surface_full_screen_exclusive_info_EXT``` has its ```sType```
as ```VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT```; and its ```pNext``` is attached to
the ```surface_full_screen_exclusive_Win32_info_EXT```. Based on the ```full screen exclusive``` mode selection,
following ```Enums``` can be chosen per selection of the display mode, Where:

1. ```VK_FULL_SCREEN_EXCLUSIVE_DEFAULT_EXT```
2. ```VK_FULL_SCREEN_EXCLUSIVE_DESALLOWED_EXT```
3. ```VK_FULL_SCREEN_EXCLUSIVE_ALLOWED_EXT```
4. ```VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT```

The swapchain chain creation info in declared locally, in form of ```VkSwapchainCreateInfoKHR```, its ```sType``` is
```VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR```, and in ```Windows``` platform, its ```pNext``` should be attached to
the ```surface_full_screen_exclusive_info_EXT```, otherwise, its ```pNext``` is normally connected to a ```nullptr```.

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

This section introduces how to manually configure the ```application window``` modes, using ```Windows``` commands. In
this particular sample, a private function ```update_application_window()``` is introduced, which, it configures
the ```application window``` modes from ```windowed``` to ```fullscreen```, vice versa, Where:

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

In addition, an ```Enum class``` is introduced to detect the desired ```application window``` mode:

```cpp
enum class ApplicationWindowMode
{
    Windowed,
    Fullscreen
};
```

## Recreation

In case when a display mode is switched from one to another, the ```recreate()``` function will be called. As was
introduced, this function simply idles the ```logic device```, then teardown the current ```frame buffers```, and then
recreates the proper ```swapchain``` and its corresponding new ```frame buffers```. Most importantly,
the ```application window``` mode has to be reconfigured based on the display mode, where:

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

The input event function is overridden in order to help to select the desired display mode. Where, display modes can be
selected using keyboard input from F1 to F3, where:

1. F1: ```Windowed``` with ```VK_FULL_SCREEN_EXCLUSIVE_DISALLOWED_EXT``` and ```ApplicationWindowMode::Windowed```.
2. F2: ```Borderless Fullscreen``` with ```VK_FULL_SCREEN_EXCLUSIVE_ALLOWED_EXT```
   and ```ApplicationWindowMode::Fullscreen```.
3. F3: ```Exclusive Fullscreen``` with ```VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT```
   and ```ApplicationWindowMode::Fullscreen```.

Its corresponding code for the ```keyboard input event``` is addressed as follows:

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

Where, a ```enum class``` is defined for the display mode:

```cpp
enum class SwapchainMode
{
    Default,
    Windowed,
    BorderlessFullscreen,
    ExclusiveFullscreen
};
```

Meantime, per proper input events, swapchain recreation will be triggered, where:

```cpp
if (isRecreate)
{
   FullScreenExclusive::recreate();
}
```