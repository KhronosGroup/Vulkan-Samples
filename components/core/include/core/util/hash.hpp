/* Copyright (c) 2023, Thomas Atkinson
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

#include <functional>

namespace vkb
{
inline void hash_combine(size_t &seed, size_t hash)
{
	hash += 0x9e3779b9 + (seed << 6) + (seed >> 2);
	seed ^= hash;
}

/**
 * @brief Helper function to combine a given hash
 *        with a generated hash for the input param.
 */
template <class T>
inline void hash_combine(size_t &seed, const T &v)
{
	std::hash<T> hasher;

	hash_combine(seed, hasher(v));
}

class HashBuilder
{
  public:
	HashBuilder(size_t seed = 0) :
	    seed{seed}
	{}

	virtual ~HashBuilder() = default;

	template <class T>
	HashBuilder &with(const T &v)
	{
		hash_combine(seed, v);
		return *this;
	}

	size_t build() const
	{
		return seed;
	}

  private:
	size_t seed{0};
};
}        // namespace vkb