/* Copyright (c) 2025 Holochip Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "platform.h"

#include <stdexcept>

#if defined(PLATFORM_ANDROID)
// Android platform implementation

AndroidPlatform::AndroidPlatform(android_app *androidApp) :
    app(androidApp)
{
	// Set up the app's user data
	app->userData = this;

	// Set up the command callback
	app->onAppCmd = [](android_app *app, int32_t cmd) {
		auto *platform = static_cast<AndroidPlatform *>(app->userData);

		switch (cmd)
		{
			case APP_CMD_INIT_WINDOW:
				if (app->window != nullptr)
				{
					// Get the window dimensions
					ANativeWindow *window   = app->window;
					platform->width         = ANativeWindow_getWidth(window);
					platform->height        = ANativeWindow_getHeight(window);
					platform->windowResized = true;

					// Call the resize callback if set
					if (platform->resizeCallback)
					{
						platform->resizeCallback(platform->width, platform->height);
					}
				}
				break;

			case APP_CMD_TERM_WINDOW:
				// Window is being hidden or closed
				break;

			case APP_CMD_WINDOW_RESIZED:
				if (app->window != nullptr)
				{
					// Get the new window dimensions
					ANativeWindow *window   = app->window;
					platform->width         = ANativeWindow_getWidth(window);
					platform->height        = ANativeWindow_getHeight(window);
					platform->windowResized = true;

					// Call the resize callback if set
					if (platform->resizeCallback)
					{
						platform->resizeCallback(platform->width, platform->height);
					}
				}
				break;

			default:
				break;
		}
	};
}

bool AndroidPlatform::Initialize(const std::string &appName, int requestedWidth, int requestedHeight)
{
	// On Android, the window dimensions are determined by the device
	if (app->window != nullptr)
	{
		width  = ANativeWindow_getWidth(app->window);
		height = ANativeWindow_getHeight(app->window);

		// Get device information for performance optimizations
		// This is important for mobile development to adapt to different device capabilities
		DetectDeviceCapabilities();

		// Set up power-saving mode based on battery level
		SetupPowerSavingMode();

		// Initialize touch input handling
		InitializeTouchInput();

		return true;
	}
	return false;
}

void AndroidPlatform::Cleanup()
{
	// Nothing to clean up for Android
}

bool AndroidPlatform::ProcessEvents()
{
	// Process Android events
	int                  events;
	android_poll_source *source;

	// Poll for events with a timeout of 0 (non-blocking)
	while (ALooper_pollAll(0, nullptr, &events, (void **) &source) >= 0)
	{
		if (source != nullptr)
		{
			source->process(app, source);
		}

		// Check if we are exiting
		if (app->destroyRequested != 0)
		{
			return false;
		}
	}

	return true;
}

bool AndroidPlatform::HasWindowResized()
{
	bool resized  = windowResized;
	windowResized = false;
	return resized;
}

bool AndroidPlatform::CreateVulkanSurface(VkInstance instance, VkSurfaceKHR *surface)
{
	if (app->window == nullptr)
	{
		return false;
	}

	VkAndroidSurfaceCreateInfoKHR createInfo{};
	createInfo.sType  = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
	createInfo.window = app->window;

	if (vkCreateAndroidSurfaceKHR(instance, &createInfo, nullptr, surface) != VK_SUCCESS)
	{
		return false;
	}

	return true;
}

void AndroidPlatform::SetResizeCallback(std::function<void(int, int)> callback)
{
	resizeCallback = std::move(callback);
}

void AndroidPlatform::SetMouseCallback(std::function<void(float, float, uint32_t)> callback)
{
	mouseCallback = std::move(callback);
}

void AndroidPlatform::SetKeyboardCallback(std::function<void(uint32_t, bool)> callback)
{
	keyboardCallback = std::move(callback);
}

void AndroidPlatform::SetCharCallback(std::function<void(uint32_t)> callback)
{
	charCallback = std::move(callback);
}

void AndroidPlatform::SetWindowTitle(const std::string &title)
{
	// No-op on Android - mobile apps don't have window titles
	(void) title;        // Suppress unused parameter warning
}

void AndroidPlatform::DetectDeviceCapabilities()
{
	if (!app)
	{
		return;
	}

	// Get API level
	JNIEnv *env = nullptr;
	app->activity->vm->AttachCurrentThread(&env, nullptr);
	if (env)
	{
		// Get Build.VERSION.SDK_INT
		jclass   versionClass       = env->FindClass("android/os/Build$VERSION");
		jfieldID sdkFieldID         = env->GetStaticFieldID(versionClass, "SDK_INT", "I");
		deviceCapabilities.apiLevel = env->GetStaticIntField(versionClass, sdkFieldID);

		// Get device model and manufacturer
		jclass   buildClass          = env->FindClass("android/os/Build");
		jfieldID modelFieldID        = env->GetStaticFieldID(buildClass, "MODEL", "Ljava/lang/String;");
		jfieldID manufacturerFieldID = env->GetStaticFieldID(buildClass, "MANUFACTURER", "Ljava/lang/String;");

		jstring modelJString        = (jstring) env->GetStaticObjectField(buildClass, modelFieldID);
		jstring manufacturerJString = (jstring) env->GetStaticObjectField(buildClass, manufacturerFieldID);

		const char *modelChars        = env->GetStringUTFChars(modelJString, nullptr);
		const char *manufacturerChars = env->GetStringUTFChars(manufacturerJString, nullptr);

		deviceCapabilities.deviceModel        = modelChars;
		deviceCapabilities.deviceManufacturer = manufacturerChars;

		env->ReleaseStringUTFChars(modelJString, modelChars);
		env->ReleaseStringUTFChars(manufacturerJString, manufacturerChars);

		// Get CPU cores
		jclass    runtimeClass        = env->FindClass("java/lang/Runtime");
		jmethodID getRuntime          = env->GetStaticMethodID(runtimeClass, "getRuntime", "()Ljava/lang/Runtime;");
		jobject   runtime             = env->CallStaticObjectMethod(runtimeClass, getRuntime);
		jmethodID availableProcessors = env->GetMethodID(runtimeClass, "availableProcessors", "()I");
		deviceCapabilities.cpuCores   = env->CallIntMethod(runtime, availableProcessors);

		// Get total memory
		jclass    activityManagerClass  = env->FindClass("android/app/ActivityManager");
		jclass    memoryInfoClass       = env->FindClass("android/app/ActivityManager$MemoryInfo");
		jmethodID memoryInfoConstructor = env->GetMethodID(memoryInfoClass, "<init>", "()V");
		jobject   memoryInfo            = env->NewObject(memoryInfoClass, memoryInfoConstructor);

		jmethodID getSystemService = env->GetMethodID(env->GetObjectClass(app->activity->clazz),
		                                              "getSystemService",
		                                              "(Ljava/lang/String;)Ljava/lang/Object;");
		jstring   serviceStr       = env->NewStringUTF("activity");
		jobject   activityManager  = env->CallObjectMethod(app->activity->clazz, getSystemService, serviceStr);

		jmethodID getMemoryInfo = env->GetMethodID(activityManagerClass, "getMemoryInfo",
		                                           "(Landroid/app/ActivityManager$MemoryInfo;)V");
		env->CallVoidMethod(activityManager, getMemoryInfo, memoryInfo);

		jfieldID totalMemField         = env->GetFieldID(memoryInfoClass, "totalMem", "J");
		deviceCapabilities.totalMemory = env->GetLongField(memoryInfo, totalMemField);

		env->DeleteLocalRef(serviceStr);

		// Check Vulkan support
		// In a real implementation, this would check for Vulkan support and available extensions
		deviceCapabilities.supportsVulkan   = true;
		deviceCapabilities.supportsVulkan11 = deviceCapabilities.apiLevel >= 28;        // Android 9 (Pie)
		deviceCapabilities.supportsVulkan12 = deviceCapabilities.apiLevel >= 29;        // Android 10

		// Add some common Vulkan extensions for mobile
		deviceCapabilities.supportedVulkanExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		deviceCapabilities.supportedVulkanExtensions.push_back(VK_KHR_MAINTENANCE1_EXTENSION_NAME);
		deviceCapabilities.supportedVulkanExtensions.push_back(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);

		if (deviceCapabilities.apiLevel >= 28)
		{
			deviceCapabilities.supportedVulkanExtensions.push_back(VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME);
			deviceCapabilities.supportedVulkanExtensions.push_back(VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME);
		}

		app->activity->vm->DetachCurrentThread();
	}

	LOGI("Device capabilities detected:");
	LOGI("  API Level: %d", deviceCapabilities.apiLevel);
	LOGI("  Device: %s by %s", deviceCapabilities.deviceModel.c_str(), deviceCapabilities.deviceManufacturer.c_str());
	LOGI("  CPU Cores: %d", deviceCapabilities.cpuCores);
	LOGI("  Total Memory: %lld bytes", (long long) deviceCapabilities.totalMemory);
	LOGI("  Vulkan Support: %s", deviceCapabilities.supportsVulkan ? "Yes" : "No");
	LOGI("  Vulkan 1.1 Support: %s", deviceCapabilities.supportsVulkan11 ? "Yes" : "No");
	LOGI("  Vulkan 1.2 Support: %s", deviceCapabilities.supportsVulkan12 ? "Yes" : "No");
}

void AndroidPlatform::SetupPowerSavingMode()
{
	if (!app)
	{
		return;
	}

	// Check battery level and status
	JNIEnv *env = nullptr;
	app->activity->vm->AttachCurrentThread(&env, nullptr);
	if (env)
	{
		// Get battery level
		jclass    intentFilterClass       = env->FindClass("android/content/IntentFilter");
		jmethodID intentFilterConstructor = env->GetMethodID(intentFilterClass, "<init>", "(Ljava/lang/String;)V");
		jstring   actionBatteryChanged    = env->NewStringUTF("android.intent.action.BATTERY_CHANGED");
		jobject   filter                  = env->NewObject(intentFilterClass, intentFilterConstructor, actionBatteryChanged);

		jmethodID registerReceiver = env->GetMethodID(env->GetObjectClass(app->activity->clazz),
		                                              "registerReceiver",
		                                              "(Landroid/content/BroadcastReceiver;Landroid/content/IntentFilter;)Landroid/content/Intent;");
		jobject   intent           = env->CallObjectMethod(app->activity->clazz, registerReceiver, nullptr, filter);

		if (intent)
		{
			// Get battery level
			jclass    intentClass = env->GetObjectClass(intent);
			jmethodID getIntExtra = env->GetMethodID(intentClass, "getIntExtra", "(Ljava/lang/String;I)I");

			jstring levelKey  = env->NewStringUTF("level");
			jstring scaleKey  = env->NewStringUTF("scale");
			jstring statusKey = env->NewStringUTF("status");

			int level  = env->CallIntMethod(intent, getIntExtra, levelKey, -1);
			int scale  = env->CallIntMethod(intent, getIntExtra, scaleKey, -1);
			int status = env->CallIntMethod(intent, getIntExtra, statusKey, -1);

			env->DeleteLocalRef(levelKey);
			env->DeleteLocalRef(scaleKey);
			env->DeleteLocalRef(statusKey);

			if (level != -1 && scale != -1)
			{
				float batteryPct = (float) level / (float) scale;

				// Enable power-saving mode if battery is low (below 20%) and not charging
				// Status values: 2 = charging, 3 = discharging, 4 = not charging, 5 = full
				bool isCharging = (status == 2 || status == 5);

				if (batteryPct < 0.2f && !isCharging)
				{
					EnablePowerSavingMode(true);
					LOGI("Battery level low (%.0f%%), enabling power-saving mode", batteryPct * 100.0f);
				}
				else
				{
					LOGI("Battery level: %.0f%%, %s", batteryPct * 100.0f, isCharging ? "charging" : "not charging");
				}
			}
		}

		env->DeleteLocalRef(actionBatteryChanged);
		app->activity->vm->DetachCurrentThread();
	}
}

void AndroidPlatform::InitializeTouchInput()
{
	if (!app)
	{
		return;
	}

	// Set up input handling for touch events
	app->onInputEvent = [](android_app *app, AInputEvent *event) -> int32_t {
		auto *platform = static_cast<AndroidPlatform *>(app->userData);

		if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION)
		{
			int32_t  action = AMotionEvent_getAction(event);
			uint32_t flags  = action & AMOTION_EVENT_ACTION_MASK;

			// Handle multi-touch if enabled
			int32_t pointerCount = AMotionEvent_getPointerCount(event);
			if (platform->IsMultiTouchEnabled() && pointerCount > 1)
			{
				// In a real implementation, this would handle multi-touch gestures
				// For now, just log the number of touch points
				LOGI("Multi-touch event with %d pointers", pointerCount);
			}

			// Convert touch event to mouse event for the engine
			if (platform->mouseCallback)
			{
				float x = AMotionEvent_getX(event, 0);
				float y = AMotionEvent_getY(event, 0);

				uint32_t buttons = 0;
				if (flags == AMOTION_EVENT_ACTION_DOWN || flags == AMOTION_EVENT_ACTION_MOVE)
				{
					buttons |= 0x01;        // Left button
				}

				platform->mouseCallback(x, y, buttons);
			}

			return 1;        // Event handled
		}

		return 0;        // Event not handled
	};

	LOGI("Touch input initialized");
}

void AndroidPlatform::EnablePowerSavingMode(bool enable)
{
	powerSavingMode = enable;

	// In a real implementation, this would adjust rendering quality, update frequency, etc.
	LOGI("Power-saving mode %s", enable ? "enabled" : "disabled");

	// Example of what would be done in a real implementation:
	// - Reduce rendering resolution
	// - Lower frame rate
	// - Disable post-processing effects
	// - Reduce draw distance
	// - Use simpler shaders
}

#else
// Desktop platform implementation

bool DesktopPlatform::Initialize(const std::string &appName, int requestedWidth, int requestedHeight)
{
	// Initialize GLFW
	if (!glfwInit())
	{
		throw std::runtime_error("Failed to initialize GLFW");
	}

	// GLFW was designed for OpenGL, so we need to tell it not to create an OpenGL context
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	// Create the window
	window = glfwCreateWindow(requestedWidth, requestedHeight, appName.c_str(), nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		throw std::runtime_error("Failed to create GLFW window");
	}

	// Set up the user pointer for callbacks
	glfwSetWindowUserPointer(window, this);

	// Set up the callbacks
	glfwSetFramebufferSizeCallback(window, WindowResizeCallback);
	glfwSetCursorPosCallback(window, MousePositionCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetCharCallback(window, CharCallback);

	// Get the initial window size
	glfwGetFramebufferSize(window, &width, &height);

	return true;
}

void DesktopPlatform::Cleanup()
{
	if (window)
	{
		glfwDestroyWindow(window);
		window = nullptr;
	}

	glfwTerminate();
}

bool DesktopPlatform::ProcessEvents()
{
	// Process GLFW events
	glfwPollEvents();

	// Check if the window should close
	return !glfwWindowShouldClose(window);
}

bool DesktopPlatform::HasWindowResized()
{
	bool resized  = windowResized;
	windowResized = false;
	return resized;
}

bool DesktopPlatform::CreateVulkanSurface(VkInstance instance, VkSurfaceKHR *surface)
{
	if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS)
	{
		return false;
	}

	return true;
}

void DesktopPlatform::SetResizeCallback(std::function<void(int, int)> callback)
{
	resizeCallback = std::move(callback);
}

void DesktopPlatform::SetMouseCallback(std::function<void(float, float, uint32_t)> callback)
{
	mouseCallback = std::move(callback);
}

void DesktopPlatform::SetKeyboardCallback(std::function<void(uint32_t, bool)> callback)
{
	keyboardCallback = std::move(callback);
}

void DesktopPlatform::SetCharCallback(std::function<void(uint32_t)> callback)
{
	charCallback = std::move(callback);
}

void DesktopPlatform::SetWindowTitle(const std::string &title)
{
	if (window)
	{
		glfwSetWindowTitle(window, title.c_str());
	}
}

void DesktopPlatform::WindowResizeCallback(GLFWwindow *window, int width, int height)
{
	auto *platform          = static_cast<DesktopPlatform *>(glfwGetWindowUserPointer(window));
	platform->width         = width;
	platform->height        = height;
	platform->windowResized = true;

	// Call the resize callback if set
	if (platform->resizeCallback)
	{
		platform->resizeCallback(width, height);
	}
}

void DesktopPlatform::MousePositionCallback(GLFWwindow *window, double xpos, double ypos)
{
	auto *platform = static_cast<DesktopPlatform *>(glfwGetWindowUserPointer(window));

	// Call the mouse callback if set
	if (platform->mouseCallback)
	{
		// Get the mouse button state
		uint32_t buttons = 0;
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
		{
			buttons |= 0x01;        // Left button
		}
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
		{
			buttons |= 0x02;        // Right button
		}
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS)
		{
			buttons |= 0x04;        // Middle button
		}

		platform->mouseCallback(static_cast<float>(xpos), static_cast<float>(ypos), buttons);
	}
}

void DesktopPlatform::MouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
	auto *platform = static_cast<DesktopPlatform *>(glfwGetWindowUserPointer(window));

	// Call the mouse callback if set
	if (platform->mouseCallback)
	{
		// Get the mouse position
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		// Get the mouse button state
		uint32_t buttons = 0;
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
		{
			buttons |= 0x01;        // Left button
		}
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
		{
			buttons |= 0x02;        // Right button
		}
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS)
		{
			buttons |= 0x04;        // Middle button
		}

		platform->mouseCallback(static_cast<float>(xpos), static_cast<float>(ypos), buttons);
	}
}

void DesktopPlatform::KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	auto *platform = static_cast<DesktopPlatform *>(glfwGetWindowUserPointer(window));

	// Call the keyboard callback if set
	if (platform->keyboardCallback)
	{
		platform->keyboardCallback(key, action != GLFW_RELEASE);
	}
}

void DesktopPlatform::CharCallback(GLFWwindow *window, unsigned int codepoint)
{
	auto *platform = static_cast<DesktopPlatform *>(glfwGetWindowUserPointer(window));

	// Call the char callback if set
	if (platform->charCallback)
	{
		platform->charCallback(codepoint);
	}
}
#endif
