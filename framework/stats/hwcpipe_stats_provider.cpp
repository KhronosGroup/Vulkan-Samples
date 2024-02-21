/* Copyright (c) 2018-2024, Arm Limited and Contributors
 * Copyright (c) 2020-2024, Broadcom Inc.
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

#include "hwcpipe_stats_provider.h"

namespace
{
const char *get_product_family_name(hwcpipe::device::product_id::gpu_family f)
{
	using gpu_family = hwcpipe::device::product_id::gpu_family;

	switch (f)
	{
		case gpu_family::bifrost:
			return "Bifrost";
		case gpu_family::midgard:
			return "Midgard";
		case gpu_family::valhall:
			return "Valhall";
		default:
			return "Unknown";
	}
}
}        // namespace

namespace vkb
{
HWCPipeStatsProvider::HWCPipeStatsProvider(std::set<StatIndex> &requested_stats)
{
	// Mapping of stats to their hwcpipe availability
	// clang-format off
	StatDataMap hwcpipe_stats = {
	    {StatIndex::gpu_cycles,            {hwcpipe_counter::MaliGPUActiveCy}},
	    {StatIndex::gpu_vertex_cycles,     {hwcpipe_counter::MaliNonFragQueueActiveCy}},
	    {StatIndex::gpu_load_store_cycles, {hwcpipe_counter::MaliLSIssueCy}},
	    {StatIndex::gpu_tiles,             {hwcpipe_counter::MaliFragTile}},
	    {StatIndex::gpu_killed_tiles,      {hwcpipe_counter::MaliFragTileKill}},
	    {StatIndex::gpu_fragment_cycles,   {hwcpipe_counter::MaliFragQueueActiveCy}},
	    {StatIndex::gpu_fragment_jobs,     {hwcpipe_counter::MaliFragQueueJob}},
	    {StatIndex::gpu_ext_reads,         {hwcpipe_counter::MaliExtBusRdBt}},
	    {StatIndex::gpu_ext_writes,        {hwcpipe_counter::MaliExtBusWrBt}},
	    {StatIndex::gpu_ext_read_stalls,   {hwcpipe_counter::MaliExtBusRdStallCy}},
	    {StatIndex::gpu_ext_write_stalls,  {hwcpipe_counter::MaliExtBusWrStallCy}},
	    {StatIndex::gpu_ext_read_bytes,    {hwcpipe_counter::MaliExtBusRdBy}},
	    {StatIndex::gpu_ext_write_bytes,   {hwcpipe_counter::MaliExtBusWrBy}},
	    {StatIndex::gpu_tex_cycles,        {hwcpipe_counter::MaliTexIssueCy}}};
	// clang-format on

	// Detect all GPUs & print some info
	for (const auto &gpu : hwcpipe::find_gpus())
	{
		LOGI("HWCPipe: ------------------------------------------------------------");
		LOGI("HWCPipe:  GPU Device {}:", gpu.get_device_number());
		LOGI("HWCPipe: ------------------------------------------------------------");
		LOGI("HWCPipe:     Product Family:  {}", get_product_family_name(gpu.get_product_id().get_gpu_family()));
		LOGI("HWCPipe:     Number of Cores: {}", gpu.num_shader_cores());
		LOGI("HWCPipe:     Bus Width:       {}", gpu.bus_width());
	}

	// Probe device 0 (i.e. /dev/mali0)
	auto gpu = hwcpipe::gpu(0);
	if (!gpu)
	{
		LOGE("HWCPipe: Mali GPU device 0 is missing");
	}

	auto config     = hwcpipe::sampler_config(gpu);
	auto counter_db = hwcpipe::counter_database{};

	std::error_code ec;
	for (const auto &stat : requested_stats)
	{
		auto it = hwcpipe_stats.find(stat);
		if (it != hwcpipe_stats.end())
		{
			hwcpipe::counter_metadata meta;

			auto ec = counter_db.describe_counter(it->second.counter, meta);
			if (ec)
			{
				LOGE("HWCPipe: unknown counter");
			}

			ec = config.add_counter(it->second.counter);
			if (ec)
			{
				LOGE("HWCPipe: '{}' counter not supported by this GPU.", meta.name);
			}
			else
			{
				stat_data[stat] = hwcpipe_stats[stat];

				LOGI("HWCPipe: enabled '{}' counter", meta.name);
			}
		}
	}

	// Remove any supported stats from the requested set.
	// Subsequent providers will then only look for things that aren't already supported.
	for (const auto &iter : stat_data)
	{
		requested_stats.erase(iter.first);
	}

	sampler = std::make_unique<hwcpipe::sampler<>>(config);

	ec = sampler->start_sampling();
	if (ec)
	{
		LOGE("HWCPipe: {}", ec.message());
	}
}

HWCPipeStatsProvider::~HWCPipeStatsProvider()
{
	std::error_code ec;

	ec = sampler->stop_sampling();
	if (ec)
	{
		LOGE("HWCPipe: {}", ec.message());
	}
}

bool HWCPipeStatsProvider::is_available(StatIndex index) const
{
	return stat_data.find(index) != stat_data.end();
}

const StatGraphData &HWCPipeStatsProvider::get_graph_data(StatIndex index) const
{
	assert(is_available(index) && "HWCPipeStatsProvider::get_graph_data() called with invalid StatIndex");

	static StatGraphData vertex_compute_cycles{"Vertex Compute Cycles", "{:4.1f} M/s", static_cast<float>(1e-6)};

	// HWCPipe reports combined vertex/compute cycles (which is Arm specific)
	// Ensure we report graph with the correct name when asked for vertex cycles
	if (index == StatIndex::gpu_vertex_cycles)
	{
		return vertex_compute_cycles;
	}

	return default_graph_map[index];
}

static double get_gpu_counter_value(const hwcpipe::counter_sample &sample)
{
	switch (sample.type)
	{
		case hwcpipe::counter_sample::type::uint64:
			return sample.value.uint64;
		case hwcpipe::counter_sample::type::float64:
			return sample.value.float64;
		default:
			return 0.0;
	}
}

StatsProvider::Counters HWCPipeStatsProvider::sample(float delta_time)
{
	Counters res;

	std::error_code ec;

	ec = sampler->sample_now();
	if (ec)
	{
		LOGE("HWCPipe: {}", ec.message());
	}
	else
	{
		// Map from hwcpipe measurement to our sample result for each counter
		for (auto iter : stat_data)
		{
			StatIndex       index = iter.first;
			const StatData &data  = iter.second;

			hwcpipe::counter_sample sample;

			ec = sampler->get_counter_value(data.counter, sample);
			if (ec)
			{
				LOGE("HWCPipe: {}", ec.message());
				continue;
			}

			auto d = get_gpu_counter_value(sample);

			if (data.scaling == StatScaling::ByDeltaTime && delta_time != 0.0f)
			{
				d /= delta_time;
			}
			else if (data.scaling == StatScaling::ByCounter)
			{
				ec = sampler->get_counter_value(data.divisor, sample);
				if (ec)
				{
					LOGE("HWCPipe: {}", ec.message());
					continue;
				}

				double divisor = get_gpu_counter_value(sample);
				if (divisor != 0.0)
				{
					d /= divisor;
				}
				else
				{
					d = 0.0;
				}
			}

			res[index].result = d;
		}
	}

	return res;
}

StatsProvider::Counters HWCPipeStatsProvider::continuous_sample(float delta_time)
{
	return sample(delta_time);
}

}        // namespace vkb
