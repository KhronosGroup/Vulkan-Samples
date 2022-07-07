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

#include <string.h>

#include <deque>
#include <exception>
#include <memory>
#include <sstream>
#include <string>

namespace components
{
class StackError;
using StackErrorPtr = std::unique_ptr<StackError>;

class StackError : std::exception
{
  public:
	StackError()  = default;
	~StackError() = default;

	StackError(const std::string &reason, const char *file = nullptr, int line = 0)
	{
		push(reason, file, line);
	}

	static StackErrorPtr combine(StackErrorPtr &&first, StackErrorPtr &&second)
	{
		auto combined = std::make_unique<StackError>();

		for (auto &err : first->m_stack)
		{
			combined->push(err);
		}

		for (auto &err : second->m_stack)
		{
			combined->push(err);
		}

		return combined;
	}

	void push(const std::string &reason, const char *file = nullptr, int line = 0)
	{
		std::stringstream stream;

		if (file && strlen(file) > 0)
		{
			stream << "[" << file << ":" << line << "] ";
		}

		stream << reason.c_str();

		m_stack.push_back(stream.str());
	}

	inline size_t size() const
	{
		return m_stack.size();
	}

	/**
	 * @brief Returns the Vulkan error code as string
	 * @return String message of exception
	 */
	const char *what() const noexcept override
	{
		if (m_stack.empty())
		{
			return nullptr;
		}

		std::deque<std::string> queue{m_stack};

		m_final = "";

		while (!queue.empty())
		{
			auto first = queue.front();

			m_final += first;
			m_final += "\n";

			queue.pop_front();
		}

		return m_final.c_str();
	}

	static std::unique_ptr<StackError> unique(const std::string &reason, const char *file = nullptr, int line = 0)
	{
		return std::make_unique<StackError>(reason, file, line);
	}

  private:
	StackError &operator+=(const StackError &b)
	{
		std::deque<std::string> queue{b.m_stack};

		while (!queue.empty())
		{
			m_stack.push_front(queue.back());
			queue.pop_back();
		}

		return *this;
	}

	std::deque<std::string> m_stack;
	mutable std::string     m_final;
};

}        // namespace components