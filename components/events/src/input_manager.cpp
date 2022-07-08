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

#include "components/events/input_manager.hpp"

#include <cassert>

namespace components
{
namespace events
{
void InputManager::process(ChannelReceiverPtr<KeyEvent> &events)
{
	while (events->has_next())
	{
		KeyEvent event = events->next();

		m_key_state[event.code] = event.action;
	}
}

void InputManager::process(ChannelReceiverPtr<CursorPositionEvent> &events)
{
	auto next_known_position = events->drain();

	if (next_known_position.x != m_last_cursor_position.x || next_known_position.y != m_last_cursor_position.y)
	{
		m_cursor_position_delta.x = next_known_position.x - m_last_cursor_position.x;
		m_cursor_position_delta.y = next_known_position.y - m_last_cursor_position.y;
	}

	m_last_cursor_position = CursorPosition{next_known_position.x, next_known_position.y};
}

void InputManager::process(ChannelReceiverPtr<TouchEvent> &events)
{
	while (events->has_next())
	{
		auto event = events->next();

		auto it = m_touch_state.find(event.pointer_id);
		if (it == m_touch_state.end())
		{
			auto res = m_touch_state.emplace(event.pointer_id, Touch{});
			assert(res.second);
			it = res.first;

			it->second.position = CursorPosition{event.x, event.y};
		}

		auto &touch = it->second;

		if (event.x != touch.position.x || event.y != touch.position.y)
		{
			touch.position.x = event.x - touch.position.x;
			touch.position.y = event.y - touch.position.y;
		}

		touch.position = CursorPosition{event.x, event.y};
	}
}

KeyAction InputManager::key_action(KeyCode key) const
{
	auto it = m_key_state.find(key);
	if (it != m_key_state.end())
	{
		return it->second;
	}

	return KeyAction::Unknown;
}

bool InputManager::key_down(KeyCode key) const
{
	auto action = key_action(key);
	return action == KeyAction::Down || action == KeyAction::Repeat;
}

bool InputManager::key_up(KeyCode key) const
{
	return key_action(key) == KeyAction::Up;
}

CursorPosition InputManager::current_cursor_position() const
{
	return m_last_cursor_position;
}

CursorPosition InputManager::cursor_position_delta() const
{
	return m_cursor_position_delta;
}

std::optional<Touch> InputManager::touch(uint32_t index) const
{
	auto it = m_touch_state.find(index);
	if (it == m_touch_state.end())
	{
		return {};
	}
	return it->second;
}

void InputManager::flush()
{
	// reset cursor delta
	m_cursor_position_delta = {};

	// flush key state
	for (auto &state : m_key_state)
	{
		if (state.second == KeyAction::Up)
		{
			state.second = KeyAction::Unknown;
		}
	}
}
}        // namespace events
}        // namespace components
