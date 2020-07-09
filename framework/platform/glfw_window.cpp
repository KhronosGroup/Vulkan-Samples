/* Copyright (c) 2019-2020, Arm Limited and Contributors
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

#include "glfw_window.h"

#include <unordered_map>

#include "common/error.h"

VKBP_DISABLE_WARNINGS()
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
VKBP_ENABLE_WARNINGS()

#include "platform/platform.h"

namespace vkb
{
namespace
{
void error_callback(int error, const char *description)
{
	LOGE("GLFW Error (code {}): {}", error, description);
}

void window_close_callback(GLFWwindow *window)
{
	glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void window_size_callback(GLFWwindow *window, int width, int height)
{
	if (auto glfw_window = reinterpret_cast<GlfwWindow *>(glfwGetWindowUserPointer(window)))
	{
		auto &platform = glfw_window->get_platform();
		platform.get_app().resize(width, height);
		glfw_window->resize(width, height);
	}
}

void window_focus_callback(GLFWwindow *window, int focused)
{
	if (auto glfw_window = reinterpret_cast<GlfwWindow *>(glfwGetWindowUserPointer(window)))
	{
		auto &platform = glfw_window->get_platform();
		platform.get_app().set_focus(focused ? true : false);
	}
}

inline KeyCode translate_key_code(int key)
{
	static const std::unordered_map<int, KeyCode> key_lookup =
	    {
	        {GLFW_KEY_SPACE, KeyCode::Space},
	        {GLFW_KEY_APOSTROPHE, KeyCode::Apostrophe},
	        {GLFW_KEY_COMMA, KeyCode::Comma},
	        {GLFW_KEY_MINUS, KeyCode::Minus},
	        {GLFW_KEY_PERIOD, KeyCode::Period},
	        {GLFW_KEY_SLASH, KeyCode::Slash},
	        {GLFW_KEY_0, KeyCode::_0},
	        {GLFW_KEY_1, KeyCode::_1},
	        {GLFW_KEY_2, KeyCode::_2},
	        {GLFW_KEY_3, KeyCode::_3},
	        {GLFW_KEY_4, KeyCode::_4},
	        {GLFW_KEY_5, KeyCode::_5},
	        {GLFW_KEY_6, KeyCode::_6},
	        {GLFW_KEY_7, KeyCode::_7},
	        {GLFW_KEY_8, KeyCode::_8},
	        {GLFW_KEY_9, KeyCode::_9},
	        {GLFW_KEY_SEMICOLON, KeyCode::Semicolon},
	        {GLFW_KEY_EQUAL, KeyCode::Equal},
	        {GLFW_KEY_A, KeyCode::A},
	        {GLFW_KEY_B, KeyCode::B},
	        {GLFW_KEY_C, KeyCode::C},
	        {GLFW_KEY_D, KeyCode::D},
	        {GLFW_KEY_E, KeyCode::E},
	        {GLFW_KEY_F, KeyCode::F},
	        {GLFW_KEY_G, KeyCode::G},
	        {GLFW_KEY_H, KeyCode::H},
	        {GLFW_KEY_I, KeyCode::I},
	        {GLFW_KEY_J, KeyCode::J},
	        {GLFW_KEY_K, KeyCode::K},
	        {GLFW_KEY_L, KeyCode::L},
	        {GLFW_KEY_M, KeyCode::M},
	        {GLFW_KEY_N, KeyCode::N},
	        {GLFW_KEY_O, KeyCode::O},
	        {GLFW_KEY_P, KeyCode::P},
	        {GLFW_KEY_Q, KeyCode::Q},
	        {GLFW_KEY_R, KeyCode::R},
	        {GLFW_KEY_S, KeyCode::S},
	        {GLFW_KEY_T, KeyCode::T},
	        {GLFW_KEY_U, KeyCode::U},
	        {GLFW_KEY_V, KeyCode::V},
	        {GLFW_KEY_W, KeyCode::W},
	        {GLFW_KEY_X, KeyCode::X},
	        {GLFW_KEY_Y, KeyCode::Y},
	        {GLFW_KEY_Z, KeyCode::Z},
	        {GLFW_KEY_LEFT_BRACKET, KeyCode::LeftBracket},
	        {GLFW_KEY_BACKSLASH, KeyCode::Backslash},
	        {GLFW_KEY_RIGHT_BRACKET, KeyCode::RightBracket},
	        {GLFW_KEY_GRAVE_ACCENT, KeyCode::GraveAccent},
	        {GLFW_KEY_ESCAPE, KeyCode::Escape},
	        {GLFW_KEY_ENTER, KeyCode::Enter},
	        {GLFW_KEY_TAB, KeyCode::Tab},
	        {GLFW_KEY_BACKSPACE, KeyCode::Backspace},
	        {GLFW_KEY_INSERT, KeyCode::Insert},
	        {GLFW_KEY_DELETE, KeyCode::DelKey},
	        {GLFW_KEY_RIGHT, KeyCode::Right},
	        {GLFW_KEY_LEFT, KeyCode::Left},
	        {GLFW_KEY_DOWN, KeyCode::Down},
	        {GLFW_KEY_UP, KeyCode::Up},
	        {GLFW_KEY_PAGE_UP, KeyCode::PageUp},
	        {GLFW_KEY_PAGE_DOWN, KeyCode::PageDown},
	        {GLFW_KEY_HOME, KeyCode::Home},
	        {GLFW_KEY_END, KeyCode::End},
	        {GLFW_KEY_CAPS_LOCK, KeyCode::CapsLock},
	        {GLFW_KEY_SCROLL_LOCK, KeyCode::ScrollLock},
	        {GLFW_KEY_NUM_LOCK, KeyCode::NumLock},
	        {GLFW_KEY_PRINT_SCREEN, KeyCode::PrintScreen},
	        {GLFW_KEY_PAUSE, KeyCode::Pause},
	        {GLFW_KEY_F1, KeyCode::F1},
	        {GLFW_KEY_F2, KeyCode::F2},
	        {GLFW_KEY_F3, KeyCode::F3},
	        {GLFW_KEY_F4, KeyCode::F4},
	        {GLFW_KEY_F5, KeyCode::F5},
	        {GLFW_KEY_F6, KeyCode::F6},
	        {GLFW_KEY_F7, KeyCode::F7},
	        {GLFW_KEY_F8, KeyCode::F8},
	        {GLFW_KEY_F9, KeyCode::F9},
	        {GLFW_KEY_F10, KeyCode::F10},
	        {GLFW_KEY_F11, KeyCode::F11},
	        {GLFW_KEY_F12, KeyCode::F12},
	        {GLFW_KEY_KP_0, KeyCode::KP_0},
	        {GLFW_KEY_KP_1, KeyCode::KP_1},
	        {GLFW_KEY_KP_2, KeyCode::KP_2},
	        {GLFW_KEY_KP_3, KeyCode::KP_3},
	        {GLFW_KEY_KP_4, KeyCode::KP_4},
	        {GLFW_KEY_KP_5, KeyCode::KP_5},
	        {GLFW_KEY_KP_6, KeyCode::KP_6},
	        {GLFW_KEY_KP_7, KeyCode::KP_7},
	        {GLFW_KEY_KP_8, KeyCode::KP_8},
	        {GLFW_KEY_KP_9, KeyCode::KP_9},
	        {GLFW_KEY_KP_DECIMAL, KeyCode::KP_Decimal},
	        {GLFW_KEY_KP_DIVIDE, KeyCode::KP_Divide},
	        {GLFW_KEY_KP_MULTIPLY, KeyCode::KP_Multiply},
	        {GLFW_KEY_KP_SUBTRACT, KeyCode::KP_Subtract},
	        {GLFW_KEY_KP_ADD, KeyCode::KP_Add},
	        {GLFW_KEY_KP_ENTER, KeyCode::KP_Enter},
	        {GLFW_KEY_KP_EQUAL, KeyCode::KP_Equal},
	        {GLFW_KEY_LEFT_SHIFT, KeyCode::LeftShift},
	        {GLFW_KEY_LEFT_CONTROL, KeyCode::LeftControl},
	        {GLFW_KEY_LEFT_ALT, KeyCode::LeftAlt},
	        {GLFW_KEY_RIGHT_SHIFT, KeyCode::RightShift},
	        {GLFW_KEY_RIGHT_CONTROL, KeyCode::RightControl},
	        {GLFW_KEY_RIGHT_ALT, KeyCode::RightAlt},
	    };

	auto key_it = key_lookup.find(key);

	if (key_it == key_lookup.end())
	{
		return KeyCode::Unknown;
	}

	return key_it->second;
}

inline KeyAction translate_key_action(int action)
{
	if (action == GLFW_PRESS)
	{
		return KeyAction::Down;
	}
	else if (action == GLFW_RELEASE)
	{
		return KeyAction::Up;
	}
	else if (action == GLFW_REPEAT)
	{
		return KeyAction::Repeat;
	}

	return KeyAction::Unknown;
}

void key_callback(GLFWwindow *window, int key, int /*scancode*/, int action, int /*mods*/)
{
	KeyCode   key_code   = translate_key_code(key);
	KeyAction key_action = translate_key_action(action);

	if (auto glfw_window = reinterpret_cast<GlfwWindow *>(glfwGetWindowUserPointer(window)))
	{
		auto &platform = glfw_window->get_platform();
		platform.get_app().input_event(KeyInputEvent{platform, key_code, key_action});
	}
}

