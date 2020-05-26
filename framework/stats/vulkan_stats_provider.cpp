/* Copyright (c) 2020, Broadcom Inc. and Contributors
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

#include "common/error.h"
#include "core/device.h"

#include "core/command_buffer.h"
#include "rendering/render_context.h"
#include "vulkan_stats_provider.h"

#include <regex>

namespace vkb
{
VulkanStatsProvider::VulkanStatsProvider(std::set<StatIndex> &        requested_stats,
                                         const CounterSamplingConfig &sampling_config,
                                         RenderContext &              render_context) :
    render_context(render_context)
{
	// Check all the Vulkan capabilities we require are present
	if (!is_supported(sampling_config))
		return;

	Device &              device = render_context.get_device();
	const PhysicalDevice &gpu    = device.get_gpu();

	has_timestamps   = gpu.get_properties().limits.timestampComputeAndGraphics;
	timestamp_period = gpu.get_properties().limits.timestampPeriod;

	// Interrogate device for supported stats
	uint32_t queue_family_index = device.get_queue_family_index(VK_QUEUE_GRAPHICS_BIT);

	// Query number of available counters
	uint32_t count = 0;
	gpu.enumerate_queue_family_performance_query_counters(queue_family_index, &count,
	                                                      nullptr, nullptr);

	if (count == 0)
		return;        // No counters available

	std::vector<VkPerformanceCounterKHR>            counters(count);
	std::vector<VkPerformanceCounterDescriptionKHR> descs(count);

	for (uint32_t i = 0; i < count; i++)
	{
		counters[i].sType = VK_STRUCTURE_TYPE_PERFORMANCE_COUNTER_KHR;
		counters[i].pNext = nullptr;
		descs[i].sType    = VK_STRUCTURE_TYPE_PERFORMANCE_COUNTER_DESCRIPTION_KHR;
		descs[i].pNext    = nullptr;
	}

	// Now get the list of counters and their descriptions
	gpu.enumerate_queue_family_performance_query_counters(queue_family_index, &count,
	                                                      counters.data(), descs.data());

	// Every vendor has a different set of performance counters each
	// with different names. Match them to the stats we want, where available.
	if (!fill_vendor_data())
		return;

	bool performance_impact = false;

	// Now build stat_data by matching vendor_data to Vulkan counter data
	for (auto &s : vendor_data)
	{
		StatIndex index = s.first;

		if (requested_stats.find(index) == requested_stats.end())
			continue;        // We weren't asked for this stat

		VendorStat &init      = s.second;
		bool        found_ctr = false;
		bool        found_div = (init.divisor_name == "");
		uint32_t    ctr_idx, div_idx;

		std::regex name_regex(init.name);
		std::regex div_regex(init.divisor_name);

		for (uint32_t i = 0; !(found_ctr && found_div) && i < descs.size(); i++)
		{
			if (!found_ctr && std::regex_match(descs[i].name, name_regex))
			{
				ctr_idx   = i;
				found_ctr = true;
			}
			if (!found_div && std::regex_match(descs[i].name, div_regex))
			{
				div_idx   = i;
				found_div = true;
			}
		}

		if (found_ctr && found_div)
		{
			if ((descs[ctr_idx].flags & VK_PERFORMANCE_COUNTER_DESCRIPTION_PERFORMANCE_IMPACTING_KHR) ||
			    (init.divisor_name != "" && descs[div_idx].flags != VK_PERFORMANCE_COUNTER_DESCRIPTION_PERFORMANCE_IMPACTING_KHR))
			{
				performance_impact = true;
			}

			// Record the counter data
			counter_indices.emplace_back(ctr_idx);
			if (init.divisor_name == "")
			{
				stat_data[index] = StatData(ctr_idx, counters[ctr_idx].storage);
			}
			else
			{
				counter_indices.emplace_back(div_idx);
				stat_data[index] = StatData(ctr_idx, counters[ctr_idx].storage, init.scaling,
				                            div_idx, counters[div_idx].storage);
			}
		}
	}

	if (performance_impact)
		LOGW("The collection of performance counters may impact performance");

	if (counter_indices.size() == 0)
		return;        // No stats available

	// Acquire the profiling lock, without which we can't collect stats
	VkAcquireProfilingLockInfoKHR info{};
	info.sType   = VK_STRUCTURE_TYPE_ACQUIRE_PROFILING_LOCK_INFO_KHR;
	info.timeout = 2000000000;        // 2 seconds (in ns)

	if (vkAcquireProfilingLockKHR(device.get_handle(), &info) != VK_SUCCESS)
	{
		stat_data.clear();
		counter_indices.clear();
		LOGW("Profiling lock acquisition timed-out");
		return;
	}

	// Now we know the counters and that we can collect them, make a query pool for the results.
	if (!create_query_pools(queue_family_index))
	{
		stat_data.clear();
		counter_indices.clear();
		return;
	}

	// These stats are fully supported by this provider and in a single pass, so remove
	// from the requested set.
	// Subsequent providers will then only look for things that aren't already supported.
	for (const auto &s : stat_data)
		requested_stats.erase(s.first);
}

VulkanStatsProvider::~VulkanStatsProvider()
{
	if (stat_data.size() > 0)
	{
		// Release profiling lock
		vkReleaseProfilingLockKHR(render_context.get_device().get_handle());
	}
}

bool VulkanStatsProvider::fill_vendor_data()
{
	const auto &pd_props = render_context.get_device().get_gpu().get_properties();
	if (pd_props.vendorID == 0x14E4)        // Broadcom devices
	{
		LOGI("Using Vulkan performance counters from Broadcom device");

		// NOTE: The names here are actually regular-expressions.
		// Counter names can change between hardware variants for the same vendor,
		// so regular expression names mean that multiple h/w variants can be easily supported.
		// clang-format off
		vendor_data = {
		    {StatIndex::gpu_cycles,          {"cycle_count"}},
		    {StatIndex::gpu_vertex_cycles,   {"gpu_vertex_cycles"}},
		    {StatIndex::gpu_fragment_cycles, {"gpu_fragment_cycles"}},
		    {StatIndex::gpu_fragment_jobs,   {"render_jobs_completed"}},
		    {StatIndex::gpu_ext_reads,       {"gpu_mem_reads"}},
		    {StatIndex::gpu_ext_writes,      {"gpu_mem_writes"}},
		    {StatIndex::gpu_ext_read_bytes,  {"gpu_bytes_read"}},
		    {StatIndex::gpu_ext_write_bytes, {"gpu_bytes_written"}},
		};
		// clang-format on

		// Override vendor-specific graph data
		vendor_data.at(StatIndex::gpu_vertex_cycles).set_vendor_graph_data({"Vertex/Coord/User Cycles", "{:4.1f} M/s", float(1e-6)});
		vendor_data.at(StatIndex::gpu_fragment_jobs).set_vendor_graph_data({"Render Jobs", "{:4.0f}/s"});

		return true;
	}
#if 0
	else if (pd_props.vendorID == xxxx) // Other vendor's devices
	{
		// Fill vendor_data for other vendor
		return true;
	}
#endif
	{
		// Unsupported vendor
		return false;
	}
}

bool VulkanStatsProvider::create_query_pools(uint32_t queue_family_index)
{
	Device &              device           = render_context.get_device();
	const PhysicalDevice &gpu              = device.get_gpu();
	uint32_t              num_framebuffers = uint32_t(render_context.get_render_frames().size());

	// Now we know the available counters, we can build a query pool that will collect them.
	// We will check that the counters can be collected in a single pass. Multi-pass would
	// be a big performance hit so for these samples, we don't want to use it.
	VkQueryPoolPerformanceCreateInfoKHR perf_create_info{};
	perf_create_info.sType             = VK_STRUCTURE_TYPE_QUERY_POOL_PERFORMANCE_CREATE_INFO_KHR;
	perf_create_info.queueFamilyIndex  = queue_family_index;
	perf_create_info.counterIndexCount = uint32_t(counter_indices.size());
	perf_create_info.pCounterIndices   = counter_indices.data();

	uint32_t passes_needed = gpu.get_queue_family_performance_query_passes(&perf_create_info);
	if (passes_needed != 1)
	{
		// Needs more than one pass, remove all our supported stats
		LOGW("Requested Vulkan stats require multiple passes, we won't collect them");
		return false;
	}

	// We will need a query pool to report the stats back to us
	VkQueryPoolCreateInfo pool_create_info{};
	pool_create_info.sType      = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	pool_create_info.pNext      = &perf_create_info;
	pool_create_info.queryType  = VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR;
	pool_create_info.queryCount = num_framebuffers;

	query_pool = std::make_unique<QueryPool>(device, pool_create_info);

	if (!query_pool)
	{
		LOGW("Failed to create performance query pool");
		return false;
	}

	// Reset the query pool before first use. We cannot do these in the command buffer
	// as that is invalid usage for performance queries due to the potential for multple
	// passes being required.
	query_pool->host_reset(0, num_framebuffers);

	if (has_timestamps)
	{
		// If we support timestamp queries we will use those to more accurately measure
		// the time spent executing a command buffer than just a frame-to-frame timer
		// in software.
		VkQueryPoolCreateInfo timestamp_pool_create_info{};
		timestamp_pool_create_info.sType      = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		timestamp_pool_create_info.queryType  = VK_QUERY_TYPE_TIMESTAMP;
		timestamp_pool_create_info.queryCount = num_framebuffers * 2;        // 2 timestamps per frame (start & end)

		timestamp_pool = std::make_unique<QueryPool>(device, timestamp_pool_create_info);
	}

	return true;
}

bool VulkanStatsProvider::is_supported(const CounterSamplingConfig &sampling_config) const
{
	// Continuous sampling mode cannot be supported by VK_KHR_performance_query
	if (sampling_config.mode == CounterSamplingMode::Continuous)
		return false;

	Device &device = render_context.get_device();

	// The VK_KHR_performance_query must be available and enabled
	if (!(device.is_enabled("VK_KHR_performance_query") && device.is_enabled("VK_EXT_host_query_reset")))
		return false;

	// Check the performance query features flag.
	// Note: VK_KHR_get_physical_device_properties2 is a pre-requisite of VK_KHR_performance_query
	// so must be present.
	VkPhysicalDevicePerformanceQueryFeaturesKHR perf_query_features{};
	perf_query_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_FEATURES_KHR;

	VkPhysicalDeviceFeatures2KHR device_features{};
	device_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
	device_features.pNext = &perf_query_features;

	vkGetPhysicalDeviceFeatures2(device.get_gpu().get_handle(), &device_features);
	if (!perf_query_features.performanceCounterQueryPools)
		return false;

	return true;
}

bool VulkanStatsProvider::is_available(StatIndex index) const
{
	return stat_data.find(index) != stat_data.end();
}

const StatGraphData &VulkanStatsProvider::get_graph_data(StatIndex index) const
{
	assert(is_available(index) && "VulkanStatsProvider::get_graph_data() called with invalid StatIndex");

	const auto &data = vendor_data.find(index)->second;
	if (data.has_vendor_graph_data)
		return data.graph_data;

	return default_graph_map[index];
}

void VulkanStatsProvider::begin_sampling(CommandBuffer &cb)
{
	uint32_t active_frame_idx = render_context.get_active_frame_index();
	if (timestamp_pool)
	{
		// We use TimestampQueries when available to provide a more accurate delta_time.
		// This counters are from a single command buffer execution, but the passed
		// delta time is a frame-to-frame s/w measure. A timestamp query in the the cmd
		// buffer gives the actual elapsed time where the counters were measured.
		cb.reset_query_pool(*timestamp_pool, active_frame_idx * 2, 1);
		cb.write_timestamp(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, *timestamp_pool,
		                   active_frame_idx * 2);
	}

	if (query_pool)
		cb.begin_query(*query_pool, active_frame_idx, VkQueryControlFlags(0));
}

void VulkanStatsProvider::end_sampling(CommandBuffer &cb)
{
	uint32_t active_frame_idx = render_context.get_active_frame_index();

	if (query_pool)
	{
		// Perform a barrier to ensure all previous commands complete before ending the query
		// This does not block later commands from executing as we use BOTTOM_OF_PIPE in the
		// dst stage mask
		vkCmdPipelineBarrier(cb.get_handle(),
		                     VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		                     VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		                     0, 0, nullptr, 0, nullptr, 0, nullptr);
		cb.end_query(*query_pool, active_frame_idx);

		++queries_ready;
	}

	if (timestamp_pool)
	{
		cb.reset_query_pool(*timestamp_pool, active_frame_idx * 2 + 1, 1);
		cb.write_timestamp(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, *timestamp_pool,
		                   active_frame_idx * 2 + 1);
	}
}

static double get_counter_value(const VkPerformanceCounterResultKHR &result,
                                VkPerformanceCounterStorageKHR       storage)
{
	switch (storage)
	{
		case VK_PERFORMANCE_COUNTER_STORAGE_INT32_KHR:
			return double(result.int32);
		case VK_PERFORMANCE_COUNTER_STORAGE_INT64_KHR:
			return double(result.int64);
		case VK_PERFORMANCE_COUNTER_STORAGE_UINT32_KHR:
			return double(result.uint32);
		case VK_PERFORMANCE_COUNTER_STORAGE_UINT64_KHR:
			return double(result.uint64);
		case VK_PERFORMANCE_COUNTER_STORAGE_FLOAT32_KHR:
			return double(result.float32);
		case VK_PERFORMANCE_COUNTER_STORAGE_FLOAT64_KHR:
			return double(result.float64);
		default:
			assert(0);
			return 0.0;
	}
}

float VulkanStatsProvider::get_best_delta_time(float sw_delta_time) const
{
	if (!timestamp_pool)
		return sw_delta_time;

	float delta_time = sw_delta_time;

	// Query the timestamps to get an accurate delta time
	std::array<uint64_t, 2> timestamps;

	uint32_t active_frame_idx = render_context.get_active_frame_index();

	VkResult r = timestamp_pool->get_results(active_frame_idx * 2, 2,
	                                         timestamps.size() * sizeof(uint64_t),
	                                         timestamps.data(), sizeof(uint64_t),
	                                         VK_QUERY_RESULT_WAIT_BIT | VK_QUERY_RESULT_64_BIT);
	if (r == VK_SUCCESS)
	{
		float elapsed_ns = timestamp_period * float(timestamps[1] - timestamps[0]);
		delta_time       = elapsed_ns * 0.000000001f;
	}

	return delta_time;
}

StatsProvider::Counters VulkanStatsProvider::sample(float delta_time)
{
	Counters out;
	if (!query_pool || queries_ready == 0)
		return out;

	uint32_t active_frame_idx = render_context.get_active_frame_index();

	VkDeviceSize stride = sizeof(VkPerformanceCounterResultKHR) * counter_indices.size();

	std::vector<VkPerformanceCounterResultKHR> results(counter_indices.size());

	VkResult r = query_pool->get_results(active_frame_idx, 1,
	                                     results.size() * sizeof(VkPerformanceCounterResultKHR),
	                                     results.data(), stride, VK_QUERY_RESULT_WAIT_BIT);
	if (r != VK_SUCCESS)
		return out;

	// Use timestamps to get a more accurate delta if available
	delta_time = get_best_delta_time(delta_time);

	// Parse the results - they are in the order we gave in counter_indices
	for (const auto &s : stat_data)
	{
		StatIndex si = s.first;

		bool   need_divisor  = (stat_data[si].scaling == StatScaling::ByCounter);
		double divisor_value = 1.0;
		double value         = 0.0;
		bool   found_ctr = false, found_div = !need_divisor;

		for (uint32_t i = 0; !(found_ctr && found_div) && i < counter_indices.size(); i++)
		{
			if (s.second.counter_index == counter_indices[i])
			{
				value     = get_counter_value(results[i], stat_data[si].storage);
				found_ctr = true;
			}
			if (need_divisor && s.second.divisor_counter_index == counter_indices[i])
			{
				divisor_value = get_counter_value(results[i], stat_data[si].divisor_storage);
				found_div     = true;
			}
		}

		if (found_ctr && found_div)
		{
			if (stat_data[si].scaling == StatScaling::ByDeltaTime && delta_time != 0.0)
				value /= delta_time;
			else if (stat_data[si].scaling == StatScaling::ByCounter && divisor_value != 0.0)
				value /= divisor_value;
			out[si].result = value;
		}
	}

	// Now reset the query we just fetched the results from
	query_pool->host_reset(active_frame_idx, 1);

	--queries_ready;

	return out;
}

}        // namespace vkb
