/* Copyright (c) 2018-2022, Arm Limited and Contributors
 * Copyright (c) 2020-2022, Broadcom Inc.
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

namespace vkb
{
HWCPipeStatsProvider::HWCPipeStatsProvider(std::set<StatIndex> &requested_stats)
{
	// Mapping of stats to their hwcpipe availability
	// clang-format off
	StatDataMap hwcpipe_stats = {
	    {StatIndex::cpu_cycles,            {hwcpipe::CpuCounter::Cycles}},
	    {StatIndex::cpu_instructions,      {hwcpipe::CpuCounter::Instructions}},
	    {StatIndex::cpu_cache_miss_ratio,  {hwcpipe::CpuCounter::CacheMisses,  StatScaling::ByCounter, hwcpipe::CpuCounter::CacheReferences}},
	    {StatIndex::cpu_branch_miss_ratio, {hwcpipe::CpuCounter::BranchMisses, StatScaling::ByCounter, hwcpipe::CpuCounter::BranchInstructions}},
	    {StatIndex::cpu_l1_accesses,       {hwcpipe::CpuCounter::L1Accesses}},
	    {StatIndex::cpu_instr_retired,     {hwcpipe::CpuCounter::InstrRetired}},
	    {StatIndex::cpu_l2_accesses,       {hwcpipe::CpuCounter::L2Accesses}},
	    {StatIndex::cpu_l3_accesses,       {hwcpipe::CpuCounter::L3Accesses}},
	    {StatIndex::cpu_bus_reads,         {hwcpipe::CpuCounter::BusReads}},
	    {StatIndex::cpu_bus_writes,        {hwcpipe::CpuCounter::BusWrites}},
	    {StatIndex::cpu_mem_reads,         {hwcpipe::CpuCounter::MemReads}},
	    {StatIndex::cpu_mem_writes,        {hwcpipe::CpuCounter::MemWrites}},
	    {StatIndex::cpu_ase_spec,          {hwcpipe::CpuCounter::ASESpec}},
	    {StatIndex::cpu_vfp_spec,          {hwcpipe::CpuCounter::VFPSpec}},
	    {StatIndex::cpu_crypto_spec,       {hwcpipe::CpuCounter::CryptoSpec}},
	    {StatIndex::gpu_cycles,            {hwcpipe::GpuCounter::GpuCycles}},
	    {StatIndex::gpu_vertex_cycles,     {hwcpipe::GpuCounter::VertexComputeCycles}},
	    {StatIndex::gpu_load_store_cycles, {hwcpipe::GpuCounter::ShaderLoadStoreCycles}},
	    {StatIndex::gpu_tiles,             {hwcpipe::GpuCounter::Tiles}},
	    {StatIndex::gpu_killed_tiles,      {hwcpipe::GpuCounter::TransactionEliminations}}, 
	    {StatIndex::gpu_fragment_cycles,   {hwcpipe::GpuCounter::FragmentCycles}},
	    {StatIndex::gpu_fragment_jobs,     {hwcpipe::GpuCounter::FragmentJobs}},
	    {StatIndex::gpu_ext_reads,         {hwcpipe::GpuCounter::ExternalMemoryReadAccesses}},
	    {StatIndex::gpu_ext_writes,        {hwcpipe::GpuCounter::ExternalMemoryWriteAccesses}},
	    {StatIndex::gpu_ext_read_stalls,   {hwcpipe::GpuCounter::ExternalMemoryReadStalls}},
	    {StatIndex::gpu_ext_write_stalls,  {hwcpipe::GpuCounter::ExternalMemoryWriteStalls}},
	    {StatIndex::gpu_ext_read_bytes,    {hwcpipe::GpuCounter::ExternalMemoryReadBytes}},
	    {StatIndex::gpu_ext_write_bytes,   {hwcpipe::GpuCounter::ExternalMemoryWriteBytes}},
	    {StatIndex::gpu_tex_cycles,        {hwcpipe::GpuCounter::ShaderTextureCycles}}};
	// clang-format on

	hwcpipe::CpuCounterSet enabled_cpu_counters{};
	hwcpipe::GpuCounterSet enabled_gpu_counters{};

	for (const auto &stat : requested_stats)
	{
		auto res = hwcpipe_stats.find(stat);
		if (res != hwcpipe_stats.end())
		{
			stat_data[stat] = hwcpipe_stats[stat];

			switch (res->second.type)
			{
				case StatType::Cpu:
					enabled_cpu_counters.insert(res->second.cpu_counter);
					if (res->second.divisor_cpu_counter != hwcpipe::CpuCounter::MaxValue)
					{
						enabled_cpu_counters.insert(res->second.divisor_cpu_counter);
					}
					break;
				case StatType::Gpu:
					enabled_gpu_counters.insert(res->second.gpu_counter);
					if (res->second.divisor_gpu_counter != hwcpipe::GpuCounter::MaxValue)
					{
						enabled_gpu_counters.insert(res->second.divisor_gpu_counter);
					}
					break;
			}
		}
	}

	hwcpipe = std::make_unique<hwcpipe::HWCPipe>(enabled_cpu_counters, enabled_gpu_counters);

	// Now that we've made a hwcpipe with the counters we'd like, remove any that
	// aren't actually supported
	for (auto iter = stat_data.begin(); iter != stat_data.end();)
	{
		switch (iter->second.type)
		{
			case StatType::Cpu: {
				if (hwcpipe->cpu_profiler())
				{
					const auto &cpu_supp = hwcpipe->cpu_profiler()->supported_counters();
					if (cpu_supp.find(iter->second.cpu_counter) == cpu_supp.end())
					{
						iter = stat_data.erase(iter);
					}
					else
					{
						++iter;
					}
				}
				else
				{
					iter = stat_data.erase(iter);
				}
				break;
			}
			case StatType::Gpu: {
				if (hwcpipe->gpu_profiler())
				{
					const auto &gpu_supp = hwcpipe->gpu_profiler()->supported_counters();
					if (gpu_supp.find(iter->second.gpu_counter) == gpu_supp.end())
					{
						iter = stat_data.erase(iter);
					}
					else
					{
						++iter;
					}
				}
				else
				{
					iter = stat_data.erase(iter);
				}
				break;
			}
		}
	}

	// Remove any supported stats from the requested set.
	// Subsequent providers will then only look for things that aren't already supported.
	for (const auto &iter : stat_data)
	{
		requested_stats.erase(iter.first);
	}

	hwcpipe->run();
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

static double get_cpu_counter_value(const hwcpipe::CpuMeasurements *cpu, hwcpipe::CpuCounter counter)
{
	auto hwcpipe_ctr = cpu->find(counter);
	if (hwcpipe_ctr != cpu->end())
	{
		return hwcpipe_ctr->second.get<double>();
	}
	return 0.0;
}

static double get_gpu_counter_value(const hwcpipe::GpuMeasurements *gpu, hwcpipe::GpuCounter counter)
{
	auto hwcpipe_ctr = gpu->find(counter);
	if (hwcpipe_ctr != gpu->end())
	{
		return hwcpipe_ctr->second.get<double>();
	}
	return 0.0;
}

StatsProvider::Counters HWCPipeStatsProvider::sample(float delta_time)
{
	Counters              res;
	hwcpipe::Measurements m = hwcpipe->sample();

	// Map from hwcpipe measurement to our sample result for each counter
	for (auto iter : stat_data)
	{
		StatIndex       index = iter.first;
		const StatData &data  = iter.second;

		double d = 0.0;
		if (data.type == StatType::Cpu)
		{
			d = get_cpu_counter_value(m.cpu, data.cpu_counter);

			if (data.scaling == StatScaling::ByDeltaTime && delta_time != 0.0f)
			{
				d /= delta_time;
			}
			else if (data.scaling == StatScaling::ByCounter)
			{
				double divisor = get_cpu_counter_value(m.cpu, data.divisor_cpu_counter);
				if (divisor != 0.0)
				{
					d /= divisor;
				}
				else
				{
					d = 0.0;
				}
			}
		}
		else if (data.type == StatType::Gpu)
		{
			d = get_gpu_counter_value(m.gpu, data.gpu_counter);

			if (data.scaling == StatScaling::ByDeltaTime && delta_time != 0.0f)
			{
				d /= delta_time;
			}
			else if (data.scaling == StatScaling::ByCounter)
			{
				double divisor = get_gpu_counter_value(m.gpu, data.divisor_gpu_counter);
				if (divisor != 0.0)
				{
					d /= divisor;
				}
				else
				{
					d = 0.0;
				}
			}
		}
		res[index].result = d;
	}

	return res;
}

StatsProvider::Counters HWCPipeStatsProvider::continuous_sample(float delta_time)
{
	return sample(delta_time);
}

}        // namespace vkb
