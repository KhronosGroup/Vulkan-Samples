/*
* Copyright 2024 NVIDIA Corporation.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef _VKCODECUTILS_VKTHREADSAFEQUEUE_H_
#define _VKCODECUTILS_VKTHREADSAFEQUEUE_H_

#include <queue>
#include <atomic>
#include <condition_variable>

template <typename QueueNodeType>
class VkThreadSafeQueue {
public:
    VkThreadSafeQueue(uint32_t maxPendingQueueNodes = 4)
     : m_maxPendingQueueNodes(maxPendingQueueNodes),
       m_queueIsFlushing(0) {}

    bool SetMaxPendingQueueNodes(uint32_t maxPendingQueueNodes = 16) {
        m_maxPendingQueueNodes = maxPendingQueueNodes;
        return true;
    }

    bool Push(QueueNodeType& node) {
        std::unique_lock<std::mutex> lock(m_mutex);

        if (m_queueIsFlushing) {
            return false;
        }

        // Wait for the consumer to consume the previous node item(s)
        m_condProducer.wait(lock, [this]{ return (!m_queueIsFlushing && (m_queue.size() < m_maxPendingQueueNodes)); });

        m_queue.push(node);
        m_condConsumer.notify_one();

        return true;
    }

    bool TryPop(QueueNodeType& node) {
        std::unique_lock<std::mutex> lock(m_mutex);
        return TryPopNoLock(node);
    }

    bool WaitAndPop(QueueNodeType& node) {
        std::unique_lock<std::mutex> lock(m_mutex);

        if (m_queueIsFlushing) {
            return TryPopNoLock(node);
        }

        m_condConsumer.wait(lock, [this]{ return ((m_queueIsFlushing == false) && !m_queue.empty()); });
        node = m_queue.front();
        m_queue.pop();
        // Notify the producer
        m_condProducer.notify_one();

        return true;
    }

    bool Empty() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    bool Size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

    void SetFlushAndExit()
    {
        std::unique_lock<std::mutex> lock(m_mutex);

        m_queueIsFlushing = true;

        m_condProducer.notify_one();
        m_condConsumer.notify_one();
    }

    bool ExitQueue() {
        std::unique_lock<std::mutex> lock(m_mutex);
        return ((m_queueIsFlushing == true) && m_queue.empty());
    }

private:
    bool TryPopNoLock(QueueNodeType& node) {

        if (m_queue.empty()) {
            return false;
        }
        node = m_queue.front();
        m_queue.pop();
        return true;
    }

private:
    mutable std::mutex        m_mutex;
    uint32_t                  m_maxPendingQueueNodes;
    uint32_t                  m_queueIsFlushing : 1;
    std::condition_variable   m_condProducer;
    std::condition_variable   m_condConsumer;
    std::queue<QueueNodeType> m_queue;
};

#endif /* _VKCODECUTILS_VKTHREADSAFEQUEUE_H_ */
