/* Copyright (c) 2022, Arm Limited and Contributors
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

#include <components/windows/glfw.hpp>

#include <GLFW/glfw3.h>

namespace components
{
namespace windows
{
class GLFWCallbackHelper
{
  public:
	GLFWCallbackHelper(GLFWWindow &window) :
	    m_window{window}
	{
	}

	events::ChannelSenderPtr<PositionChangedEvent> &position_sender() const
	{
		return m_window.m_position_sender;
	}

	events::ChannelSenderPtr<ContentRectChangedEvent> &content_rect_sender() const
	{
		return m_window.m_content_rect_sender;
	}

	events::ChannelSenderPtr<FocusChangedEvent> &focus_sender() const
	{
		return m_window.m_focus_sender;
	}

	events::ChannelSenderPtr<ShouldCloseEvent> &should_close_sender() const
	{
		return m_window.m_should_close_sender;
	}

	events::ChannelSenderPtr<events::KeyEvent> &key_sender() const
	{
		return m_window.m_key_sender;
	}

	events::ChannelSenderPtr<events::CursorPositionEvent> &cursor_position_sender() const
	{
		return m_window.m_cursor_position_sender;
	}

	events::ChannelSenderPtr<events::TouchEvent> &touch_sender() const
	{
		return m_window.m_touch_sender;
	}

  private:
	GLFWWindow &m_window;
};

void error_callback(int /* error */, const char * /* description */)
{
	// TODO: logging
	// LOGE("GLFW Error (code {}): {}", error, description);
}

void window_close_callback(GLFWwindow *handle)
{
	glfwSetWindowShouldClose(handle, GLFW_TRUE);

	if (auto *helper = reinterpret_cast<GLFWCallbackHelper *>(glfwGetWindowUserPointer(handle)))
	{
		auto &sender = helper->should_close_sender();
		if (sender)
		{
			sender->push(ShouldCloseEvent{});
		}
	}
}

void window_size_callback(GLFWwindow *handle, int width, int height)
{
	if (auto *helper = reinterpret_cast<GLFWCallbackHelper *>(glfwGetWindowUserPointer(handle)))
	{
		auto &sender = helper->content_rect_sender();
		if (sender)
		{
			sender->push(ContentRectChangedEvent{static_cast<uint32_t>(width), static_cast<uint32_t>(height)});
		}
	}
}

void window_focus_callback(GLFWwindow *handle, int focused)
{
	if (auto *helper = reinterpret_cast<GLFWCallbackHelper *>(glfwGetWindowUserPointer(handle)))
	{
		auto &sender = helper->focus_sender();
		if (sender)
		{
			sender->push(FocusChangedEvent{focused == GLFW_FOCUSED});
		}
	}
}

