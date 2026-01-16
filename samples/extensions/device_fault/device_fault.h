/* Copyright (c) 2026, Arm Limited and Contributors
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
#include <memory>
#include <vector>

class DeviceFault : public ApiVulkanSample
{
public:
    DeviceFault();
    ~DeviceFault();

private:
    virtual void request_gpu_features(vkb::core::PhysicalDeviceC &gpu) override;
    virtual void render(float delta_time) override;
    virtual void build_command_buffers() override;
    virtual void on_update_ui_overlay(vkb::Drawer &drawer) override;
    virtual bool prepare(const vkb::ApplicationOptions &options) override;

    void             create_pipelines();
    VkPipelineLayout create_pipeline_layout(bool graphics);
    void             create_compute_pipeline();
    void             create_graphics_pipeline();
    void             check_device_fault();
    void update_pointer_buffer(VkCommandBuffer cmd);
    void update_meshlets(VkCommandBuffer cmd);

    bool                     triggerDeviceFaultInvalidAddress = false;
    bool                     deviceBindingReportEnabled = true;

    VkDebugUtilsMessengerEXT debug_utils_messenger{VK_NULL_HANDLE};
    void handle_address_binding(const VkDeviceAddressBindingCallbackDataEXT& callbackData);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                         VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                         const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                         void* pUserData);

    struct Pipelines
    {
        VkPipelineLayout compute_pipeline_layout{};
        VkPipelineLayout graphics_pipeline_layout{};
        VkPipeline       bindless_vbo_pipeline{};
        VkPipeline       compute_update_pipeline{};
    } pipelines;

    struct TestBuffer
    {
        VkBuffer        buffer{};
        VkDeviceMemory  memory{};
        VkDeviceAddress gpu_address{};
    };

    std::vector<TestBuffer> test_buffers;
    void                    create_vbo_buffers();
    TestBuffer              create_vbo_buffer();
    TestBuffer              create_pointer_buffer();
    TestBuffer              pointer_buffer;

    // Simple tagging variable to follow memory address changes in pipeline
    std::string             current_memory_label;
    void                    set_memory_debug_label(std::string current_address_reporter);

    std::unique_ptr<vkb::core::BufferC> create_index_buffer();
    std::unique_ptr<vkb::core::BufferC> index_buffer;

    std::default_random_engine            rnd{42};
    std::uniform_real_distribution<float> distribution{0.0f, 0.1f};
    uint32_t                              descriptor_offset{};
    float                                 accumulated_time{};
    uint32_t                              num_indices_per_mesh{};
};

std::unique_ptr<ApiVulkanSample> create_device_fault();
