/* Copyright (c) 2020-2021, Arm Limited and Contributors
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

#include <algorithm>
#include <vector>

/**
 * @brief Used to represent a tag
 */
typedef void (*TagID)();

/**
 * @brief Tag acts as a unique identifier to categories objects
 * 
 * Tags are uniquely defined using different type names. The easiest way of creating a new tag is to use an empty struct
 * struct TagName{};
 * struct DifferentTag{};
 * Tag<TagName>::ID == Tag<TagName>::member != Tag<DifferentTag>:ID
 * 
 * @tparam TAGS A set of tags
 */
template <typename... TAGS>
class Tag
{
  public:
	Tag()
	{
		tags = {Tag<TAGS>::ID...};
	}

	static void member(){};

	/**
	 * @brief Unique TagID for a given Tag<TagName>
	 */
	constexpr static TagID ID = &member;

	static bool has_tag(TagID id)
	{
		return std::find(tags.begin(), tags.end(), id) != tags.end();
	}

	template <typename C>
	static bool has_tag()
	{
		return has_tag(Tag<C>::ID);
	}

	template <typename... C>
	static bool has_tags()
	{
		std::vector<TagID> query = {Tag<C>::ID...};
		bool               res   = true;
		for (auto id : query)
		{
			res &= has_tag(id);
		}
		return res;
	}

  private:
	static std::vector<TagID> tags;
};

#ifndef DOXYGEN_SKIP

template <typename... TAGS>
std::vector<TagID> Tag<TAGS...>::tags;

#endif