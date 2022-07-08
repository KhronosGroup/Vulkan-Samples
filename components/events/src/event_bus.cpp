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

#include "components/events/event_bus.hpp"

#include <cassert>

namespace components
{
namespace events
{
EventBus &EventBus::attach(std::weak_ptr<EventObserver> &&observer)
{
	assert((std::find_if(m_observers.begin(), m_observers.end(), [this, observer](const auto &obs) { return same_ptr(obs, observer); }) == m_observers.end()) && "attempting to attach an existing observer");

	if (auto shared_observer = observer.lock())
	{
		shared_observer->attach(*this);
		m_observers.push_back(shared_observer);
	}

	return *this;
}

void EventBus::process()
{
	for (auto it = m_observers.begin(); it != m_observers.end();)
	{
		if (it->expired())
		{
			it = m_observers.erase(it);
			continue;
		}

		if (auto shared = it->lock())
		{
			shared->update();
		}

		++it;
	}

	flush_callbacks();
}

EventObserverGroup &EventObserverGroup::attach(std::shared_ptr<EventObserver> &observer)
{
	m_observers.emplace(observer);
	return *this;
}

EventObserverGroup &EventObserverGroup::remove(std::shared_ptr<EventObserver> &observer)
{
	auto it = m_observers.find(observer);
	if (it != m_observers.end())
	{
		m_observers.erase(it);
	}
	return *this;
}

void EventObserverGroup::update()
{
	for (auto &it : m_observers)
	{
		it->update();
	}
}

void EventObserverGroup::attach(EventBus &bus)
{
	for (auto &it : m_observers)
	{
		it->attach(bus);
	}
}
}        // namespace events
}        // namespace components