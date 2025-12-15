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
#pragma once

#include <functional>
#include <memory>
#include <string>

#if defined(PLATFORM_ANDROID)
#	include <android/asset_manager.h>
#	include <android/asset_manager_jni.h>
#	include <android/log.h>
#	include <android/native_activity.h>
#	include <game-activity/native_app_glue/android_native_app_glue.h>
#	define LOGI(...) ((void) __android_log_print(ANDROID_LOG_INFO, "SimpleEngine", __VA_ARGS__))
#	define LOGW(...) ((void) __android_log_print(ANDROID_LOG_WARN, "SimpleEngine", __VA_ARGS__))
#	define LOGE(...) ((void) __android_log_print(ANDROID_LOG_ERROR, "SimpleEngine", __VA_ARGS__))
#else
#	define GLFW_INCLUDE_VULKAN
#	include <GLFW/glfw3.h>
#	define LOGI(...)        \
		printf(__VA_ARGS__); \
		printf("\n")
#	define LOGW(...)        \
		printf(__VA_ARGS__); \
		printf("\n")
#	define LOGE(...)                 \
		fprintf(stderr, __VA_ARGS__); \
		fprintf(stderr, "\n")
#endif

/**
 * @brief Interface for platform-specific functionality.
 *
 * This class implements the platform abstraction as described in the Engine_Architecture chapter:
 * @see en/Building_a_Simple_Engine/Engine_Architecture/02_architectural_patterns.adoc
 */
class Platform
{
  public:
	/**
	 * @brief Default constructor.
	 */
	Platform() = default;

	/**
	 * @brief Virtual destructor for proper cleanup.
	 */
	virtual ~Platform() = default;

	/**
	 * @brief Initialize the platform.
	 * @param appName The name of the application.
	 * @param width The width of the window.
	 * @param height The height of the window.
	 * @return True if initialization was successful, false otherwise.
	 */
	virtual bool Initialize(const std::string &appName, int width, int height) = 0;

	/**
	 * @brief Clean up platform resources.
	 */
	virtual void Cleanup() = 0;

	/**
	 * @brief Process platform events.
	 * @return True if the application should continue running, false if it should exit.
	 */
	virtual bool ProcessEvents() = 0;

	/**
	 * @brief Check if the window has been resized.
	 * @return True if the window has been resized, false otherwise.
	 */
	virtual bool HasWindowResized() = 0;

	/**
	 * @brief Get the current window width.
	 * @return The window width.
	 */
	virtual int GetWindowWidth() const = 0;

	/**
	 * @brief Get the current window height.
	 * @return The window height.
	 */
	virtual int GetWindowHeight() const = 0;

	/**
	 * @brief Get the current window size.
	 * @param width Pointer to store the window width.
	 * @param height Pointer to store the window height.
	 */
	virtual void GetWindowSize(int *width, int *height) const
	{
		*width  = GetWindowWidth();
		*height = GetWindowHeight();
	}

	/**
	 * @brief Create a Vulkan surface.
	 * @param instance The Vulkan instance.
	 * @param surface Pointer to the surface handle to be filled.
	 * @return True if the surface was created successfully, false otherwise.
	 */
	virtual bool CreateVulkanSurface(VkInstance instance, VkSurfaceKHR *surface) = 0;

	/**
	 * @brief Set a callback for window resize events.
	 * @param callback The callback function to be called when the window is resized.
	 */
	virtual void SetResizeCallback(std::function<void(int, int)> callback) = 0;

	/**
	 * @brief Set a callback for mouse input events.
	 * @param callback The callback function to be called when mouse input is received.
	 */
	virtual void SetMouseCallback(std::function<void(float, float, uint32_t)> callback) = 0;

	/**
	 * @brief Set a callback for keyboard input events.
	 * @param callback The callback function to be called when keyboard input is received.
	 */
	virtual void SetKeyboardCallback(std::function<void(uint32_t, bool)> callback) = 0;

	/**
	 * @brief Set a callback for character input events.
	 * @param callback The callback function to be called when character input is received.
	 */
	virtual void SetCharCallback(std::function<void(uint32_t)> callback) = 0;

	/**
	 * @brief Set the window title.
	 * @param title The new window title.
	 */
	virtual void SetWindowTitle(const std::string &title) = 0;
};

#if defined(PLATFORM_ANDROID)
/**
 * @brief Android implementation of the Platform interface.
 */
class AndroidPlatform : public Platform
{
  private:
	android_app                                *app           = nullptr;
	int                                         width         = 0;
	int                                         height        = 0;
	bool                                        windowResized = false;
	std::function<void(int, int)>               resizeCallback;
	std::function<void(float, float, uint32_t)> mouseCallback;
	std::function<void(uint32_t, bool)>         keyboardCallback;
	std::function<void(uint32_t)>               charCallback;

