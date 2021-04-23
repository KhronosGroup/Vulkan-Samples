/* Copyright (c) 2021, Arm Limited and Contributors
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

#include <cassert>

namespace vkb
{
// Simple optional class for pre c++17 std::optional
// May be replaced by std::optional in the future
template <typename Type>
class Optional
{
  public:
	virtual ~Optional() = default;
	Optional()          = default;

	Optional(const Optional &optional);

	template <typename U = Type>
	Optional(const U &value);

	bool        has_value() const;
	const Type &value() const;
	const Type  value_or(Type &&alternative) const;
	const Type  value_or(const Type &alternative = {}) const;

	Optional &operator=(Optional &&opt)
	{
		_has_value = opt._has_value;
		_value     = opt._value;
		return *this;
	}

	template <typename U = Type>
	Optional &operator=(U &&value)
	{
		_has_value = true;
		_value     = value;
		return *this;
	}

	Optional &operator=(const Optional &value)
	{
		_has_value = value._has_value;
		_value     = value._value;
		return *this;
	}

	template <typename U = Type>
	Optional &operator=(U *value)
	{
		if (value == nullptr)
		{
			_has_value = false;
			return *this;
		}

		_has_value = true;
		_value     = *value;
		return *this;
	}

  private:
	bool _has_value = false;
	Type _value;
};

template <typename Type>
Optional<Type>::Optional(const Optional &optional)
{
	_has_value = optional._has_value;
	_value     = optional._value;
}

template <typename Type>
template <typename U>
Optional<Type>::Optional(const U &value)
{
	_has_value = true;
	_value     = value;
}

template <typename Type>
bool Optional<Type>::has_value() const
{
	return _has_value;
}

template <typename Type>
const Type &Optional<Type>::value() const
{
	assert(_has_value && "Value does not exist");
	return _value;
}

template <typename Type>
const Type Optional<Type>::value_or(Type &&alternative) const
{
	if (has_value())
	{
		return _value;
	}

	return alternative;
}

template <typename Type>
const Type Optional<Type>::value_or(const Type &alternative) const
{
	if (has_value())
	{
		return _value;
	}

	return alternative;
}
}        // namespace vkb