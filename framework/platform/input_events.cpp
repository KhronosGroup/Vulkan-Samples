/* Copyright (c) 2019-2021, Arm Limited and Contributors
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

#include "input_events.h"

namespace vkb
{
InputEvent::InputEvent(EventSource source) :
    source{source}
{
}

EventSource InputEvent::get_source() const
{
	return source;
}

KeyInputEvent::KeyInputEvent(KeyCode code, KeyAction action) :
    InputEvent{EventSource::Keyboard},
    code{code},
    action{action}
{
}

KeyCode KeyInputEvent::get_code() const
{
	return code;
}

KeyAction KeyInputEvent::get_action() const
{
	return action;
}

MouseButtonInputEvent::MouseButtonInputEvent(MouseButton button, MouseAction action, float pos_x, float pos_y) :
    InputEvent{EventSource::Mouse},
    button{button},
    action{action},
    pos_x{pos_x},
    pos_y{pos_y}
{
}

MouseButton MouseButtonInputEvent::get_button() const
{
	return button;
}

MouseAction MouseButtonInputEvent::get_action() const
{
	return action;
}

float MouseButtonInputEvent::get_pos_x() const
{
	return pos_x;
}

float MouseButtonInputEvent::get_pos_y() const
{
	return pos_y;
}

TouchInputEvent::TouchInputEvent(int32_t pointer_id, std::size_t touch_points, TouchAction action, float pos_x, float pos_y) :
    InputEvent{EventSource::Touchscreen},
    action{action},
    pointer_id{pointer_id},
    touch_points{touch_points},
    pos_x{pos_x},
    pos_y{pos_y}
{
}

TouchAction TouchInputEvent::get_action() const
{
	return action;
}

int32_t TouchInputEvent::get_pointer_id() const
{
	return pointer_id;
}

std::size_t TouchInputEvent::get_touch_points() const
{
	return touch_points;
}

float TouchInputEvent::get_pos_x() const
{
	return pos_x;
}

float TouchInputEvent::get_pos_y() const
{
	return pos_y;
}
}        // namespace vkb
