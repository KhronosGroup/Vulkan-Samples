/* Copyright (c) 2019, Arm Limited and Contributors
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

#include <cstddef>
#include <cstdint>

namespace vkb
{
class Platform;

enum class EventSource
{
	Keyboard,
	Mouse,
	Touchscreen
};

class InputEvent
{
  public:
	InputEvent(Platform &platform, EventSource source);

	const Platform &get_platform() const;

	EventSource get_source() const;

  private:
	Platform &platform;

	EventSource source;
};

enum class KeyCode
{
	Unknown,
	Space,
	Apostrophe, /* ' */
	Comma,      /* , */
	Minus,      /* - */
	Period,     /* . */
	Slash,      /* / */
	_0,
	_1,
	_2,
	_3,
	_4,
	_5,
	_6,
	_7,
	_8,
	_9,
	Semicolon, /* ; */
	Equal,     /* = */
	A,
	B,
	C,
	D,
	E,
	F,
	G,
	H,
	I,
	J,
	K,
	L,
	M,
	N,
	O,
	P,
	Q,
	R,
	S,
	T,
	U,
	V,
	W,
	X,
	Y,
	Z,
	LeftBracket,  /* [ */
	Backslash,    /* \ */
	RightBracket, /* ] */
	GraveAccent,  /* ` */
	Escape,
	Enter,
	Tab,
	Backspace,
	Insert,
	DelKey,
	Right,
	Left,
	Down,
	Up,
	PageUp,
	PageDown,
	Home,
	End,
	Back,
	CapsLock,
	ScrollLock,
	NumLock,
	PrintScreen,
	Pause,
	F1,
	F2,
	F3,
	F4,
	F5,
	F6,
	F7,
	F8,
	F9,
	F10,
	F11,
	F12,
	KP_0,
	KP_1,
	KP_2,
	KP_3,
	KP_4,
	KP_5,
	KP_6,
	KP_7,
	KP_8,
	KP_9,
	KP_Decimal,
	KP_Divide,
	KP_Multiply,
	KP_Subtract,
	KP_Add,
	KP_Enter,
	KP_Equal,
	LeftShift,
	LeftControl,
	LeftAlt,
	RightShift,
	RightControl,
	RightAlt
};

enum class KeyAction
{
	Down,
	Up,
	Repeat,
	Unknown
};

class KeyInputEvent : public InputEvent
{
  public:
	KeyInputEvent(Platform &platform, KeyCode code, KeyAction action);

	KeyCode get_code() const;

	KeyAction get_action() const;

  private:
	KeyCode code;

	KeyAction action;
};

enum class MouseButton
{
	Left,
	Right,
	Middle,
	Back,
	Forward,
	Unknown
};

enum class MouseAction
{
	Down,
	Up,
	Move,
	Unknown
};

class MouseButtonInputEvent : public InputEvent
{
  public:
	MouseButtonInputEvent(Platform &platform, MouseButton button, MouseAction action, float pos_x, float pos_y);

	MouseButton get_button() const;

	MouseAction get_action() const;

	float get_pos_x() const;

	float get_pos_y() const;

  private:
	MouseButton button;

	MouseAction action;

	float pos_x;

	float pos_y;
};

enum class TouchAction
{
	Down,
	Up,
	Move,
	Cancel,
	PointerDown,
	PointerUp,
	Unknown
};

class TouchInputEvent : public InputEvent
{
  public:
	TouchInputEvent(Platform &platform, int32_t pointer_id, size_t pointer_count, TouchAction action, float pos_x, float pos_y);

	TouchAction get_action() const;

	int32_t get_pointer_id() const;

	size_t get_touch_points() const;

	float get_pos_x() const;

	float get_pos_y() const;

  private:
	TouchAction action;

	int32_t pointer_id;

	size_t touch_points;

	float pos_x;

	float pos_y;
};
}        // namespace vkb