inline MouseButton translate_mouse_button(int button)
{
	if (button < GLFW_MOUSE_BUTTON_6)
	{
		return static_cast<MouseButton>(button);
	}

	return MouseButton::Unknown;
}

inline MouseAction translate_mouse_action(int action)
{
	if (action == GLFW_PRESS)
	{
		return MouseAction::Down;
	}
	else if (action == GLFW_RELEASE)
	{
		return MouseAction::Up;
	}

	return MouseAction::Unknown;
}

void cursor_position_callback(GLFWwindow *window, double xpos, double ypos)
{
	if (auto glfw_window = reinterpret_cast<GlfwWindow *>(glfwGetWindowUserPointer(window)))
	{
		auto &platform = glfw_window->get_platform();
		platform.get_app().input_event(MouseButtonInputEvent{
		    platform,
		    MouseButton::Unknown,
		    MouseAction::Move,
		    static_cast<float>(xpos),
		    static_cast<float>(ypos)});
	}
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int /*mods*/)
{
	MouseAction mouse_action = translate_mouse_action(action);

	if (auto glfw_window = reinterpret_cast<GlfwWindow *>(glfwGetWindowUserPointer(window)))
	{
		auto & platform = glfw_window->get_platform();
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		platform.get_app().input_event(MouseButtonInputEvent{
		    platform,
		    translate_mouse_button(button),
		    mouse_action,
		    static_cast<float>(xpos),
		    static_cast<float>(ypos)});
	}
}
}        // namespace