	// Mobile-specific properties
	struct DeviceCapabilities
	{
		int                      apiLevel = 0;
		std::string              deviceModel;
		std::string              deviceManufacturer;
		int                      cpuCores         = 0;
		int64_t                  totalMemory      = 0;
		bool                     supportsVulkan   = false;
		bool                     supportsVulkan11 = false;
		bool                     supportsVulkan12 = false;
		std::vector<std::string> supportedVulkanExtensions;
	};

	DeviceCapabilities deviceCapabilities;
	bool               powerSavingMode   = false;
	bool               multiTouchEnabled = true;

	/**
	 * @brief Detect device capabilities for performance optimizations.
	 */
	void DetectDeviceCapabilities();

	/**
	 * @brief Set up power-saving mode based on battery level.
	 */
	void SetupPowerSavingMode();

	/**
	 * @brief Initialize touch input handling.
	 */
	void InitializeTouchInput();

  public:
	/**
	 * @brief Enable or disable power-saving mode.
	 * @param enable Whether to enable power-saving mode.
	 */
	void EnablePowerSavingMode(bool enable);

	/**
	 * @brief Check if power-saving mode is enabled.
	 * @return True if power-saving mode is enabled, false otherwise.
	 */
	bool IsPowerSavingModeEnabled() const
	{
		return powerSavingMode;
	}

	/**
	 * @brief Enable or disable multi-touch input.
	 * @param enable Whether to enable multi-touch input.
	 */
	void EnableMultiTouch(bool enable)
	{
		multiTouchEnabled = enable;
	}

	/**
	 * @brief Check if multi-touch input is enabled.
	 * @return True if multi-touch input is enabled, false otherwise.
	 */
	bool IsMultiTouchEnabled() const
	{
		return multiTouchEnabled;
	}

	/**
	 * @brief Get the device capabilities.
	 * @return The device capabilities.
	 */
	const DeviceCapabilities &GetDeviceCapabilities() const
	{
		return deviceCapabilities;
	}
	/**
	 * @brief Constructor with an Android app.
	 * @param androidApp The Android app.
	 */
	explicit AndroidPlatform(android_app *androidApp);

	/**
	 * @brief Initialize the platform.
	 * @param appName The name of the application.
	 * @param width The width of the window.
	 * @param height The height of the window.
	 * @return True if initialization was successful, false otherwise.
	 */
	bool Initialize(const std::string &appName, int width, int height) override;

	/**
	 * @brief Clean up platform resources.
	 */
	void Cleanup() override;

	/**
	 * @brief Process platform events.
	 * @return True if the application should continue running, false if it should exit.
	 */
	bool ProcessEvents() override;

	/**
	 * @brief Check if the window has been resized.
	 * @return True if the window has been resized, false otherwise.
	 */
	bool HasWindowResized() override;

	/**
	 * @brief Get the current window width.
	 * @return The window width.
	 */
	int GetWindowWidth() const override
	{
		return width;
	}

	/**
	 * @brief Get the current window height.
	 * @return The window height.
	 */
	int GetWindowHeight() const override
	{
		return height;
	}

	/**
	 * @brief Create a Vulkan surface.
	 * @param instance The Vulkan instance.
	 * @param surface Pointer to the surface handle to be filled.
	 * @return True if the surface was created successfully, false otherwise.
	 */
	bool CreateVulkanSurface(VkInstance instance, VkSurfaceKHR *surface) override;

	/**
	 * @brief Set a callback for window resize events.
	 * @param callback The callback function to be called when the window is resized.
	 */
	void SetResizeCallback(std::function<void(int, int)> callback) override;

	/**
	 * @brief Set a callback for mouse input events.
	 * @param callback The callback function to be called when mouse input is received.
	 */
	void SetMouseCallback(std::function<void(float, float, uint32_t)> callback) override;

	/**
	 * @brief Set a callback for keyboard input events.
	 * @param callback The callback function to be called when keyboard input is received.
	 */
	void SetKeyboardCallback(std::function<void(uint32_t, bool)> callback) override;

	/**
	 * @brief Set a callback for character input events.
	 * @param callback The callback function to be called when character input is received.
	 */
	void SetCharCallback(std::function<void(uint32_t)> callback) override;

	/**
	 * @brief Set the window title (no-op on Android).
	 * @param title The new window title.
	 */
	void SetWindowTitle(const std::string &title) override;

	/**
	 * @brief Get the Android app.
	 * @return The Android app.
	 */
	android_app *GetApp() const
	{
		return app;
	}

	/**
	 * @brief Get the asset manager.
	 * @return The asset manager.
	 */
	AAssetManager *GetAssetManager() const
	{
		return app ? app->activity->assetManager : nullptr;
	}
};
#else
/**
 * @brief Desktop implementation of the Platform interface.
 */
