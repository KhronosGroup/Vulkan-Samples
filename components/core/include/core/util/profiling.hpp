/* Copyright (c) 2023-2024, Thomas Atkinson
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

#include <cstdint>
#include <cstdio>

#include <unordered_map>

#include "core/util/error.hpp"

VKBP_DISABLE_WARNINGS()
#include <tracy/Tracy.hpp>
VKBP_ENABLE_WARNINGS()

// malloc and free are used by Tracy to provide memory profiling
void *operator new(size_t count);
void  operator delete(void *ptr) noexcept;

// Tracy a scope
#define PROFILE_SCOPE(name) ZoneScopedN(name)

// Trace a function
#define PROFILE_FUNCTION() ZoneScoped

// The type of plot to use
enum class PlotType
{
	Number,
	Percentage,
	Memory,
};

// tracy::PlotFormatType is not defined if TRACY_ENABLE is not defined
// so we need to define a function to convert our enum to the tracy enum
#ifdef TRACY_ENABLE
namespace
{
inline tracy::PlotFormatType to_tracy_plot_format(PlotType type)
{
	switch (type)
	{
		case PlotType::Number:
			return tracy::PlotFormatType::Number;
		case PlotType::Percentage:
			return tracy::PlotFormatType::Percentage;
		case PlotType::Memory:
			return tracy::PlotFormatType::Memory;
		default:
			return tracy::PlotFormatType::Number;
	}
}
}        // namespace

#	define TO_TRACY_PLOT_FORMAT(name) to_tracy_plot_format(name)
#else
#	define TO_TRACY_PLOT_FORMAT(name)
#endif

// Create plots
template <typename T, PlotType PT = PlotType::Number>
class Plot
{
  public:
	static void plot(const char *name, T value)
	{
		auto *p        = get_instance();
		p->plots[name] = value;
		update_tracy_plot(name, value);
	}

	static void increment(const char *name, T amount)
	{
		auto *p = get_instance();
		p->plots[name] += amount;
		update_tracy_plot(name, p->plots[name]);
	}

	static void decrement(const char *name, T amount)
	{
		auto *p = get_instance();
		p->plots[name] -= amount;
		update_tracy_plot(name, p->plots[name]);
	}

	static void reset(const char *name)
	{
		auto *p        = get_instance();
		p->plots[name] = T{};
		update_tracy_plot(name, p->plots[name]);
	}

  private:
	static void update_tracy_plot(const char *name, T value)
	{
		TracyPlot(name, value);
		TracyPlotConfig(name, TO_TRACY_PLOT_FORMAT(PT), true, true, 0);
	}

	static Plot *get_instance()
	{
		static_assert((std::is_same<T, int64_t>::value || std::is_same<T, double>::value || std::is_same<T, float>::value), "PlotStore only supports int64_t, double and float");
		static Plot instance;
		return &instance;
	}

	std::unordered_map<const char *, T> plots;
};
