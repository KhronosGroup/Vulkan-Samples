/* Copyright (c) 2026, Holochip Inc.
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

#include "api_vulkan_sample.h"
#include <functional>
#include <unordered_map>

// Interactive sample demonstrating VK_EXT_layer_settings usage with multiple
// toggleable validation scenarios that trigger Best Practices warnings.
// Each scenario demonstrates a common real-world mistake and displays validation
// messages in the UI with visual feedback.
class LayerSettingsSample : public ApiVulkanSample
{
  public:
	LayerSettingsSample();
	~LayerSettingsSample() override;

	// Install the DebugUtils messenger as early as possible by overriding instance creation.
	std::unique_ptr<vkb::core::InstanceC> create_instance() override;

	bool prepare(const vkb::ApplicationOptions &options) override;
	void render(float delta_time) override;
	void build_command_buffers() override
	{}
	virtual void on_update_ui_overlay(vkb::Drawer &drawer) override;

  private:
	// Validation scenario identifiers
	enum class Scenario
	{
		WrongBufferFlags,
		SuboptimalTransitions,
		SmallAllocations
	};

	// Per-scenario state and statistics
	struct ScenarioState
	{
		bool        enabled       = false;
		uint32_t    warning_count = 0;
		uint32_t    error_count   = 0;
		std::string recent_messages;
	};

	// Records a per-frame command buffer with validation scenarios
	void record_minimal_present_cmd(VkCommandBuffer cmd, uint32_t image_index);

	// Helper to execute one-time commands
	void execute_one_time_commands(std::function<void(VkCommandBuffer)> commands);

	// Setup functions for each scenario
	void setup_wrong_buffer_flags_scenario();
	void setup_suboptimal_transitions_scenario();
	void setup_small_allocations_scenario();

	// Cleanup functions for each scenario
	void cleanup_wrong_buffer_flags_scenario();
	void cleanup_suboptimal_transitions_scenario();
	void cleanup_small_allocations_scenario();
	void cleanup_scenarios();

	// Render visual feedback for active scenarios
	void render_scenario_visuals(VkCommandBuffer cmd);

	// Aggregated validation output for GUI
	std::string log_text_;

	// Per-scenario state tracking
	struct ScenarioHash
	{
		size_t operator()(Scenario s) const noexcept
		{
			return static_cast<size_t>(s);
		}
	};
	std::unordered_map<Scenario, ScenarioState, ScenarioHash> scenario_states_;

	// Resources for visual rendering
	struct
	{
		VkBuffer                    vertex_buffer        = VK_NULL_HANDLE;
		VkDeviceMemory              vertex_buffer_memory = VK_NULL_HANDLE;
		VkBuffer                    wrong_usage_buffer   = VK_NULL_HANDLE;
		VkDeviceMemory              wrong_usage_memory   = VK_NULL_HANDLE;
		VkImage                     test_image           = VK_NULL_HANDLE;
		VkDeviceMemory              test_image_memory    = VK_NULL_HANDLE;
		VkImageView                 test_image_view      = VK_NULL_HANDLE;
		VkPipeline                  simple_pipeline      = VK_NULL_HANDLE;
		VkPipelineLayout            pipeline_layout      = VK_NULL_HANDLE;
		std::vector<VkBuffer>       small_buffers;
		std::vector<VkDeviceMemory> small_allocations;
	} resources_;

	VkDebugUtilsMessengerEXT debug_messenger_ = VK_NULL_HANDLE;

	// Debug messenger callback
	static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
	    VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
	    VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
	    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
	    void                                       *pUserData);
};

std::unique_ptr<vkb::Application> create_layer_settings();
