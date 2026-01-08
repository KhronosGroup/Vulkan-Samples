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
#include <deque>
#include <string>

class ShaderRelaxedExtendedInstruction : public ApiVulkanSample
{
  public:
	ShaderRelaxedExtendedInstruction();
	~ShaderRelaxedExtendedInstruction() override;

	void build_command_buffers() override;        // Not used; per-frame recording in render()
	void request_gpu_features(vkb::core::PhysicalDeviceC &gpu) override;
	bool prepare(const vkb::ApplicationOptions &options) override;
	void render(float delta_time) override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;
	void append_message(const char *msg);

	// Override instance creation to enable debugPrintf via layer settings or validation features fallback
	std::unique_ptr<vkb::core::InstanceC> create_instance() override;

  private:
	void record_minimal_present_cmd(VkCommandBuffer cmd) const;

	// Minimal compute pipeline used to demonstrate non-semantic extended instructions
	VkPipeline       compute_pipeline{VK_NULL_HANDLE};
	VkPipelineLayout pipeline_layout{VK_NULL_HANDLE};

	// Debug utils messenger to receive INFO-severity debugPrintf messages
	VkDebugUtilsMessengerEXT debug_utils_messenger{VK_NULL_HANDLE};

	// UI and message state
	uint32_t                ui_value_              = 0;
	uint32_t                last_dispatched_value_ = 0xFFFFFFFFu;
	bool                    request_dispatch_once_ = false;
	std::deque<std::string> last_messages_;
	static constexpr size_t kMaxMessages_ = 3;
};

std::unique_ptr<vkb::Application> create_shader_relaxed_extended_instruction();
