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

#include <components/common/error.hpp>

VKBP_DISABLE_WARNINGS()
#include <catch2/catch_test_macros.hpp>
VKBP_ENABLE_WARNINGS()

#include <vector>

#include <components/events/input_manager.hpp>

using namespace components::events;

TEST_CASE("key_down", "[events]")
{
	auto channel  = Channel<KeyEvent>::create();
	auto receiver = channel->create_receiver();

	auto sender = channel->create_sender();
	sender->push(KeyEvent{KeyCode::A, KeyAction::Down});

	InputManager manager;
	manager.process(receiver);

	REQUIRE(manager.key_down(KeyCode::A));
	REQUIRE_FALSE(manager.key_down(KeyCode::B));
	REQUIRE_FALSE(manager.key_down(KeyCode::C));

	sender->push(KeyEvent{KeyCode::A, KeyAction::Repeat});
	manager.process(receiver);
	REQUIRE(manager.key_down(KeyCode::A));
	REQUIRE_FALSE(manager.key_down(KeyCode::B));
	REQUIRE_FALSE(manager.key_down(KeyCode::C));
}

TEST_CASE("key_down multiple events", "[events]")
{
	auto channel  = Channel<KeyEvent>::create();
	auto receiver = channel->create_receiver();

	auto sender = channel->create_sender();
	sender->push(KeyEvent{KeyCode::A, KeyAction::Down});
	sender->push(KeyEvent{KeyCode::A, KeyAction::Up});
	sender->push(KeyEvent{KeyCode::A, KeyAction::Repeat});
	sender->push(KeyEvent{KeyCode::A, KeyAction::Repeat});
	sender->push(KeyEvent{KeyCode::A, KeyAction::Down});

	InputManager manager;
	manager.process(receiver);

	REQUIRE(manager.key_down(KeyCode::A));
	REQUIRE_FALSE(manager.key_down(KeyCode::B));
	REQUIRE_FALSE(manager.key_down(KeyCode::C));

	sender->push(KeyEvent{KeyCode::A, KeyAction::Repeat});
	manager.process(receiver);
	REQUIRE(manager.key_down(KeyCode::A));
	REQUIRE_FALSE(manager.key_down(KeyCode::B));
	REQUIRE_FALSE(manager.key_down(KeyCode::C));
}

TEST_CASE("key_up", "[events]")
{
	auto channel  = Channel<KeyEvent>::create();
	auto receiver = channel->create_receiver();

	auto sender = channel->create_sender();
	sender->push(KeyEvent{KeyCode::A, KeyAction::Up});

	InputManager manager;
	manager.process(receiver);

	REQUIRE(manager.key_up(KeyCode::A));
	REQUIRE_FALSE(manager.key_up(KeyCode::B));
	REQUIRE_FALSE(manager.key_up(KeyCode::C));
}

TEST_CASE("current_cursor_position", "[events]")
{
	auto channel  = Channel<CursorPositionEvent>::create();
	auto receiver = channel->create_receiver();

	auto sender = channel->create_sender();
	sender->push(CursorPositionEvent{10, 25});

	InputManager manager;
	manager.process(receiver);

	auto pos = manager.current_cursor_position();
	REQUIRE(pos.x == 10);
	REQUIRE(pos.y == 25);
}

TEST_CASE("cursor_position_movement default", "[events]")
{
	InputManager manager;
	auto         drift = manager.cursor_position_delta();
	REQUIRE(drift.x == 0);
	REQUIRE(drift.y == 0);
}

TEST_CASE("cursor_position_movement", "[events]")
{
	auto channel  = Channel<CursorPositionEvent>::create();
	auto receiver = channel->create_receiver();

	auto sender = channel->create_sender();
	sender->push(CursorPositionEvent{10, 25});

	InputManager manager;
	manager.process(receiver);

	sender->push(CursorPositionEvent{25, 10});
	manager.process(receiver);

	auto drift = manager.cursor_position_delta();
	REQUIRE(drift.x == 15);
	REQUIRE(drift.y == -15);
}

TEST_CASE("cursor_position_movement after flush", "[events]")
{
	auto channel  = Channel<CursorPositionEvent>::create();
	auto receiver = channel->create_receiver();

	auto sender = channel->create_sender();
	sender->push(CursorPositionEvent{10, 25});

	InputManager manager;
	manager.process(receiver);
	manager.flush();

	sender->push(CursorPositionEvent{25, 10});
	manager.process(receiver);

	auto drift = manager.cursor_position_delta();
	REQUIRE(drift.x == 15);
	REQUIRE(drift.y == -15);
}

TEST_CASE("get touch state", "[events]")
{
	auto channel  = Channel<TouchEvent>::create();
	auto receiver = channel->create_receiver();

	auto sender = channel->create_sender();
	sender->push(TouchEvent{TouchAction::PointerDown, 0, 10, 10});

	InputManager manager;
	manager.process(receiver);

	REQUIRE(manager.touch(0).has_value());
	REQUIRE_FALSE(manager.touch(1).has_value());

	auto touch = manager.touch(0);
	REQUIRE(touch->position.x == 10);
	REQUIRE(touch->position.y == 10);
	REQUIRE(touch->delta.x == 0);
	REQUIRE(touch->delta.y == 0);
}