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
#include <array>
#include <random>

/**
 * @brief Demonstrates VK_KHR_device_address_commands.
 *
 * This extension enables Vulkan commands that accept raw VkDeviceAddress values in place
 * of VkBuffer handles, mirroring each classic bind/draw/dispatch command with a new
 * device-address variant:
 *
 *   vkCmdFillMemoryKHR          — zero a region of device memory by address
 *   vkCmdUpdateMemoryKHR        — write host data to device memory by address
 *   vkCmdBindIndexBuffer3KHR    — bind an index buffer by device address
 *   vkCmdBindVertexBuffers3KHR  — bind vertex buffers by device address
 *   vkCmdDrawIndexedIndirectCount2KHR — GPU-driven indirect draw with address-based count
 *   VkMemoryRangeBarrierKHR     — address-scoped memory barrier (extends VkDependencyInfo)
 *
 * The sample renders 256 animated octahedra arranged in a four-arm galaxy spiral.
 * A compute shader animates each object and writes VkDrawIndexedIndirectCommand records
 * into a GPU buffer entirely through device addresses and push constants — no descriptor
 * sets required.  A distance-based visibility test (also in the compute shader) sets
 * instanceCount=0 for far objects, while vkCmdDrawIndexedIndirectCount2KHR reads the
 * draw count from a device address previously written by vkCmdFillMemoryKHR.
 */
class DeviceAddressCommands : public ApiVulkanSample
{
  public:
	DeviceAddressCommands();
	~DeviceAddressCommands() override;

	bool prepare(const vkb::ApplicationOptions &options) override;
	void render(float delta_time) override;
	void build_command_buffers() override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;

  protected:
	uint32_t get_api_version() const override
	{
		return VK_API_VERSION_1_3;
	}
	void request_gpu_features(vkb::core::PhysicalDeviceC &gpu) override;

  private:
	// -------------------------------------------------------------------------
	// GPU buffer with device address
	// -------------------------------------------------------------------------
	struct GpuBuffer
	{
		VkBuffer        handle{VK_NULL_HANDLE};
		VkDeviceMemory  memory{VK_NULL_HANDLE};
		VkDeviceAddress address{0};
		VkDeviceSize    size{0};
	};

	GpuBuffer create_gpu_buffer(VkDeviceSize size, VkBufferUsageFlags usage);
	void      upload_buffer(GpuBuffer &dst, const void *data, VkDeviceSize bytes);
	void      destroy_gpu_buffer(GpuBuffer &buf);

	void create_geometry_buffers();
	void create_object_buffers();
	void create_compute_pipeline();
	void create_graphics_pipeline();
	void load_extension_functions();

	// -------------------------------------------------------------------------
	// Scene constants
	// -------------------------------------------------------------------------
	static constexpr uint32_t kObjectCount = 256;
	static constexpr uint32_t kIndexCount  = 24;        // 8 triangles × 3 vertices
	static constexpr uint32_t kVertexCount = 6;
	static constexpr float    kCamRadius   = 50.0f;
	static constexpr float    kCamHeight   = 15.0f;
	static constexpr float    kCamSpeed    = 0.08f;        // rad/s, slow orbit
	static constexpr float    kMaxVisDist  = 62.0f;        // visibility radius for culling

	// -------------------------------------------------------------------------
	// Push constants shared between CPU and shaders
	// -------------------------------------------------------------------------
	// Compute shader push constants (64 bytes).
	// GLSL std430 aligns vec3/float[3] to 16 bytes, so after draw_count_addr at
	// offset 32 (8 bytes), camera_pos lands at offset 48, not 40.
	// _align_pad makes the C++ layout match the shader layout explicitly.
	struct PushCompute
	{
		float    time;                     // 0
		float    max_vis_dist;             // 4
		uint64_t orbit_params_addr;        // 8
		uint64_t transforms_addr;          // 16
		uint64_t draw_cmds_addr;           // 24
		uint64_t draw_count_addr;          // 32  ← GPU atomically increments this
		float    _align_pad[2];            // 40  (padding to std430 vec3 alignment)
		float    camera_pos[3];            // 48
		float    _pad;                     // 60
	};
	static_assert(sizeof(PushCompute) == 64, "PushCompute size mismatch");

	// Vertex shader push constants (16 bytes)
	struct PushVertex
	{
		uint64_t camera_addr;            // 0  → mat4 view_projection
		uint64_t transforms_addr;        // 8  → mat4[256]
	};
	static_assert(sizeof(PushVertex) == 16, "PushVertex size mismatch");

	// -------------------------------------------------------------------------
	// GPU buffers
	// -------------------------------------------------------------------------
	GpuBuffer vertex_buffer;              // 6 octahedron vertices (vec3 each)
	GpuBuffer index_buffer;               // 24 uint16 indices
	GpuBuffer orbit_params_buffer;        // 256 × vec4 (radius, phase, speed, scale)
	GpuBuffer transforms_buffer;          // 256 × mat4, written by compute each frame
	GpuBuffer draw_cmds_buffer;           // 256 × VkDrawIndexedIndirectCommand
	GpuBuffer draw_count_buffer;          // uint32, zeroed by FillMemory + count by compute
	GpuBuffer camera_buffer;              // mat4 view_projection, updated by UpdateMemory

	// -------------------------------------------------------------------------
	// Pipelines
	// -------------------------------------------------------------------------
	VkPipelineLayout compute_pipeline_layout{VK_NULL_HANDLE};
	VkPipeline       compute_pipeline{VK_NULL_HANDLE};
	VkPipelineLayout graphics_pipeline_layout{VK_NULL_HANDLE};
	VkPipeline       graphics_pipeline{VK_NULL_HANDLE};

	// -------------------------------------------------------------------------
	// VK_KHR_device_address_commands function pointers (loaded at runtime)
	// -------------------------------------------------------------------------
	PFN_vkCmdFillMemoryKHR                vkCmdFillMemoryKHR{nullptr};
	PFN_vkCmdUpdateMemoryKHR              vkCmdUpdateMemoryKHR{nullptr};
	PFN_vkCmdBindIndexBuffer3KHR          vkCmdBindIndexBuffer3KHR{nullptr};
	PFN_vkCmdBindVertexBuffers3KHR        vkCmdBindVertexBuffers3KHR{nullptr};
	PFN_vkCmdDrawIndexedIndirectCount2KHR vkCmdDrawIndexedIndirectCount2KHR{nullptr};

	float accumulated_time{0.0f};
};

std::unique_ptr<vkb::VulkanSampleC> create_device_address_commands();
