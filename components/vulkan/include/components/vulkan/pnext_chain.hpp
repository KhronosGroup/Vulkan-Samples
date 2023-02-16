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

#include "components/vulkan/common/structure_type_helpers.hpp"

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

	// append a new structure to the chain
	template <typename Type>
	pNextChain &append(AppendFunc<Type> &&func)
	{
		std::unique_ptr<Type> target = std::make_unique<Type>();
		target->sType                = get_structure_type<Type>();
		func(*target.get());
		_chain.push_back(reinterpret_cast<Header *>(target.get()));
		return *this;
	}

	// get a structure from the chain
	// creates a cloned copy of the structure and resets the pNext ptr
	template <typename Type>
	Type get()
	{
		for (auto *header : _chain)
		{
			if (header->sType == get_structure_type<Type>())
			{
				if (auto *data = reinterpret_cast<Type *>(header))
				{
					Type copy  = *data;
					copy.pNext = nullptr;
					return copy;
				}
			}
		}

		// return empty default
		return Type{get_structure_type<Type>(), nullptr};
	}

	// check if a structure is in the chain
	template <typename Type>
	bool has()
	{
		for (auto *header : _chain)
		{
			if (header->sType == get_structure_type<Type>())
			{
				return true;
			}
		}

		return false;
	}

	// build the chain and correctly assemble the pNext pointers
	void *build() const
	{
		void *p_next = nullptr;

		// reverse order so that the chain is constructed in the appended order
		std::vector<Header *> chain{_chain.begin(), _chain.end()};
		std::reverse(chain.begin(), chain.end());

		for (Header *header : chain)
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