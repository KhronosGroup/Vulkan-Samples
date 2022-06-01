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

#pragma once

#include <optional>
#include <unordered_map>

#include "components/events/channel.hpp"
#include "components/events/event_types.hpp"

namespace components
{
namespace events
{
struct CursorPosition
{
	uint32_t x;
	uint32_t y;
};

struct Touch
{
	CursorPosition position;
	CursorPosition delta;
};

class InputManager final
{
  public:
	InputManager()  = default;
	~InputManager() = default;

	/**
	 * @brief read KeyEvent channel and processes events
	 *        Channel will be depleted
	 *
	 * @param events events to process
	 */
	void process(ChannelReceiverPtr<KeyEvent> &events);

	/**
	 * @brief read CursorPositionEvent channel and processes events
	 *        Channel will be depleted
	 *
	 * @param events events to process
	 */
	void process(ChannelReceiverPtr<CursorPositionEvent> &events);

	/**
	 * @brief read TouchEvent channel and processes events
	 *        Channel will be depleted
	 *
	 * @param events events to process
	 */
	void process(ChannelReceiverPtr<TouchEvent> &events);

	/**
	 * @brief Query if a key is pressed
	 *
	 * @param key the key to query
	 */
	bool key_down(KeyCode key) const;

	/**
	 * @brief Query if a key is lifted
	 *        The lifted state is reset by flush
	 *
	 * @param key the key to query
	 */
	bool key_up(KeyCode key) const;

	/**
	 * @brief Query the cursors current known position
	 *
	 * @return CursorPosition the cursor position
	 */
	CursorPosition current_cursor_position() const;

	/**
	 * @brief Query the change in position between the current and last flush
	 *
	 * @return CursorPosition the movement delta
	 */
	CursorPosition cursor_position_delta() const;

	/**
	 * @brief Get a touch event for a given touch index
	 *
	 * @param index finger index
	 *
	 * @return Touch the state of a touch
	 */
	std::optional<Touch> touch(uint32_t index) const;

	/**
	 * @brief Reset the state of the manager
	 */
	void flush();

  private:
	KeyAction key_action(KeyCode key) const;

  private:
	std::unordered_map<KeyCode, KeyAction> m_key_state;

	CursorPosition m_last_cursor_position{};
	CursorPosition m_cursor_position_delta{};

	std::unordered_map<uint32_t, Touch> m_touch_state;
};
}        // namespace events
}        // namespace components