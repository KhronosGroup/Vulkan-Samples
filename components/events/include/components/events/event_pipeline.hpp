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

#include <functional>

#include "components/events/event_bus.hpp"

namespace components
{
namespace events
{
class EventPipelineStage
{
  public:
	EventPipelineStage()          = default;
	virtual ~EventPipelineStage() = default;

	virtual const char *name() const        = 0;
	virtual void        emit(EventBus &bus) = 0;
};

template <typename Event>
class TypedEventPipelineStage : public EventPipelineStage
{
  public:
	TypedEventPipelineStage()
	{}

	virtual ~TypedEventPipelineStage() = default;

	virtual const char *name() const override
	{
		return typeid(Event).name();
	}

	virtual void emit(EventBus &bus) override
	{
		auto sender = bus.request_sender<Event>();
		sender->push(Event{});
	}
};

template <typename Event>
class TypedEventPipelineStageWithFunc : public TypedEventPipelineStage<Event>
{
  public:
	typedef Event (*Func)();

	TypedEventPipelineStageWithFunc(Func &&func) :
	    m_func{func}
	{}

	virtual ~TypedEventPipelineStageWithFunc() = default;

	virtual void emit(EventBus &bus) override
	{
		auto sender = bus.request_sender<Event>();
		sender->push(m_func());
	}

  private:
	Func m_func;
};

class EventPipeline : public EventBus
{
  public:
	EventPipeline()          = default;
	virtual ~EventPipeline() = default;

	EventPipeline &once(std::unique_ptr<EventPipelineStage> &&stage);

	EventPipeline &then(std::unique_ptr<EventPipelineStage> &&stage);

	virtual void process() override;

  protected:
	bool running{false};

	std::vector<std::unique_ptr<EventPipelineStage>> m_once_stages;
	std::vector<std::unique_ptr<EventPipelineStage>> m_stages;
};
}        // namespace events
}        // namespace components