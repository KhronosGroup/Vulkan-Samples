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

#include <memory>
#include <vector>

#include <components/events/pipelines.hpp>

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

class TestObserver final : public EventObserver
{
  public:
	TestObserver(std::function<void()> update = nullptr, std::function<void(EventBus &)> attach = nullptr) :
	    EventObserver{},
	    m_update{update},
	    m_attach{attach}
	{
	}

	virtual ~TestObserver() = default;

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

struct TestEvent
{};

// tracks the order of pipeline execution
class TestStage : public TypedPipelineStage<TestEvent>
{
  public:
	TestStage(uint32_t &current_value, uint32_t expected_value) :
	    m_current_value{current_value}, m_expected_value{expected_value}
	{}

	virtual ~TestStage() = default;

	virtual void emit(EventBus &bus) override
	{
		auto sender = bus.request_sender<TestEvent>();
		sender->push(TestEvent{});

		REQUIRE(m_current_value == m_expected_value);

		m_current_value++;
	}

  protected:
	uint32_t &m_current_value;
	uint32_t  m_expected_value;
};

class TestPipeline final : public Pipeline
{
  public:
	TestPipeline()          = default;
	virtual ~TestPipeline() = default;

	inline size_t once_count() const
	{
		return m_once_stages.size();
	}

	inline size_t then_count() const
	{
		return m_stages.size();
	}

	inline size_t total_stage_count() const
	{
		return once_count() + then_count();
	}
};

TEST_CASE("register once stage", "[events]")
{
	TestPipeline pipeline{};

	REQUIRE(pipeline.once_count() == 0);

	pipeline.once(std::make_unique<TypedPipelineStage<TestEvent>>());

	REQUIRE(pipeline.once_count() == 1);
}

TEST_CASE("register then stage", "[events]")
{
	TestPipeline pipeline{};

	REQUIRE(pipeline.then_count() == 0);

	pipeline.then(std::make_unique<TypedPipelineStage<TestEvent>>());

	REQUIRE(pipeline.then_count() == 1);
}

TEST_CASE("register multiple stages", "[events]")
{
	TestPipeline pipeline{};

	REQUIRE(pipeline.then_count() == 0);
	REQUIRE(pipeline.once_count() == 0);

	pipeline
	    .once(std::make_unique<TypedPipelineStage<TestEvent>>())
	    .once(std::make_unique<TypedPipelineStage<TestEvent>>())
	    .then(std::make_unique<TypedPipelineStage<TestEvent>>())
	    .then(std::make_unique<TypedPipelineStage<TestEvent>>())
	    .then(std::make_unique<TypedPipelineStage<TestEvent>>());

	REQUIRE(pipeline.then_count() == 3);
	REQUIRE(pipeline.once_count() == 2);
	REQUIRE(pipeline.total_stage_count() == 5);
}

TEST_CASE("stages are executed in the correct order", "[events]")
{
	TestPipeline pipeline{};

	uint32_t execution_index{0};

	pipeline
	    .once(std::make_unique<TestStage>(execution_index, 0))
	    .once(std::make_unique<TestStage>(execution_index, 1))
	    .then(std::make_unique<TestStage>(execution_index, 2))
	    .then(std::make_unique<TestStage>(execution_index, 3))
	    .then(std::make_unique<TestStage>(execution_index, 4));

	// assertions inside TestStage
	pipeline.process();

	// on a second pass we expect only the "then" stages to be processed
	execution_index = 2;

	pipeline.process();
}

TEST_CASE("event observers are executed in the correct order", "[events]")
{
	struct EventOne
	{
		uint32_t value{4};
	};

	struct EventTwo
	{
		uint32_t value{1};
	};

	struct EventThree
	{
		uint32_t value{56};
	};

	TestPipeline pipeline{};

	pipeline
	    .once(std::make_unique<TypedPipelineStage<EventOne>>())
	    .then(std::make_unique<TypedPipelineStage<EventTwo>>())
	    .then(std::make_unique<TypedPipelineStage<EventThree>>());

	uint32_t one_execution_count{0};
	uint32_t two_execution_count{0};
	uint32_t three_execution_count{0};

	pipeline
	    .each<EventOne>([&](const EventOne &event) {
		    REQUIRE(event.value == 4);
		    one_execution_count++;
	    })
	    .each<EventTwo>([&](const EventTwo &event) {
		    REQUIRE(event.value == 1);
		    two_execution_count++;
	    })
	    .each<EventThree>([&](const EventThree &event) {
		    REQUIRE(event.value == 56);
		    three_execution_count++;
	    });

	pipeline.process();
	pipeline.process();
	pipeline.process();
	pipeline.process();

	REQUIRE(one_execution_count == 1);
	REQUIRE(two_execution_count == 4);
	REQUIRE(three_execution_count == 4);
}