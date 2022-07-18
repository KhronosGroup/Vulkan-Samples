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

#include <components/events/event_bus.hpp>

using namespace components::events;

class TestEventBus final : public EventBus
{
  public:
	TestEventBus() :
	    EventBus{}
	{}

	virtual ~TestEventBus() = default;

	inline size_t observer_count() const
	{
		return m_observers.size();
	}

	inline size_t unobserved_each_event_count() const
	{
		size_t unprocessed_events{0};

		for (auto &it : m_each_callbacks)
		{
			unprocessed_events += it.second->queue_size();
		}

		return unprocessed_events;
	}

	inline size_t unobserved_last_event_count() const
	{
		size_t unprocessed_events{0};

		for (auto &it : m_last_callbacks)
		{
			unprocessed_events += it.second->queue_size();
		}

		return unprocessed_events;
	}

	inline size_t each_callback_count() const
	{
		size_t callback_count{0};

		for (auto &it : m_each_callbacks)
		{
			callback_count += it.second->callback_count();
		}

		return callback_count;
	}

	inline size_t last_callback_count() const
	{
		size_t callback_count{0};

		for (auto &it : m_last_callbacks)
		{
			callback_count += it.second->callback_count();
		}

		return callback_count;
	}
};

class Observer final : public EventObserver
{
  public:
	Observer(std::function<void()> update = nullptr, std::function<void(EventBus &)> attach = nullptr) :
	    EventObserver{},
	    m_update{update},
	    m_attach{attach}
	{
	}

	virtual ~Observer() = default;

	virtual void update() override
	{
		if (m_update)
		{
			m_update();
		}
	}

	virtual void attach(EventBus &bus) override
	{
		if (m_attach)
		{
			m_attach(bus);
		}
	}

  private:
	std::function<void()>           m_update{nullptr};
	std::function<void(EventBus &)> m_attach{nullptr};
};

TEST_CASE("register observer", "[events]")
{
	TestEventBus bus{};

	std::shared_ptr<EventObserver> observer = std::make_shared<Observer>();

	REQUIRE(bus.observer_count() == 0);

	bus.attach(observer);

	REQUIRE(bus.observer_count() == 1);
}

TEST_CASE("register multiple observers of the different instances", "[events]")
{
	TestEventBus bus{};

	std::shared_ptr<EventObserver> observer_1 = std::make_shared<Observer>();
	std::shared_ptr<EventObserver> observer_2 = std::make_shared<Observer>();
	std::shared_ptr<EventObserver> observer_3 = std::make_shared<Observer>();

	REQUIRE(bus.observer_count() == 0);

	bus
	    .attach(observer_1)
	    .attach(observer_2)
	    .attach(observer_3);

	REQUIRE(bus.observer_count() == 3);
}

TEST_CASE("request sender", "[events]")
{
	struct EventType
	{
		uint32_t value;
	};

	TestEventBus bus{};

	auto sender = bus.request_sender<EventType>();
	REQUIRE(sender != nullptr);
}

TEST_CASE("event bus for each event", "[events]")
{
	struct EventType
	{
		uint32_t value;
	};

	TestEventBus bus{};

	auto sender = bus.request_sender<EventType>();
	REQUIRE(sender != nullptr);

	bus.each<EventType>([&](const EventType &event) {
		REQUIRE(event.value == 12);
	});

	REQUIRE(bus.each_callback_count() == 1);

	sender->push(EventType{12});

	REQUIRE(bus.unobserved_each_event_count() == 1);

	bus.process();

	REQUIRE(bus.unobserved_each_event_count() == 0);
}

TEST_CASE("event bus for each event with multiple callbacks", "[events]")
{
	struct EventType
	{
		uint32_t value;
	};

	TestEventBus bus{};

	auto sender = bus.request_sender<EventType>();
	REQUIRE(sender != nullptr);

	bool first = true;        // < tick tock between event callbacks, this technique should be avoided in practice

	// ! do not change the order here
	bus.each<EventType>([&](const EventType &event) {
		if (!first)        // < will skip first event
		{
			REQUIRE(event.value == 15);
		}
	});

	bus.each<EventType>([&](const EventType &event) {
		if (first)        // < will process the first event and skip the second
		{
			first = false;
			REQUIRE(event.value == 12);
		}
	});

	REQUIRE(bus.each_callback_count() == 2);

	sender->push(EventType{12});
	sender->push(EventType{15});

	REQUIRE(bus.unobserved_each_event_count() == 2);

	bus.process();

	REQUIRE(bus.unobserved_each_event_count() == 0);
}

TEST_CASE("event bus for last event", "[events]")
{
	struct EventType
	{
		uint32_t value;
	};

	TestEventBus bus{};

	auto sender = bus.request_sender<EventType>();
	REQUIRE(sender != nullptr);

	bus.last<EventType>([&](const EventType &event) {
		REQUIRE(event.value == 4);
	});

	REQUIRE(bus.last_callback_count() == 1);

	sender->push(EventType{1});
	sender->push(EventType{2});
	sender->push(EventType{3});
	sender->push(EventType{4});

	REQUIRE(bus.unobserved_last_event_count() == 4);

	bus.process();

	REQUIRE(bus.unobserved_last_event_count() == 0);
}

TEST_CASE("process observer", "[events]")
{
	struct EventType
	{
		// true if sent from the observer
		bool     internal;
		uint32_t value;
	};

	TestEventBus bus{};

	auto sender = bus.request_sender<EventType>();
	REQUIRE(sender != nullptr);

	auto observer = std::make_shared<Observer>(
	    [&]() {        // update()
		    sender->push(EventType{true, 5});
	    },
	    [&](EventBus &eb) {        // attach(bus)
		    eb.each<EventType>([](const EventType &event) {
			    if (event.internal)
			    {
				    REQUIRE(event.value == 5);
			    }
			    else
			    {
				    REQUIRE(event.value == 1);
			    }
		    });

		    eb.last<EventType>([](const EventType &event) {
			    // last event is pushed by the observer
			    REQUIRE(event.internal == true);
			    REQUIRE(event.value == 5);
		    });
	    });

	bus.attach(observer);

	REQUIRE(bus.last_callback_count() == 1);
	REQUIRE(bus.each_callback_count() == 1);

	// will always be the first events added
	sender->push(EventType{false, 1});
	sender->push(EventType{false, 1});
	sender->push(EventType{false, 1});
	sender->push(EventType{false, 1});

	REQUIRE(bus.unobserved_last_event_count() == 4);

	// adds events from observer update
	bus.process();

	REQUIRE(bus.unobserved_last_event_count() == 0);
}

TEST_CASE("expire an observer before process", "[events]")
{
	struct EventType
	{
		uint32_t value;
	};

	TestEventBus bus{};

	std::shared_ptr<EventObserver> observer_1 = std::make_shared<Observer>();
	std::shared_ptr<EventObserver> observer_2 = std::make_shared<Observer>();

	bus.attach(observer_1).attach(observer_2);

	REQUIRE(bus.observer_count() == 2);

	observer_1.reset();

	bus.process();

	REQUIRE(bus.observer_count() == 1);

	observer_2.reset();

	bus.process();

	REQUIRE(bus.observer_count() == 0);
}