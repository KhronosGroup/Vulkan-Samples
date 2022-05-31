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

template <typename Type>
class Channel
{
	friend ChannelReceiver<Type>;
	friend ChannelSender<Type>;

  public:
	~Channel();

	static ChannelPtr<Type> shared()
	{
		return std::shared_ptr<Channel<Type>>(new Channel<Type>());
	}

	ChannelReceiverPtr<Type> receiver();

	ChannelSenderPtr<Type> sender();

  protected:
	Channel();

	void put(const Type &type);

	void unsubscribe(ChannelReceiver<Type> &receiver);

	void subscribe(ChannelReceiver<Type> &receiver);

  private:
	mutable std::mutex                m_mut;
	std::set<ChannelReceiver<Type> *> m_receivers;
};        // namespace components

template <typename Type>
class ChannelReceiver
{
	friend Channel<Type>;

  public:
	~ChannelReceiver();

	bool has_next() const;

	Type next();

	Type last();

  private:
	ChannelReceiver(Channel<Type> &channel);

	void put(const Type &item);

	mutable std::mutex m_mut;
	Channel<Type>     &m_channel;
	std::queue<Type>   m_queue;
};

template <typename Type>
class ChannelSender
{
	friend Channel<Type>;

  public:
	~ChannelSender();

	void put(const Type &item);

  private:
	ChannelSender(Channel<Type> &channel);

	mutable std::mutex m_mut;
	Channel<Type>     &m_channel;
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
Channel<Type>::Channel()
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
void Channel<Type>::put(const Type &type)
{
	std::lock_guard<std::mutex> lock{m_mut};

	for (auto *receiver : m_receivers)
	{
		receiver->put(type);
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
Type ChannelReceiver<Type>::last()
{
	std::lock_guard<std::mutex> lock{m_mut};

	if (m_queue.empty())
	{
		return {};
	}

	Type front;
	while (!m_queue.empty())
	{
		front = m_queue.front();
		m_queue.pop();
	}

	return front;
}

template <typename Type>
void ChannelReceiver<Type>::put(const Type &item)
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
void ChannelSender<Type>::put(const Type &item)
{
	std::lock_guard<std::mutex> lock{m_mut};

	m_channel.put(item);
}
}        // namespace events
}        // namespace components