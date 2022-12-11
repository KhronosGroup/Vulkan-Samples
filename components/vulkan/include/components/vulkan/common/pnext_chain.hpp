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

#include <volk.h>

#include <algorithm>
#include <any>
#include <functional>
#include <memory>
#include <vector>

#include "components/vulkan/common/struct_initializers.hpp"

namespace components
{
namespace vulkan
{
class pNextChain
{
  public:
	pNextChain()  = default;
	~pNextChain() = default;

	template <typename Type>
	using AppendFunc = std::function<void(Type &)>;

	template <typename Type>
	inline pNextChain &append(AppendFunc<Type> &&func)
	{
		std::unique_ptr<Type> target = std::make_unique<Type>(init_default<Type>());
		func(target);
		_chain.push_back(reinterpret_cast<Header *>(target.get()));
	}

	inline void *build() const
	{
		void *p_next = nullptr;

		// reverse order so that the chain is constructed in the appended order
		std::reverse(_chain.begin(), _chain.end());

		for (Header *header : _chain)
		{
			header->pNext = p_next;
			p_next        = header;
		}

		return p_next;
	}

  private:
	struct Header
	{
		VkStructureType sType{VK_STRUCTURE_TYPE_MAX_ENUM};
		const void     *pNext{nullptr};
	};

	std::vector<std::unique_ptr<std::any>> _memory;
	std::vector<Header *>                  _chain;
};
}        // namespace vulkan
}        // namespace components