GlfwWindow::GlfwWindow(Platform &platform, uint32_t width, uint32_t height) :
    Window(platform, width, height)
{
	if (!glfwInit())
	{
		throw std::runtime_error("GLFW couldn't be initialized.");
	}

	glfwSetErrorCallback(error_callback);

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	auto &options = platform.get_app().get_options();

	if (options.contains("--width"))
	{
		width = static_cast<uint32_t>(options.get_int("--width"));
	}

	if (options.contains("--height"))
	{
		height = static_cast<uint32_t>(options.get_int("--height"));
	}

	handle = glfwCreateWindow(width, height, platform.get_app().get_name().c_str(), NULL, NULL);

	if (!handle)
	{
		throw std::runtime_error("Couldn't create glfw window.");
	}

	glfwSetWindowUserPointer(handle, this);

	glfwSetWindowCloseCallback(handle, window_close_callback);
	glfwSetWindowSizeCallback(handle, window_size_callback);
	glfwSetWindowFocusCallback(handle, window_focus_callback);
	glfwSetKeyCallback(handle, key_callback);
	glfwSetCursorPosCallback(handle, cursor_position_callback);
	glfwSetMouseButtonCallback(handle, mouse_button_callback);

	glfwSetInputMode(handle, GLFW_STICKY_KEYS, 1);
	glfwSetInputMode(handle, GLFW_STICKY_MOUSE_BUTTONS, 1);
}

GlfwWindow::~GlfwWindow()
{
	glfwTerminate();
}

VkSurfaceKHR GlfwWindow::create_surface(Instance &instance)
{
	if (instance.get_handle() == VK_NULL_HANDLE || !handle)
	{
		return VK_NULL_HANDLE;
	}

	VkSurfaceKHR surface;

	VkResult errCode = glfwCreateWindowSurface(instance.get_handle(), handle, NULL, &surface);

	if (errCode != VK_SUCCESS)
	{
		return VK_NULL_HANDLE;
	}

	return surface;
}

bool GlfwWindow::should_close()
{
	return glfwWindowShouldClose(handle);
}

void GlfwWindow::process_events()
{
	glfwPollEvents();
}

void GlfwWindow::close()
{
	glfwSetWindowShouldClose(handle, GLFW_TRUE);
}

/// @brief It calculates the dpi factor using the density from GLFW physical size
/// <a href="https://www.glfw.org/docs/latest/monitor_guide.html#monitor_size">GLFW docs for dpi</a>
float GlfwWindow::get_dpi_factor() const
{
	auto primary_monitor = glfwGetPrimaryMonitor();
	auto vidmode         = glfwGetVideoMode(primary_monitor);

	int width_mm, height_mm;
	glfwGetMonitorPhysicalSize(primary_monitor, &width_mm, &height_mm);

	// As suggested by the GLFW monitor guide
	static const float inch_to_mm       = 25.0f;
	static const float win_base_density = 96.0f;

	auto dpi        = static_cast<uint32_t>(vidmode->width / (width_mm / inch_to_mm));
	auto dpi_factor = dpi / win_base_density;
	return dpi_factor;
}

float GlfwWindow::get_content_scale_factor() const
{
	int fb_width, fb_height;
	glfwGetFramebufferSize(handle, &fb_width, &fb_height);
	int win_width, win_height;
	glfwGetWindowSize(handle, &win_width, &win_height);

	// We could return a 2D result here instead of a scalar,
	// but non-uniform scaling is very unlikely, and would
	// require significantly more changes in the IMGUI integration
	return static_cast<float>(win_width) / fb_width;
}

}        // namespace vkb
