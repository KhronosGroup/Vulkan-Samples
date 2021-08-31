/* Copyright (c) 2018-2020, Arm Limited and Contributors
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
#include <array>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "common/error.h"
#include "common/glm.h"

namespace vkb
{
template <typename T>
inline void read(std::istringstream &is, T &value)
{
	is.read(reinterpret_cast<char *>(&value), sizeof(T));
}

inline void read(std::istringstream &is, std::string &value)
{
	std::size_t size;
	read(is, size);
	value.resize(size);
	is.read(const_cast<char *>(value.data()), static_cast<std::streamsize>(size));
}

template <class T>
inline void read(std::istringstream &is, std::set<T> &value)
{
	std::size_t size;
	read(is, size);
	for (uint32_t i = 0; i < size; i++)
	{
		T item;
		is.read(reinterpret_cast<char *>(&item), sizeof(T));
		value.insert(std::move(item));
	}
}

template <class T>
inline void read(std::istringstream &is, std::vector<T> &value)
{
	std::size_t size;
	read(is, size);
	value.resize(size);
	is.read(reinterpret_cast<char *>(value.data()), value.size() * sizeof(T));
}

template <class T, class S>
inline void read(std::istringstream &is, std::map<T, S> &value)
{
	std::size_t size;
	read(is, size);

	for (uint32_t i = 0; i < size; i++)
	{
		std::pair<T, S> item;
		read(is, item.first);
		read(is, item.second);

		value.insert(std::move(item));
	}
}

template <class T, uint32_t N>
inline void read(std::istringstream &is, std::array<T, N> &value)
{
	is.read(reinterpret_cast<char *>(value.data()), N * sizeof(T));
}

template <typename T, typename... Args>
inline void read(std::istringstream &is, T &first_arg, Args &... args)
{
	read(is, first_arg);

	read(is, args...);
}

template <typename T>
inline void write(std::ostringstream &os, const T &value)
{
	os.write(reinterpret_cast<const char *>(&value), sizeof(T));
}

inline void write(std::ostringstream &os, const std::string &value)
{
	write(os, value.size());
	os.write(value.data(), static_cast<std::streamsize>(value.size()));
}

template <class T>
inline void write(std::ostringstream &os, const std::set<T> &value)
{
	write(os, value.size());
	for (const T &item : value)
	{
		os.write(reinterpret_cast<const char *>(&item), sizeof(T));
	}
}

template <class T>
inline void write(std::ostringstream &os, const std::vector<T> &value)
{
	write(os, value.size());
	os.write(reinterpret_cast<const char *>(value.data()), value.size() * sizeof(T));
}

template <class T, class S>
inline void write(std::ostringstream &os, const std::map<T, S> &value)
{
	write(os, value.size());

	for (const std::pair<T, S> &item : value)
	{
		write(os, item.first);
		write(os, item.second);
	}
}

template <class T, uint32_t N>
inline void write(std::ostringstream &os, const std::array<T, N> &value)
{
	os.write(reinterpret_cast<const char *>(value.data()), N * sizeof(T));
}

template <typename T, typename... Args>
inline void write(std::ostringstream &os, const T &first_arg, const Args &... args)
{
	write(os, first_arg);

	write(os, args...);
}

/**
 * @brief Helper function to combine a given hash
 *        with a generated hash for the input param.
 */
template <class T>
inline void hash_combine(size_t &seed, const T &v)
{
	std::hash<T> hasher;
	glm::detail::hash_combine(seed, hasher(v));
}

/**
 * @brief Helper function to convert a data type
 *        to string using output stream operator.
 * @param value The object to be converted to string
 * @return String version of the given object
 */
template <class T>
inline std::string to_string(const T &value)
{
	std::stringstream ss;
	ss << std::fixed << value;
	return ss.str();
}

/**
 * @brief Helper function to check size_t is correctly converted to uint32_t
 * @param value Value of type @ref size_t to convert
 * @return An @ref uint32_t representation of the same value
 */
template <class T>
uint32_t to_u32(T value)
{
	static_assert(std::is_arithmetic<T>::value, "T must be numeric");

	if (static_cast<uintmax_t>(value) > static_cast<uintmax_t>(std::numeric_limits<uint32_t>::max()))
	{
		throw std::runtime_error("to_u32() failed, value is too big to be converted to uint32_t");
	}

	return static_cast<uint32_t>(value);
}

template <typename T>
inline std::vector<uint8_t> to_bytes(const T &value)
{
	return std::vector<uint8_t>{reinterpret_cast<const uint8_t *>(&value),
	                            reinterpret_cast<const uint8_t *>(&value) + sizeof(T)};
}

}        // namespace vkb
