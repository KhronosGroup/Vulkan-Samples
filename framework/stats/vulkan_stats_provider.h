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

#pragma once

#include "core/query_pool.h"
#include "stats_provider.h"

namespace vkb
{
class VulkanStatsProvider : public StatsProvider
{
  private:
	struct StatData
	{
		StatScaling                    scaling;
		uint32_t                       counter_index;
		uint32_t                       divisor_counter_index;
		VkPerformanceCounterStorageKHR storage;
		VkPerformanceCounterStorageKHR divisor_storage;
		StatGraphData                  graph_data;

		StatData() = default;

		StatData(uint32_t counter_index, VkPerformanceCounterStorageKHR storage,
		         StatScaling                    stat_scaling    = StatScaling::ByDeltaTime,
		         uint32_t                       divisor_index   = std::numeric_limits<uint32_t>::max(),
		         VkPerformanceCounterStorageKHR divisor_storage = VK_PERFORMANCE_COUNTER_STORAGE_FLOAT64_KHR) :
		    scaling(stat_scaling),
		    counter_index(counter_index),
		    divisor_counter_index(divisor_index),
		    storage(storage),
		    divisor_storage(divisor_storage)
		{}
	};

	struct VendorStat
	{
		VendorStat(const std::string &name, const std::string &divisor_name = "") :
		    name(name),
		    divisor_name(divisor_name)
		{
			if (divisor_name != "")
				scaling = StatScaling::ByCounter;
		}

		void set_vendor_graph_data(const StatGraphData &data)
		{
			has_vendor_graph_data = true;
			graph_data            = data;
		}

		std::string   name;
		StatScaling   scaling = StatScaling::ByDeltaTime;
		std::string   divisor_name;
		bool          has_vendor_graph_data = false;
		StatGraphData graph_data;
	};

	using StatDataMap   = std::unordered_map<StatIndex, StatData, StatIndexHash>;
	using VendorStatMap = std::unordered_map<StatIndex, VendorStat, StatIndexHash>;

  public:
	/**
	 * @brief Constructs a VulkanStatsProvider
	 * @param device The device on which to collect stats
	 * @param requested_stats Set of stats to be collected. Supported stats will be removed from the set.
	 */
	VulkanStatsProvider(Device &device, std::set<StatIndex> &requested_stats,
	                    const CounterSamplingConfig &sampling_config, size_t num_framebuffers);

	/**
	 * @brief Destructs a VulkanStatsProvider
	 */
	~VulkanStatsProvider();

	/**
	 * @brief Checks if this provider can supply the given enabled stat
	 * @param index The stat index
	 * @return True if the stat is available, false otherwise
	 */
	bool is_available(StatIndex index) const override;

	/**
	 * @brief Retrieve graphing data for the given enabled stat
	 * @param index The stat index
	 */
	const StatGraphData &get_graph_data(StatIndex index) const override;

	/**
	 * @brief Retrieve a new sample set from polled sampling
	 * @param delta_time Time since last sample
	 * @param active_frame_idx Which of the framebuffers is active
	 */
	Sample sample(float delta_time, uint32_t active_frame_idx) override;

	/**
	 * @brief A command buffer that we want stats about has just begun
	 * @param cb The command buffer
 	 * @param active_frame_idx Which of the framebuffers is active
	 */
	void command_buffer_begun(CommandBuffer &cb, uint32_t active_frame_idx) override;

	/**
	 * @brief A command buffer that we want stats about is about to be ended
	 * @param cb The command buffer
	 * @param active_frame_idx Which of the framebuffers is active
	 */
	void command_buffer_ending(CommandBuffer &cb, uint32_t active_frame_idx) override;

  private:
	bool is_supported(const CounterSamplingConfig &sampling_config) const;

	bool fill_vendor_data();

	bool create_query_pools(uint32_t num_framebuffers, uint32_t queue_family_index);

	float get_best_delta_time(float sw_delta_time, uint32_t active_frame_idx) const;

  private:
	Device &device;

	// The query pool for the performance queries
	std::unique_ptr<QueryPool> query_pool;

	// Do we support timestamp queries
	bool has_timestamps{false};

	// The timestamp period
	float timestamp_period{1.0f};

	// And one for timestamps
	std::unique_ptr<QueryPool> timestamp_pool;

	// Map of vendor specific stat data
	VendorStatMap vendor_data;

	// Only stats which are available and were requested end up in stat_data
	StatDataMap stat_data;

	// An ordered list of the Vulkan counter ids
	std::vector<uint32_t> counter_indices;

	uint32_t queries_ready = 0;
};

}        // namespace vkb
