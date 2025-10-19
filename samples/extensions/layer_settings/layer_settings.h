/* Copyright (c) 2025, Holochip Inc.
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

// A minimal sample demonstrating VK_EXT_layer_settings usage within this framework.
// It configures the Khronos validation layer using add_layer_setting() before the Vulkan
// instance is created, and then runs a no-op frame loop with correct WSI sync.
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
	// Records a per-frame minimal command buffer that transitions the acquired
	// swapchain image to PRESENT so we can present without validation errors,
	// and draws the UI to show validation messages.
	void record_minimal_present_cmd(VkCommandBuffer cmd, uint32_t image_index);

	// Aggregated validation output for GUI.
	std::string log_text_;

	// Local debug messenger to capture Validation Layer messages at runtime.
	VkDebugUtilsMessengerEXT debug_messenger_ = VK_NULL_HANDLE;

	// Debug messenger callback.
	static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
	    VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
	    VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
	    const VkDebugUtilsMessengerCallbackDataEXT      *pCallbackData,
	    void                                            *pUserData);
};

std::unique_ptr<vkb::Application> create_layer_settings();