inline events::KeyCode translate_key_code(int key)
{
	static const std::unordered_map<int, events::KeyCode> key_lookup =
	    {
	        {GLFW_KEY_SPACE, events::KeyCode::Space},
	        {GLFW_KEY_APOSTROPHE, events::KeyCode::Apostrophe},
	        {GLFW_KEY_COMMA, events::KeyCode::Comma},
	        {GLFW_KEY_MINUS, events::KeyCode::Minus},
	        {GLFW_KEY_PERIOD, events::KeyCode::Period},
	        {GLFW_KEY_SLASH, events::KeyCode::Slash},
	        {GLFW_KEY_0, events::KeyCode::_0},
	        {GLFW_KEY_1, events::KeyCode::_1},
	        {GLFW_KEY_2, events::KeyCode::_2},
	        {GLFW_KEY_3, events::KeyCode::_3},
	        {GLFW_KEY_4, events::KeyCode::_4},
	        {GLFW_KEY_5, events::KeyCode::_5},
	        {GLFW_KEY_6, events::KeyCode::_6},
	        {GLFW_KEY_7, events::KeyCode::_7},
	        {GLFW_KEY_8, events::KeyCode::_8},
	        {GLFW_KEY_9, events::KeyCode::_9},
	        {GLFW_KEY_SEMICOLON, events::KeyCode::Semicolon},
	        {GLFW_KEY_EQUAL, events::KeyCode::Equal},
	        {GLFW_KEY_A, events::KeyCode::A},
	        {GLFW_KEY_B, events::KeyCode::B},
	        {GLFW_KEY_C, events::KeyCode::C},
	        {GLFW_KEY_D, events::KeyCode::D},
	        {GLFW_KEY_E, events::KeyCode::E},
	        {GLFW_KEY_F, events::KeyCode::F},
	        {GLFW_KEY_G, events::KeyCode::G},
	        {GLFW_KEY_H, events::KeyCode::H},
	        {GLFW_KEY_I, events::KeyCode::I},
	        {GLFW_KEY_J, events::KeyCode::J},
	        {GLFW_KEY_K, events::KeyCode::K},
	        {GLFW_KEY_L, events::KeyCode::L},
	        {GLFW_KEY_M, events::KeyCode::M},
	        {GLFW_KEY_N, events::KeyCode::N},
	        {GLFW_KEY_O, events::KeyCode::O},
	        {GLFW_KEY_P, events::KeyCode::P},
	        {GLFW_KEY_Q, events::KeyCode::Q},
	        {GLFW_KEY_R, events::KeyCode::R},
	        {GLFW_KEY_S, events::KeyCode::S},
	        {GLFW_KEY_T, events::KeyCode::T},
	        {GLFW_KEY_U, events::KeyCode::U},
	        {GLFW_KEY_V, events::KeyCode::V},
	        {GLFW_KEY_W, events::KeyCode::W},
	        {GLFW_KEY_X, events::KeyCode::X},
	        {GLFW_KEY_Y, events::KeyCode::Y},
	        {GLFW_KEY_Z, events::KeyCode::Z},
	        {GLFW_KEY_LEFT_BRACKET, events::KeyCode::LeftBracket},
	        {GLFW_KEY_BACKSLASH, events::KeyCode::Backslash},
	        {GLFW_KEY_RIGHT_BRACKET, events::KeyCode::RightBracket},
	        {GLFW_KEY_GRAVE_ACCENT, events::KeyCode::GraveAccent},
	        {GLFW_KEY_ESCAPE, events::KeyCode::Escape},
	        {GLFW_KEY_ENTER, events::KeyCode::Enter},
	        {GLFW_KEY_TAB, events::KeyCode::Tab},
	        {GLFW_KEY_BACKSPACE, events::KeyCode::Backspace},
	        {GLFW_KEY_INSERT, events::KeyCode::Insert},
	        {GLFW_KEY_DELETE, events::KeyCode::DelKey},
	        {GLFW_KEY_RIGHT, events::KeyCode::Right},
	        {GLFW_KEY_LEFT, events::KeyCode::Left},
	        {GLFW_KEY_DOWN, events::KeyCode::Down},
	        {GLFW_KEY_UP, events::KeyCode::Up},
	        {GLFW_KEY_PAGE_UP, events::KeyCode::PageUp},
	        {GLFW_KEY_PAGE_DOWN, events::KeyCode::PageDown},
	        {GLFW_KEY_HOME, events::KeyCode::Home},
	        {GLFW_KEY_END, events::KeyCode::End},
	        {GLFW_KEY_CAPS_LOCK, events::KeyCode::CapsLock},
	        {GLFW_KEY_SCROLL_LOCK, events::KeyCode::ScrollLock},
	        {GLFW_KEY_NUM_LOCK, events::KeyCode::NumLock},
	        {GLFW_KEY_PRINT_SCREEN, events::KeyCode::PrintScreen},
	        {GLFW_KEY_PAUSE, events::KeyCode::Pause},
	        {GLFW_KEY_F1, events::KeyCode::F1},
	        {GLFW_KEY_F2, events::KeyCode::F2},
	        {GLFW_KEY_F3, events::KeyCode::F3},
	        {GLFW_KEY_F4, events::KeyCode::F4},
	        {GLFW_KEY_F5, events::KeyCode::F5},
	        {GLFW_KEY_F6, events::KeyCode::F6},
	        {GLFW_KEY_F7, events::KeyCode::F7},
	        {GLFW_KEY_F8, events::KeyCode::F8},
	        {GLFW_KEY_F9, events::KeyCode::F9},
	        {GLFW_KEY_F10, events::KeyCode::F10},
	        {GLFW_KEY_F11, events::KeyCode::F11},
	        {GLFW_KEY_F12, events::KeyCode::F12},
	        {GLFW_KEY_KP_0, events::KeyCode::KP_0},
	        {GLFW_KEY_KP_1, events::KeyCode::KP_1},
	        {GLFW_KEY_KP_2, events::KeyCode::KP_2},
	        {GLFW_KEY_KP_3, events::KeyCode::KP_3},
	        {GLFW_KEY_KP_4, events::KeyCode::KP_4},
	        {GLFW_KEY_KP_5, events::KeyCode::KP_5},
	        {GLFW_KEY_KP_6, events::KeyCode::KP_6},
	        {GLFW_KEY_KP_7, events::KeyCode::KP_7},
	        {GLFW_KEY_KP_8, events::KeyCode::KP_8},
	        {GLFW_KEY_KP_9, events::KeyCode::KP_9},
	        {GLFW_KEY_KP_DECIMAL, events::KeyCode::KP_Decimal},
	        {GLFW_KEY_KP_DIVIDE, events::KeyCode::KP_Divide},
	        {GLFW_KEY_KP_MULTIPLY, events::KeyCode::KP_Multiply},
	        {GLFW_KEY_KP_SUBTRACT, events::KeyCode::KP_Subtract},
	        {GLFW_KEY_KP_ADD, events::KeyCode::KP_Add},
	        {GLFW_KEY_KP_ENTER, events::KeyCode::KP_Enter},
	        {GLFW_KEY_KP_EQUAL, events::KeyCode::KP_Equal},
	        {GLFW_KEY_LEFT_SHIFT, events::KeyCode::LeftShift},
	        {GLFW_KEY_LEFT_CONTROL, events::KeyCode::LeftControl},
	        {GLFW_KEY_LEFT_ALT, events::KeyCode::LeftAlt},
	        {GLFW_KEY_RIGHT_SHIFT, events::KeyCode::RightShift},
	        {GLFW_KEY_RIGHT_CONTROL, events::KeyCode::RightControl},
	        {GLFW_KEY_RIGHT_ALT, events::KeyCode::RightAlt},
	    };

	auto key_it = key_lookup.find(key);

	if (key_it == key_lookup.end())
	{
		return events::KeyCode::Unknown;
	}

	return key_it->second;
}

