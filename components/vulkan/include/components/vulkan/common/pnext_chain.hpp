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
/**
	 * @brief Allows for the creation of extension chains
	 */
class pNextChain
{
  public:
	pNextChain() = default;

	~pNextChain()
	{
		for (VkBaseInStructure *header : _chain)
		{
			delete header;
		}
	}

	template <typename Type>
	using AppendFunc = std::function<void(Type &)>;

	template <typename Type>
	inline pNextChain &append(AppendFunc<Type> &&func)
	{
		Type *target = new Type();

		// infer the structure type
		target->sType = get_structure_type_name<Type>();

		// forward to the caller for configuration
		func(*target);

		_chain.push_back(reinterpret_cast<VkBaseInStructure *>(target));

		return *this;
	}

	inline void *build()
	{
		void *p_next = nullptr;

		// reverse order so that the chain is constructed in the appended order
		std::reverse(_chain.begin(), _chain.end());

		for (VkBaseInStructure *header : _chain)
		{
			header->pNext = static_cast<VkBaseInStructure *>(p_next);
			p_next        = header;
		}

		return p_next;
	}

  private:
	std::vector<VkBaseInStructure *> _chain;
};
}        // namespace vulkan
}        // namespace components