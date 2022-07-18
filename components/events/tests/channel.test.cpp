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

#include <catch2/catch_test_macros.hpp>

#include <vector>

#include <components/events/channel.hpp>

using namespace components::events;

TEST_CASE("send single event", "[events]")
{
	struct Event
	{
		uint32_t value;
	};

	ChannelPtr<Event> channel = Channel<Event>::create();

	auto send = channel->create_sender();

	auto rec1 = channel->create_receiver();
	auto rec2 = channel->create_receiver();

	send->push(Event{42});

	auto val1 = rec1->next();
	REQUIRE(val1.has_value());
	REQUIRE((*val1).value == 42);

	auto val2 = rec2->next();
	REQUIRE(val2.has_value());
	REQUIRE((*val2).value == 42);
}

TEST_CASE("send multiple events", "[events]")
{
	struct Event
	{
		uint32_t value;
	};

	ChannelPtr<Event> channel = Channel<Event>::create();

	auto send1 = channel->create_sender();
	auto send2 = channel->create_sender();

	auto rec1 = channel->create_receiver();

	send1->push({1});
	send2->push({2});
	send1->push({3});
	send1->push({4});
	send2->push({5});

	std::vector<uint32_t> expected_values = {
	    1,
	    2,
	    3,
	    4,
	    5,
	};

	for (auto val : expected_values)
	{
		auto received_value = rec1->next();
		REQUIRE(received_value.has_value());
		REQUIRE((*received_value).value == val);
	}
}

TEST_CASE("create receiver whilst sending events", "[events]")
{
	struct Event
	{
		uint32_t value;
	};

	ChannelPtr<Event> channel = Channel<Event>::create();

	auto send1 = channel->create_sender();

	auto rec1 = channel->create_receiver();

	send1->push({1});
	send1->push({2});
	send1->push({3});

	auto rec2 = channel->create_receiver();

	send1->push({4});
	send1->push({5});

	std::vector<uint32_t> expected_values_1 = {1, 2, 3, 4, 5};

	for (auto val : expected_values_1)
	{
		auto received_value = rec1->next();
		REQUIRE(received_value.has_value());
		REQUIRE((*received_value).value == val);
	}

	std::vector<uint32_t> expected_values_2 = {4, 5};

	for (auto val : expected_values_2)
	{
		auto received_value = rec2->next();
		REQUIRE(received_value.has_value());
		REQUIRE((*received_value).value == val);
	}
}

TEST_CASE("drain a channel", "[events]")
{
	struct Event
	{
		uint32_t value;
	};

	ChannelPtr<Event> channel = Channel<Event>::create();

	auto send1 = channel->create_sender();

	auto rec1 = channel->create_receiver();

	send1->push({1});
	send1->push({2});
	send1->push({3});
	send1->push({4});
	send1->push({5});

	auto last = rec1->drain();
	REQUIRE(last.has_value());
	REQUIRE((*last).value == 5);
}

TEST_CASE("type index", "[events]")
{
	struct Event
	{
		uint32_t value;
	};

	std::type_index type_index = std::type_index{typeid(Event)};

	ChannelPtr<Event> channel = Channel<Event>::create();

	REQUIRE(type_index == channel->type_index());
}