inline events::KeyAction translate_key_action(int action)
{
	if (action == GLFW_PRESS)
	{
		return events::KeyAction::Down;
	}
	else if (action == GLFW_RELEASE)
	{
		return events::KeyAction::Up;
	}
	else if (action == GLFW_REPEAT)
	{
		return events::KeyAction::Repeat;
	}

	return events::KeyAction::Unknown;
}

void key_callback(GLFWwindow *window, int key, int /*scancode*/, int action, int /*mods*/)
{
	events::KeyCode   key_code   = translate_key_code(key);
	events::KeyAction key_action = translate_key_action(action);

	if (auto *helper = reinterpret_cast<GLFWCallbackHelper *>(glfwGetWindowUserPointer(window)))
	{
		auto &sender = helper->key_sender();
		if (sender)
		{
			sender->push(events::KeyEvent{key_code, key_action});
		}
	}
}

inline events::KeyCode translate_mouse_key(int code)
{
	if (code == GLFW_MOUSE_BUTTON_LEFT)
	{
		return events::KeyCode::Mouse_Left;
	}
	if (code == GLFW_MOUSE_BUTTON_RIGHT)
	{
		return events::KeyCode::Mouse_Right;
	}
	if (code == GLFW_MOUSE_BUTTON_MIDDLE)
	{
		return events::KeyCode::Mouse_Middle;
	}

	return events::KeyCode::Unknown;
}

inline events::KeyAction translate_mouse_action(int action)
{
	if (action == GLFW_PRESS)
	{
		return events::KeyAction::Down;
	}
	else if (action == GLFW_RELEASE)
	{
		return events::KeyAction::Up;
	}

	return events::KeyAction::Unknown;
}

