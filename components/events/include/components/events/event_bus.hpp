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

#include <cassert>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include <components/events/channel.hpp>

namespace components
{
namespace events
{
class EventBus;

class EventObserver
{
  public:
	EventObserver()          = default;
	virtual ~EventObserver() = default;

	virtual void update()              = 0;
	virtual void attach(EventBus &bus) = 0;
};

/**
 * @brief EventBus acts as a collection of event channels and observers
 * 
 * 		  An observer is added to the event bus through attach(observer). Once attached, an observer can register event listeners (each<EventType>(), last<EventType>()) and request
 * 		  ChannelSender<EventTypes>. Each step of the EventBus calls update() on its observers. Which in turn allows an observer to submit events to the bus. After this, the bus then processes
 * 		  all event callbacks with a stream of events.
 * 
 * 		  The combination of these actions will allow for inter component communication without any hard links. This allows samples to create and organize components in anyway they deem fit.
 *
 * 		  TODO: Allow an observer to detach from the bus. This should also clear all its callbacks. May need to restructure the internal storage of the bus
 */
class EventBus
{
  public:
	EventBus()          = default;
	virtual ~EventBus() = default;

	/**
	 * @brief Attach a new observer
	 * 
	 * @param observer the observer to attach
	 * @return EventBus& the event bus
	 */
	EventBus &attach(std::weak_ptr<EventObserver> &&observer);

	template <typename Type>
	using EventCallback = std::function<void(const Type &)>;

	/**
	 * @brief Attach an event callback for each event in a cycle
	 * 
	 * @param cb the callback
	 * @return EventBus& the event bus
	 */
	template <typename Type>
	EventBus &each(EventCallback<Type> cb);

	/**
	 * @brief Attach an event callback for the last event in a cycle
	 * 
	 * @param cb the callback
	 * @return EventBus& the event bus
	 */
	template <typename Type>
	EventBus &last(EventCallback<Type> cb);

	/**
	 * @brief Retrieve a ChannelSender for a given type
	 * 
	 * @tparam Type the type of the required sender
	 * @return ChannelSenderPtr<Type> the requested sender
	 */
	template <typename Type>
	ChannelSenderPtr<Type> request_sender();

	/**
	 * @brief Process a cycle of events
	 * 
	 */
	void process();

  private:
	class ChannelCallbacks
	{
	  public:
		ChannelCallbacks()          = default;
		virtual ~ChannelCallbacks() = default;

		virtual void   process_each()         = 0;
		virtual void   process_last()         = 0;
		virtual size_t queue_size() const     = 0;
		virtual size_t callback_count() const = 0;
	};

	template <typename Type>
	class TypedChannelCallbacks final : public ChannelCallbacks
	{
	  public:
		TypedChannelCallbacks(ChannelReceiverPtr<Type> &&receiver) :
		    m_receiver{std::move(receiver)}
		{}

		virtual ~TypedChannelCallbacks() = default;

		virtual void process_each() override;
		virtual void process_last() override;

#ifdef VKB_BUILD_TESTS
		virtual size_t queue_size() const override;
		virtual size_t callback_count() const override;
#endif

		void append(EventCallback<Type> func);

	  private:
		ChannelReceiverPtr<Type>         m_receiver;
		std::vector<EventCallback<Type>> m_callbacks;
	};

	template <typename Type>
	inline Channel<Type> *find_or_create_channel()
	{
		auto it = m_channels.find(typeid(Type));

		if (it == m_channels.end())
		{
			auto result = m_channels.emplace(typeid(Type), Channel<Type>::create());
			assert(result.second);
			it = result.first;
		}

		auto *casted_channel = dynamic_cast<Channel<Type> *>(it->second.get());
		assert(casted_channel != nullptr);

		return casted_channel;
	}

	template <typename Type>
	inline TypedChannelCallbacks<Type> *find_or_create_callbacks(std::unordered_map<std::type_index, std::unique_ptr<ChannelCallbacks>> &container)
	{
		auto it = container.find(typeid(Type));

		if (it == container.end())
		{
			auto *channel = find_or_create_channel<Type>();

			auto result = container.emplace(typeid(Type), std::make_unique<TypedChannelCallbacks<Type>>(channel->create_receiver()));
			assert(result.second);
			it = result.first;
		}

		auto *callbacks = dynamic_cast<TypedChannelCallbacks<Type> *>(it->second.get());
		assert(callbacks != nullptr);
		return callbacks;
	}

	inline bool same_ptr(const std::weak_ptr<EventObserver> &first, const std::weak_ptr<EventObserver> &second)
	{
		return !first.owner_before(second) && !second.owner_before(first);
	}

  protected:
	std::vector<std::weak_ptr<EventObserver>> m_observers;

	// TODO: this should be a LockedMap
	std::unordered_map<std::type_index, std::shared_ptr<AbstractChannel>>  m_channels;
	std::unordered_map<std::type_index, std::unique_ptr<ChannelCallbacks>> m_each_callbacks;
	std::unordered_map<std::type_index, std::unique_ptr<ChannelCallbacks>> m_last_callbacks;
};
}        // namespace events
}        // namespace components

/*

Implementations

*/
namespace components
{
namespace events
{
template <typename Type>
EventBus &EventBus::each(EventCallback<Type> cb)
{
	auto *callbacks = find_or_create_callbacks<Type>(m_each_callbacks);

	callbacks->append(cb);

	return *this;
}

template <typename Type>
EventBus &EventBus::last(EventCallback<Type> cb)
{
	auto *callbacks = find_or_create_callbacks<Type>(m_last_callbacks);

	callbacks->append(cb);

	return *this;
}

template <typename Type>
ChannelSenderPtr<Type> EventBus::request_sender()
{
	auto *channel = find_or_create_channel<Type>();
	return channel->create_sender();
}

template <typename Type>
void EventBus::TypedChannelCallbacks<Type>::process_each()
{
	while (auto optional_element = m_receiver->next())
	{
		for (auto func : m_callbacks)
		{
			func(*optional_element);
		}
	}
}

template <typename Type>
void EventBus::TypedChannelCallbacks<Type>::process_last()
{
	auto optional_last_element = m_receiver->drain();

	if (!optional_last_element)
	{
		return;
	}

	for (auto func : m_callbacks)
	{
		func(*optional_last_element);
	}
}

#ifdef VKB_BUILD_TESTS
template <typename Type>
size_t EventBus::TypedChannelCallbacks<Type>::queue_size() const
{
	return m_receiver->size();
}

template <typename Type>
size_t EventBus::TypedChannelCallbacks<Type>::callback_count() const
{
	return m_callbacks.size();
}
#endif

template <typename Type>
void EventBus::TypedChannelCallbacks<Type>::append(EventCallback<Type> func)
{
	m_callbacks.push_back(std::move(func));
}
}        // namespace events
}        // namespace components