class DesktopPlatform final : public Platform
{
  private:
	GLFWwindow                                 *window        = nullptr;
	int                                         width         = 0;
	int                                         height        = 0;
	bool                                        windowResized = false;
	std::function<void(int, int)>               resizeCallback;
	std::function<void(float, float, uint32_t)> mouseCallback;
	std::function<void(uint32_t, bool)>         keyboardCallback;
	std::function<void(uint32_t)>               charCallback;

	/**
	 * @brief Static callback for GLFW window resize events.
	 * @param window The GLFW window.
	 * @param width The new width.
	 * @param height The new height.
	 */
	static void WindowResizeCallback(GLFWwindow *window, int width, int height);

	/**
	 * @brief Static callback for GLFW mouse position events.
	 * @param window The GLFW window.
	 * @param xpos The x-coordinate of the cursor.
	 * @param ypos The y-coordinate of the cursor.
	 */
	static void MousePositionCallback(GLFWwindow *window, double xpos, double ypos);

	/**
	 * @brief Static callback for GLFW mouse button events.
	 * @param window The GLFW window.
	 * @param button The mouse button that was pressed or released.
	 * @param action The action (GLFW_PRESS or GLFW_RELEASE).
	 * @param mods The modifier keys that were held down.
	 */
	static void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods);

	/**
	 * @brief Static callback for GLFW keyboard events.
	 * @param window The GLFW window.
	 * @param key The key that was pressed or released.
	 * @param scancode The system-specific scancode of the key.
	 * @param action The action (GLFW_PRESS, GLFW_RELEASE, or GLFW_REPEAT).
	 * @param mods The modifier keys that were held down.
	 */
	static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);

	/**
	 * @brief Static callback for GLFW character events.
	 * @param window The GLFW window.
	 * @param codepoint The Unicode code point of the character.
	 */
	static void CharCallback(GLFWwindow *window, unsigned int codepoint);

  public:
	/**
	 * @brief Default constructor.
	 */
	DesktopPlatform() = default;

	/**
	 * @brief Initialize the platform.
	 * @param appName The name of the application.
	 * @param width The width of the window.
	 * @param height The height of the window.
	 * @return True if initialization was successful, false otherwise.
	 */
	bool Initialize(const std::string &appName, int width, int height) override;

	/**
	 * @brief Clean up platform resources.
	 */
	void Cleanup() override;

	/**
	 * @brief Process platform events.
	 * @return True if the application should continue running, false if it should exit.
	 */
	bool ProcessEvents() override;

	/**
	 * @brief Check if the window has been resized.
	 * @return True if the window has been resized, false otherwise.
	 */
	bool HasWindowResized() override;

	/**
	 * @brief Get the current window width.
	 * @return The window width.
	 */
	int GetWindowWidth() const override
	{
		return width;
	}

	/**
	 * @brief Get the current window height.
	 * @return The window height.
	 */
	int GetWindowHeight() const override
	{
		return height;
	}

	/**
	 * @brief Create a Vulkan surface.
	 * @param instance The Vulkan instance.
	 * @param surface Pointer to the surface handle to be filled.
	 * @return True if the surface was created successfully, false otherwise.
	 */
	bool CreateVulkanSurface(VkInstance instance, VkSurfaceKHR *surface) override;

	/**
	 * @brief Set a callback for window resize events.
	 * @param callback The callback function to be called when the window is resized.
	 */
	void SetResizeCallback(std::function<void(int, int)> callback) override;

	/**
	 * @brief Set a callback for mouse input events.
	 * @param callback The callback function to be called when mouse input is received.
	 */
	void SetMouseCallback(std::function<void(float, float, uint32_t)> callback) override;

	/**
	 * @brief Set a callback for keyboard input events.
	 * @param callback The callback function to be called when keyboard input is received.
	 */
	void SetKeyboardCallback(std::function<void(uint32_t, bool)> callback) override;

	/**
	 * @brief Set a callback for character input events.
	 * @param callback The callback function to be called when character input is received.
	 */
	void SetCharCallback(std::function<void(uint32_t)> callback) override;

	/**
	 * @brief Set the window title.
	 * @param title The new window title.
	 */
	void SetWindowTitle(const std::string &title) override;

	/**
	 * @brief Get the GLFW window.
	 * @return The GLFW window.
	 */
	GLFWwindow *GetWindow() const
	{
		return window;
	}
};
#endif

/**
 * @brief Factory function for creating a platform instance.
 * @param args Arguments to pass to the platform constructor.
 * @return A unique pointer to the platform instance.
 */
template <typename... Args>
std::unique_ptr<Platform> CreatePlatform(Args &&...args)
{
#if defined(PLATFORM_ANDROID)
	return std::make_unique<AndroidPlatform>(std::forward<Args>(args)...);
#else
	return std::make_unique<DesktopPlatform>();
#endif
}