void cursor_position_callback(GLFWwindow *window, double xpos, double ypos)
{
	if (auto *helper = reinterpret_cast<GLFWCallbackHelper *>(glfwGetWindowUserPointer(window)))
	{
		auto &sender = helper->cursor_position_sender();
		if (sender)
		{
			sender->push(events::CursorPositionEvent{static_cast<float>(xpos), static_cast<float>(ypos)});
		}
	}
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int /*mods*/)
{
	auto key_code     = translate_key_code(button);
	auto mouse_action = translate_mouse_action(action);

	if (auto *helper = reinterpret_cast<GLFWCallbackHelper *>(glfwGetWindowUserPointer(window)))
	{
		auto &sender = helper->key_sender();
		if (sender)
		{
			sender->push(events::KeyEvent{key_code, mouse_action});
		}
	}
}

GLFWWindow::GLFWWindow(const std::string &title, const Extent &initial_extent) :
    Window{},
    m_title{title},
    m_callback_helper{std::make_unique<GLFWCallbackHelper>(*this)}
{
	// TODO: GLFW_X11_XCB_VULKAN_SURFACE

	if (!glfwInit())
	{
		throw std::runtime_error("GLFW couldn't be initialized.");
	}

	glfwSetErrorCallback(error_callback);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	// TODO: do we want to support fullscreen and fullscreen borderless still?

	m_handle = glfwCreateWindow(static_cast<int>(initial_extent.width), static_cast<int>(initial_extent.height), m_title.c_str(), NULL, NULL);
	if (!m_handle)
	{
		throw std::runtime_error("Couldn't create glfw window.");
	}

	glfwSetWindowUserPointer(m_handle, m_callback_helper.get());
	glfwSetWindowCloseCallback(m_handle, window_close_callback);
	glfwSetWindowSizeCallback(m_handle, window_size_callback);
	glfwSetWindowFocusCallback(m_handle, window_focus_callback);
	glfwSetKeyCallback(m_handle, key_callback);
	glfwSetCursorPosCallback(m_handle, cursor_position_callback);
	glfwSetMouseButtonCallback(m_handle, mouse_button_callback);

	glfwSetInputMode(m_handle, GLFW_STICKY_KEYS, 1);
	glfwSetInputMode(m_handle, GLFW_STICKY_MOUSE_BUTTONS, 1);
}

GLFWWindow::~GLFWWindow()
{
	glfwDestroyWindow(m_handle);
}

void GLFWWindow::set_extent(const Extent &extent)
{
	glfwSetWindowSize(m_handle, static_cast<int>(extent.width), static_cast<int>(extent.height));
	if (m_content_rect_sender)
	{
		m_content_rect_sender->push(ContentRectChangedEvent{extent});
	}
}

Extent GLFWWindow::extent() const
{
	int width;
	int height;
	glfwGetWindowSize(m_handle, &width, &height);
	return Extent{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}

void GLFWWindow::set_position(const Position &position)
{
	glfwSetWindowPos(m_handle, static_cast<int>(position.x), static_cast<int>(position.y));
	if (m_position_sender)
	{
		m_position_sender->push(PositionChangedEvent{position});
	}
}

Position GLFWWindow::position() const
{
	int x;
	int y;
	glfwGetWindowPos(m_handle, &x, &y);
	return Position{static_cast<uint32_t>(x), static_cast<uint32_t>(y)};
}

float GLFWWindow::dpi_factor() const
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

void GLFWWindow::set_title(const std::string &title)
{
	glfwSetWindowTitle(m_handle, title.c_str());
	m_title = title;
}

std::string_view GLFWWindow::title() const
{
	return m_title;
}

void GLFWWindow::update()
{
	glfwPollEvents();
}

void GLFWWindow::attach(events::EventBus &bus)
{
	m_content_rect_sender = bus.request_sender<ContentRectChangedEvent>();

	{
		int width;
		int height;
		glfwGetWindowSize(m_handle, &width, &height);
		m_content_rect_sender->push(ContentRectChangedEvent{Extent{static_cast<uint32_t>(width), static_cast<uint32_t>(height)}});
	}

	m_position_sender = bus.request_sender<PositionChangedEvent>();

	{
		int x;
		int y;
		glfwGetWindowPos(m_handle, &x, &y);
		m_position_sender->push(PositionChangedEvent{Position{static_cast<uint32_t>(x), static_cast<uint32_t>(y)}});
	}
}

}        // namespace windows
}        // namespace components