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

#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <typeindex>

namespace components
{
namespace events
{
template <typename Type>
class Channel;
template <typename Type>
using ChannelPtr = std::shared_ptr<Channel<Type>>;

template <typename Type>
class ChannelReceiver;
template <typename Type>
using ChannelReceiverPtr = std::unique_ptr<ChannelReceiver<Type>>;

template <typename Type>
class ChannelSender;
template <typename Type>
using ChannelSenderPtr = std::unique_ptr<ChannelSender<Type>>;

// Acts as a base for storing multiple channels in a container
class AbstractChannel
{
  public:
	AbstractChannel(std::type_index &&type_index) :
	    m_type_index{type_index}
	{}
	virtual ~AbstractChannel() = default;

	inline const std::type_index &type_index() const
	{
		return m_type_index;
	}

  private:
	std::type_index m_type_index;
};

// Acts as a context which links receivers and senders together
template <typename Type>
class Channel : public AbstractChannel
{
	friend ChannelReceiver<Type>;
	friend ChannelSender<Type>;

  public:
	~Channel();

	/**
	 * @brief Create a new channel for a given type
	 *
	 * @return ChannelPtr<Type> A new chanel context
	 */
	static ChannelPtr<Type> create()
	{
		return std::shared_ptr<Channel<Type>>(new Channel<Type>());
	}

	/**
	 * @brief Create a new receiver
	 *
	 * @return ChannelReceiverPtr<Type> A receiver
	 */
	ChannelReceiverPtr<Type> receiver();

	/**
	 * @brief Create a new sender
	 *
	 * @return ChannelSenderPtr<Type> A sender
	 */
	ChannelSenderPtr<Type> sender();

  protected:
	Channel();

	/**
	 * @brief push a new type to all receivers
	 *
	 * @param type the value to give to receivers
	 */
	void push(const Type &type);

	/**
	 * @brief unsubscribe a receiver
	 *
	 * @param receiver the subscribing receiver
	 */
	void unsubscribe(ChannelReceiver<Type> &receiver);

	/**
	 * @brief subscribe a receiver
	 *
	 * @param receiver the subscribing receiver
	 */
	void subscribe(ChannelReceiver<Type> &receiver);

  private:
	mutable std::mutex                m_mut;
	std::set<ChannelReceiver<Type> *> m_receivers;
};

template <typename Type>
class ChannelReceiver
{
	friend Channel<Type>;

  public:
	~ChannelReceiver();

	/**
	 * @brief Checks if there is a next item in the channel
	 */
	bool has_next() const;

	/**
	 * @brief retrieves the next item in the channel
	 *
	 * @return Type the next item
	 */
	Type next();

	/**
	 * @brief empties the channel returning the last item
	 *
	 * @return Type the last item
	 */
	Type drain();

	inline size_t size() const
	{
		std::lock_guard<std::mutex> lock{m_mut};

		return m_queue.size();
	};

  private:
	ChannelReceiver(Channel<Type> &channel);

	/**
	 * @brief private used to receive a new item from the Channel<Type> context
	 *
	 * @param item an incoming item
	 */
	void push(const Type &item);

	mutable std::mutex m_mut;
	Channel<Type> &    m_channel;
	std::queue<Type>   m_queue;
};

template <typename Type>
class ChannelSender
{
	friend Channel<Type>;

  public:
	~ChannelSender();

	/**
	 * @brief push a new item to the channel
	 *
	 * @param item the item to push
	 */
	void push(const Type &item);

  private:
	ChannelSender(Channel<Type> &channel);

	mutable std::mutex m_mut;
	Channel<Type> &    m_channel;
};
}        // namespace events
}        // namespace components

/*
 *
 * Template Implementations
 *
 */

namespace components
{
namespace events
{
template <typename Type>
Channel<Type>::Channel() :
    AbstractChannel{typeid(Type)}
{}

template <typename Type>
Channel<Type>::~Channel()
{}

template <typename Type>
ChannelReceiverPtr<Type> Channel<Type>::receiver()
{
	return std::unique_ptr<ChannelReceiver<Type>>(new ChannelReceiver<Type>(*this));
}

template <typename Type>
ChannelSenderPtr<Type> Channel<Type>::sender()
{
	return std::unique_ptr<ChannelSender<Type>>(new ChannelSender<Type>(*this));
}

template <typename Type>
void Channel<Type>::push(const Type &type)
{
	std::lock_guard<std::mutex> lock{m_mut};

	for (auto *receiver : m_receivers)
	{
		receiver->push(type);
	}
}

template <typename Type>
void Channel<Type>::unsubscribe(ChannelReceiver<Type> &receiver)
{
	std::lock_guard<std::mutex> lock{m_mut};

	const auto it = m_receivers.find(&receiver);
	if (it == m_receivers.end())
	{
		m_receivers.erase(it);
	}
}

template <typename Type>
void Channel<Type>::subscribe(ChannelReceiver<Type> &receiver)
{
	std::lock_guard<std::mutex> lock{m_mut};

	m_receivers.emplace(&receiver);
}

template <typename Type>
ChannelReceiver<Type>::ChannelReceiver(Channel<Type> &channel) :
    m_channel{channel}
{
	m_channel.subscribe(*this);
}

template <typename Type>
ChannelReceiver<Type>::~ChannelReceiver()
{
	m_channel.unsubscribe(*this);
}

template <typename Type>
bool ChannelReceiver<Type>::has_next() const
{
	std::lock_guard<std::mutex> lock{m_mut};

	return !m_queue.empty();
}

template <typename Type>
Type ChannelReceiver<Type>::next()
{
	std::lock_guard<std::mutex> lock{m_mut};

	if (m_queue.empty())
	{
		return {};
	}

	auto front = m_queue.front();
	m_queue.pop();

	return front;
}

template <typename Type>
Type ChannelReceiver<Type>::drain()
{
	std::lock_guard<std::mutex> lock{m_mut};

	if (m_queue.empty())
	{
		return {};
	}

	Type front{};
	while (!m_queue.empty())
	{
		front = m_queue.front();
		m_queue.pop();
	}

	return front;
}

template <typename Type>
void ChannelReceiver<Type>::push(const Type &item)
{
	std::lock_guard<std::mutex> lock{m_mut};

	m_queue.push(item);
}

template <typename Type>
ChannelSender<Type>::ChannelSender(Channel<Type> &channel) :
    m_channel{channel}
{
}

template <typename Type>
ChannelSender<Type>::~ChannelSender()
{}

template <typename Type>
void ChannelSender<Type>::push(const Type &item)
{
	std::lock_guard<std::mutex> lock{m_mut};

	m_channel.push(item);
}
}        // namespace events
}        // namespace components