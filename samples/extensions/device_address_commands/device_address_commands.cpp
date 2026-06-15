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

#include "device_address_commands.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <random>

DeviceAddressCommands::DeviceAddressCommands()
{
	title = "Device address commands";

	// VK_KHR_device_address_commands provides address-based variants of
	// bind/draw/dispatch/fill/update commands.
	add_device_extension(VK_KHR_DEVICE_ADDRESS_COMMANDS_EXTENSION_NAME);

	// Buffer device address is core in Vulkan 1.2 and required by
	// VK_KHR_device_address_commands.  We also request the KHR extension
	// name so older loaders find it correctly.
	add_device_extension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, /*optional=*/true);
	add_device_extension(VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME);
}

DeviceAddressCommands::~DeviceAddressCommands()
{
	if (!has_device())
	{
		return;
	}
	VkDevice dev = get_device().get_handle();

	destroy_gpu_buffer(vertex_buffer);
	destroy_gpu_buffer(index_buffer);
	destroy_gpu_buffer(orbit_params_buffer);
	destroy_gpu_buffer(transforms_buffer);
	destroy_gpu_buffer(draw_cmds_buffer);
	destroy_gpu_buffer(draw_count_buffer);
	destroy_gpu_buffer(camera_buffer);

	vkDestroyPipeline(dev, compute_pipeline, nullptr);
	vkDestroyPipeline(dev, graphics_pipeline, nullptr);
	vkDestroyPipelineLayout(dev, compute_pipeline_layout, nullptr);
	vkDestroyPipelineLayout(dev, graphics_pipeline_layout, nullptr);
}

// ============================================================================
// ApiVulkanSample overrides
// ============================================================================

void DeviceAddressCommands::build_command_buffers()
{
	// Command buffers are recorded every frame in render(); nothing to do here.
}

bool DeviceAddressCommands::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	load_extension_functions();
	create_geometry_buffers();
	create_object_buffers();
	create_compute_pipeline();
	create_graphics_pipeline();

	prepared = true;
	return true;
}

void DeviceAddressCommands::request_gpu_features(vkb::core::PhysicalDeviceC &gpu)
{
	// Buffer device address — requested through VkPhysicalDeviceVulkan12Features
	// because we also request drawIndirectCount from the same struct.  The spec
	// forbids having both VkPhysicalDeviceVulkan12Features and the standalone
	// VkPhysicalDeviceBufferDeviceAddressFeatures in the pNext chain at once.
	REQUEST_REQUIRED_FEATURE(gpu, VkPhysicalDeviceBufferDeviceAddressFeatures, bufferDeviceAddress);

	// The new extension feature that unlocks vkCmdBindIndexBuffer3KHR etc.
	REQUEST_REQUIRED_FEATURE(gpu,
	                         VkPhysicalDeviceDeviceAddressCommandsFeaturesKHR,
	                         deviceAddressCommands);

	// vkCmdPipelineBarrier2 (used for VkMemoryRangeBarrierKHR) requires this.
	REQUEST_REQUIRED_FEATURE(gpu, VkPhysicalDeviceSynchronization2FeaturesKHR, synchronization2);


	// The compute shader sets firstInstance to the object index so the vertex
	// shader can index the transform array.  Non-zero firstInstance requires
	// this core feature (Vulkan 1.0, universally available on desktop).
	gpu.get_mutable_requested_features().drawIndirectFirstInstance = VK_TRUE;

	// vkCmdDrawIndexedIndirectCount2KHR is a multi-draw indirect variant;
	// multiDrawIndirect is required when maxDrawCount > 1.
	gpu.get_mutable_requested_features().multiDrawIndirect = VK_TRUE;
}

// ============================================================================
// Extension function loading
// ============================================================================

void DeviceAddressCommands::load_extension_functions()
{
	VkDevice dev = get_device().get_handle();

#define LOAD_PFN(name)                                                         \
	fn_##name = reinterpret_cast<PFN_##name>(vkGetDeviceProcAddr(dev, #name)); \
	if (!fn_##name)                                                            \
	{                                                                          \
		throw std::runtime_error("Failed to load " #name);                     \
	}

	LOAD_PFN(vkCmdFillMemoryKHR)
	LOAD_PFN(vkCmdUpdateMemoryKHR)
	LOAD_PFN(vkCmdBindIndexBuffer3KHR)
	LOAD_PFN(vkCmdBindVertexBuffers3KHR)
	LOAD_PFN(vkCmdDrawIndexedIndirectCount2KHR)

#undef LOAD_PFN
}

// ============================================================================
// GPU buffer helpers
// ============================================================================

DeviceAddressCommands::GpuBuffer
    DeviceAddressCommands::create_gpu_buffer(VkDeviceSize size, VkBufferUsageFlags usage)
{
	GpuBuffer buf;
	buf.size     = size;
	VkDevice dev = get_device().get_handle();

	// SHADER_DEVICE_ADDRESS_BIT is required to query a buffer's device address.
	VkBufferCreateInfo bci = vkb::initializers::buffer_create_info(
	    usage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, size);
	VK_CHECK(vkCreateBuffer(dev, &bci, nullptr, &buf.handle));

	VkMemoryRequirements req;
	vkGetBufferMemoryRequirements(dev, buf.handle, &req);

	// VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT is required to call
	// vkGetBufferDeviceAddress on memory bound to this buffer.
	VkMemoryAllocateFlagsInfo mafi{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};
	mafi.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

	VkMemoryAllocateInfo mai = vkb::initializers::memory_allocate_info();
	mai.pNext                = &mafi;
	mai.allocationSize       = req.size;
	mai.memoryTypeIndex      = get_device().get_gpu().get_memory_type(
        req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK(vkAllocateMemory(dev, &mai, nullptr, &buf.memory));
	VK_CHECK(vkBindBufferMemory(dev, buf.handle, buf.memory, 0));

	VkBufferDeviceAddressInfo bdai{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
	bdai.buffer = buf.handle;
	buf.address = vkGetBufferDeviceAddress(dev, &bdai);

	return buf;
}

void DeviceAddressCommands::upload_buffer(GpuBuffer &dst, const void *data, VkDeviceSize bytes)
{
	auto staging = vkb::core::BufferC::create_staging_buffer(get_device(), bytes, data);

	auto &cmd_pool = get_device().get_command_pool();
	auto  cmd      = cmd_pool.request_command_buffer();
	cmd->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VkBufferCopy region{0, 0, bytes};
	vkCmdCopyBuffer(cmd->get_handle(), staging.get_handle(), dst.handle, 1, &region);

	VkBufferMemoryBarrier barrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
	barrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.buffer              = dst.handle;
	barrier.offset              = 0;
	barrier.size                = VK_WHOLE_SIZE;
	vkCmdPipelineBarrier(cmd->get_handle(),
	                     VK_PIPELINE_STAGE_TRANSFER_BIT,
	                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
	                     0, 0, nullptr, 1, &barrier, 0, nullptr);

	cmd->end();

	auto &queue = get_device().get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);
	queue.submit(*cmd, VK_NULL_HANDLE);
	queue.wait_idle();
}

void DeviceAddressCommands::destroy_gpu_buffer(GpuBuffer &buf)
{
	VkDevice dev = get_device().get_handle();
	if (buf.handle != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(dev, buf.handle, nullptr);
	}
	if (buf.memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(dev, buf.memory, nullptr);
	}
	buf = {};
}

// ============================================================================
// Geometry setup
// ============================================================================

void DeviceAddressCommands::create_geometry_buffers()
{
	// Octahedron: 6 vertices at the ±axis unit positions.
	// Indices form 8 CCW triangles sharing these 6 vertices.
	static const glm::vec3 vertices[kVertexCount] = {
	    {1, 0, 0},         // 0 +X
	    {-1, 0, 0},        // 1 -X
	    {0, 1, 0},         // 2 +Y (top)
	    {0, -1, 0},        // 3 -Y (bottom)
	    {0, 0, 1},         // 4 +Z
	    {0, 0, -1},        // 5 -Z
	};

	// 8 faces: 4 top (+Y) + 4 bottom (-Y), CCW when viewed from outside.
	static const uint16_t indices[kIndexCount] = {
	    2,
	    4,
	    0,        // top  +X+Z
	    2,
	    1,
	    4,        // top  -X+Z
	    2,
	    5,
	    1,        // top  -X-Z
	    2,
	    0,
	    5,        // top  +X-Z
	    3,
	    0,
	    4,        // bot  +X+Z
	    3,
	    4,
	    1,        // bot  -X+Z
	    3,
	    1,
	    5,        // bot  -X-Z
	    3,
	    5,
	    0,        // bot  +X-Z
	};

	vertex_buffer = create_gpu_buffer(sizeof(vertices),
	                                  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
	                                      VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	upload_buffer(vertex_buffer, vertices, sizeof(vertices));

	index_buffer = create_gpu_buffer(sizeof(indices),
	                                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
	                                     VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	upload_buffer(index_buffer, indices, sizeof(indices));
}

// ============================================================================
// Scene object setup
// ============================================================================

void DeviceAddressCommands::create_object_buffers()
{
	// Generate spiral-galaxy orbit parameters: 4 arms × 64 objects each.
	std::mt19937                          rng(42);
	std::uniform_real_distribution<float> scale_dist(0.25f, 0.55f);
	std::uniform_real_distribution<float> jitter(-0.25f, 0.25f);

	std::vector<glm::vec4> params(kObjectCount);
	for (uint32_t i = 0; i < kObjectCount; i++)
	{
		uint32_t arm       = i % 4;
		uint32_t arm_index = i / 4;                           // 0..63
		float    t         = float(arm_index) / 63.0f;        // 0→1

		float radius  = 4.0f + 18.0f * t;        // 4→22
		float arm_off = float(arm) * glm::half_pi<float>();
		float phase   = arm_off + t * 2.5f * glm::pi<float>() + jitter(rng);
		float speed   = 0.70f * (1.0f - 0.65f * t);        // inner orbits faster
		float scale   = scale_dist(rng);

		params[i] = glm::vec4(radius, phase, speed, scale);
	}

	orbit_params_buffer = create_gpu_buffer(kObjectCount * sizeof(glm::vec4),
	                                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
	                                            VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	upload_buffer(orbit_params_buffer, params.data(), kObjectCount * sizeof(glm::vec4));

	// transforms: written by compute shader, read by vertex shader
	transforms_buffer = create_gpu_buffer(kObjectCount * sizeof(glm::mat4),
	                                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

	// draw_cmds: written by compute, read as indirect commands
	draw_cmds_buffer = create_gpu_buffer(kObjectCount * sizeof(VkDrawIndexedIndirectCommand),
	                                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
	                                         VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);

	// draw_count: zeroed each frame by vkCmdFillMemoryKHR, incremented by compute
	draw_count_buffer = create_gpu_buffer(sizeof(uint32_t),
	                                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
	                                          VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
	                                          VK_BUFFER_USAGE_TRANSFER_DST_BIT);

	// camera: VP matrix uploaded each frame by vkCmdUpdateMemoryKHR
	camera_buffer = create_gpu_buffer(sizeof(glm::mat4),
	                                  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
	                                      VK_BUFFER_USAGE_TRANSFER_DST_BIT);
}

// ============================================================================
// Pipeline creation
// ============================================================================

void DeviceAddressCommands::create_compute_pipeline()
{
	VkPushConstantRange pc_range{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushCompute)};

	VkPipelineLayoutCreateInfo layout_ci = vkb::initializers::pipeline_layout_create_info(nullptr, 0);
	layout_ci.pushConstantRangeCount     = 1;
	layout_ci.pPushConstantRanges        = &pc_range;
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &layout_ci, nullptr,
	                                &compute_pipeline_layout));

	VkComputePipelineCreateInfo ci =
	    vkb::initializers::compute_pipeline_create_info(compute_pipeline_layout);
	ci.stage = load_shader("device_address_commands", "update_objects.comp.spv",
	                       VK_SHADER_STAGE_COMPUTE_BIT);
	VK_CHECK(vkCreateComputePipelines(get_device().get_handle(), VK_NULL_HANDLE,
	                                  1, &ci, nullptr, &compute_pipeline));
}

void DeviceAddressCommands::create_graphics_pipeline()
{
	VkPushConstantRange pc_range{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushVertex)};

	VkPipelineLayoutCreateInfo layout_ci = vkb::initializers::pipeline_layout_create_info(nullptr, 0);
	layout_ci.pushConstantRangeCount     = 1;
	layout_ci.pPushConstantRanges        = &pc_range;
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &layout_ci, nullptr,
	                                &graphics_pipeline_layout));

	// Vertex input: position (vec3, binding 0).
	// vkCmdBindVertexBuffers3KHR supplies the actual buffer address; the pipeline
	// only needs to know the attribute format and stride.
	VkVertexInputBindingDescription   vib{0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX};
	VkVertexInputAttributeDescription via{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0};

	VkPipelineVertexInputStateCreateInfo vis = vkb::initializers::pipeline_vertex_input_state_create_info();
	vis.vertexBindingDescriptionCount        = 1;
	vis.pVertexBindingDescriptions           = &vib;
	vis.vertexAttributeDescriptionCount      = 1;
	vis.pVertexAttributeDescriptions         = &via;

	VkPipelineInputAssemblyStateCreateInfo ias =
	    vkb::initializers::pipeline_input_assembly_state_create_info(
	        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

	VkPipelineRasterizationStateCreateInfo rast =
	    vkb::initializers::pipeline_rasterization_state_create_info(
	        VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);

	VkPipelineColorBlendAttachmentState blend_att =
	    vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE);
	VkPipelineColorBlendStateCreateInfo blend =
	    vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_att);

	VkPipelineDepthStencilStateCreateInfo ds =
	    vkb::initializers::pipeline_depth_stencil_state_create_info(
	        VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);

	VkPipelineViewportStateCreateInfo vps =
	    vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);

	VkPipelineMultisampleStateCreateInfo ms =
	    vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);

	std::vector<VkDynamicState>      dyn_states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dyn        = vkb::initializers::pipeline_dynamic_state_create_info(dyn_states);

	VkGraphicsPipelineCreateInfo ci = vkb::initializers::pipeline_create_info(graphics_pipeline_layout, render_pass);

	std::array<VkPipelineShaderStageCreateInfo, 2> stages;
	stages[0] = load_shader("device_address_commands", "render.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	stages[1] = load_shader("device_address_commands", "render.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	ci.pVertexInputState   = &vis;
	ci.pInputAssemblyState = &ias;
	ci.pRasterizationState = &rast;
	ci.pColorBlendState    = &blend;
	ci.pDepthStencilState  = &ds;
	ci.pViewportState      = &vps;
	ci.pMultisampleState   = &ms;
	ci.pDynamicState       = &dyn;
	ci.pStages             = stages.data();
	ci.stageCount          = static_cast<uint32_t>(stages.size());

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), VK_NULL_HANDLE,
	                                   1, &ci, nullptr, &graphics_pipeline));
}

// ============================================================================
// Per-frame rendering
// ============================================================================

void DeviceAddressCommands::render(float delta_time)
{
	ApiVulkanSample::prepare_frame();

	VK_CHECK(vkWaitForFences(get_device().get_handle(), 1,
	                         &wait_fences[current_buffer], VK_TRUE, UINT64_MAX));
	VK_CHECK(vkResetFences(get_device().get_handle(), 1, &wait_fences[current_buffer]));

	accumulated_time += delta_time;

	// Slowly orbit the camera around the galaxy for a dynamic culling effect.
	float     cam_angle  = accumulated_time * kCamSpeed;
	glm::vec3 camera_pos = glm::vec3(
	    std::cos(cam_angle) * kCamRadius,
	    kCamHeight,
	    std::sin(cam_angle) * kCamRadius);
	glm::mat4 view = glm::lookAt(camera_pos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	glm::mat4 proj = glm::perspective(
	    glm::radians(50.0f),
	    static_cast<float>(width) / static_cast<float>(height),
	    0.5f, 300.0f);
	proj[1][1] *= -1.0f;        // Vulkan clip-space has Y pointing down
	glm::mat4 vp = proj * view;

	recreate_current_command_buffer();
	VkCommandBuffer cmd = draw_cmd_buffers[current_buffer];

	auto begin_info  = vkb::initializers::command_buffer_begin_info();
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VK_CHECK(vkBeginCommandBuffer(cmd, &begin_info));

	// ===========================================================================
	// [1] vkCmdUpdateMemoryKHR — write the view-projection matrix directly to a
	//     device-local buffer by its device address.  The vertex shader reads it
	//     back through a buffer_reference pointer (no descriptor sets required).
	//     This is the address-based replacement for vkCmdUpdateBuffer.
	// ===========================================================================
	{
		VkDeviceAddressRangeKHR cam_range{camera_buffer.address, sizeof(glm::mat4)};
		fn_vkCmdUpdateMemoryKHR(cmd, &cam_range,
		                        VK_ADDRESS_COMMAND_STORAGE_BUFFER_USAGE_BIT_KHR,
		                        sizeof(glm::mat4), glm::value_ptr(vp));
	}

	// ===========================================================================
	// [2] vkCmdFillMemoryKHR — zero the GPU draw-count at its device address.
	//     This is the address-based replacement for vkCmdFillBuffer.
	//     The compute shader will atomically increment this value for each
	//     visible object; vkCmdDrawIndexedIndirectCount2KHR reads the final count.
	// ===========================================================================
	{
		VkDeviceAddressRangeKHR count_range{draw_count_buffer.address, sizeof(uint32_t)};
		fn_vkCmdFillMemoryKHR(cmd, &count_range,
		                      VK_ADDRESS_COMMAND_STORAGE_BUFFER_USAGE_BIT_KHR, 0u);
	}

	// ===========================================================================
	// [3] VkMemoryRangeBarrierKHR (pre-compute)
	//     Scope the two "transfer" writes above to specific address ranges before
	//     the compute shader and draw command consume them.
	//
	//     Unlike VkBufferMemoryBarrier2 (which requires a VkBuffer handle and
	//     therefore knows the buffer's declared usage flags), VkMemoryRangeBarrierKHR
	//     operates on raw [address, size] ranges and is therefore usable when the
	//     caller holds only a device address.
	// ===========================================================================
	{
		std::array<VkMemoryRangeBarrierKHR, 2> pre_barriers{};

		// camera VP: TRANSFER_WRITE → VERTEX_SHADER_READ
		pre_barriers[0].sType               = VK_STRUCTURE_TYPE_MEMORY_RANGE_BARRIER_KHR;
		pre_barriers[0].srcStageMask        = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		pre_barriers[0].srcAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		pre_barriers[0].dstStageMask        = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
		pre_barriers[0].dstAccessMask       = VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
		pre_barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		pre_barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		pre_barriers[0].addressRange        = {camera_buffer.address, camera_buffer.size};
		pre_barriers[0].addressFlags        = VK_ADDRESS_COMMAND_STORAGE_BUFFER_USAGE_BIT_KHR;

		// draw count: TRANSFER_WRITE → COMPUTE_SHADER (atomicAdd)
		pre_barriers[1].sType         = VK_STRUCTURE_TYPE_MEMORY_RANGE_BARRIER_KHR;
		pre_barriers[1].srcStageMask  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		pre_barriers[1].srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		pre_barriers[1].dstStageMask  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		pre_barriers[1].dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT |
		                                VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
		pre_barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		pre_barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		pre_barriers[1].addressRange        = {draw_count_buffer.address, draw_count_buffer.size};
		pre_barriers[1].addressFlags        = VK_ADDRESS_COMMAND_STORAGE_BUFFER_USAGE_BIT_KHR;

		VkMemoryRangeBarriersInfoKHR mri{VK_STRUCTURE_TYPE_MEMORY_RANGE_BARRIERS_INFO_KHR};
		mri.memoryRangeBarrierCount = static_cast<uint32_t>(pre_barriers.size());
		mri.pMemoryRangeBarriers    = pre_barriers.data();

		VkDependencyInfo dep{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
		dep.pNext = &mri;
		vkCmdPipelineBarrier2(cmd, &dep);
	}

	// ===========================================================================
	// [4] Compute pass — animate objects and build draw commands entirely on GPU.
	//     Every buffer is accessed through device addresses in push constants;
	//     no descriptor sets are allocated or bound for this pass.
	// ===========================================================================
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline);

	PushCompute pc{};
	pc.time              = accumulated_time;
	pc.max_vis_dist      = kMaxVisDist;
	pc.orbit_params_addr = orbit_params_buffer.address;
	pc.transforms_addr   = transforms_buffer.address;
	pc.draw_cmds_addr    = draw_cmds_buffer.address;
	pc.draw_count_addr   = draw_count_buffer.address;
	pc.camera_pos[0]     = camera_pos.x;
	pc.camera_pos[1]     = camera_pos.y;
	pc.camera_pos[2]     = camera_pos.z;

	vkCmdPushConstants(cmd, compute_pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT,
	                   0, sizeof(pc), &pc);

	// 4 groups × 64 threads = 256 invocations, one per object.
	vkCmdDispatch(cmd, kObjectCount / 64, 1, 1);

	// ===========================================================================
	// [5] VkMemoryRangeBarrierKHR (post-compute)
	//     Three range barriers in one call: transforms, draw commands, and draw
	//     count must all be visible before the graphics pass consumes them.
	// ===========================================================================
	{
		std::array<VkMemoryRangeBarrierKHR, 3> post_barriers{};

		// transforms: COMPUTE_WRITE → VERTEX_SHADER_READ
		post_barriers[0].sType               = VK_STRUCTURE_TYPE_MEMORY_RANGE_BARRIER_KHR;
		post_barriers[0].srcStageMask        = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		post_barriers[0].srcAccessMask       = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
		post_barriers[0].dstStageMask        = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
		post_barriers[0].dstAccessMask       = VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
		post_barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		post_barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		post_barriers[0].addressRange        = {transforms_buffer.address, transforms_buffer.size};
		post_barriers[0].addressFlags        = VK_ADDRESS_COMMAND_STORAGE_BUFFER_USAGE_BIT_KHR;

		// draw commands: COMPUTE_WRITE → INDIRECT_COMMAND_READ
		post_barriers[1].sType               = VK_STRUCTURE_TYPE_MEMORY_RANGE_BARRIER_KHR;
		post_barriers[1].srcStageMask        = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		post_barriers[1].srcAccessMask       = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
		post_barriers[1].dstStageMask        = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
		post_barriers[1].dstAccessMask       = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
		post_barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		post_barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		post_barriers[1].addressRange        = {draw_cmds_buffer.address, draw_cmds_buffer.size};
		post_barriers[1].addressFlags        = VK_ADDRESS_COMMAND_STORAGE_BUFFER_USAGE_BIT_KHR;

		// draw count: COMPUTE_WRITE → INDIRECT_COMMAND_READ
		post_barriers[2].sType               = VK_STRUCTURE_TYPE_MEMORY_RANGE_BARRIER_KHR;
		post_barriers[2].srcStageMask        = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		post_barriers[2].srcAccessMask       = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
		post_barriers[2].dstStageMask        = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
		post_barriers[2].dstAccessMask       = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
		post_barriers[2].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		post_barriers[2].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		post_barriers[2].addressRange        = {draw_count_buffer.address, draw_count_buffer.size};
		post_barriers[2].addressFlags        = VK_ADDRESS_COMMAND_STORAGE_BUFFER_USAGE_BIT_KHR;

		VkMemoryRangeBarriersInfoKHR mri{VK_STRUCTURE_TYPE_MEMORY_RANGE_BARRIERS_INFO_KHR};
		mri.memoryRangeBarrierCount = static_cast<uint32_t>(post_barriers.size());
		mri.pMemoryRangeBarriers    = post_barriers.data();

		VkDependencyInfo dep{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
		dep.pNext = &mri;
		vkCmdPipelineBarrier2(cmd, &dep);
	}

	// ===========================================================================
	// [6] Graphics pass
	// ===========================================================================
	VkRenderPassBeginInfo rp_begin    = vkb::initializers::render_pass_begin_info();
	rp_begin.renderPass               = render_pass;
	rp_begin.framebuffer              = framebuffers[current_buffer];
	rp_begin.renderArea.extent.width  = width;
	rp_begin.renderArea.extent.height = height;
	rp_begin.clearValueCount          = 2;
	std::array<VkClearValue, 2> clears{};
	clears[0].color        = {{0.04f, 0.02f, 0.12f, 1.0f}};        // deep-space dark blue
	clears[1].depthStencil = {1.0f, 0};
	rp_begin.pClearValues  = clears.data();
	vkCmdBeginRenderPass(cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{0, 0, static_cast<float>(width), static_cast<float>(height), 0, 1};
	VkRect2D   scissor{{0, 0}, {width, height}};
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

	PushVertex pv{};
	pv.camera_addr     = camera_buffer.address;
	pv.transforms_addr = transforms_buffer.address;
	vkCmdPushConstants(cmd, graphics_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT,
	                   0, sizeof(pv), &pv);

	// ===========================================================================
	// [7] vkCmdBindIndexBuffer3KHR — bind the index buffer by its device address.
	//     This is the address-based replacement for vkCmdBindIndexBuffer.
	// ===========================================================================
	{
		VkBindIndexBuffer3InfoKHR idx_info{VK_STRUCTURE_TYPE_BIND_INDEX_BUFFER_3_INFO_KHR};
		idx_info.addressRange = {index_buffer.address, index_buffer.size};
		idx_info.addressFlags = 0;
		idx_info.indexType    = VK_INDEX_TYPE_UINT16;
		fn_vkCmdBindIndexBuffer3KHR(cmd, &idx_info);
	}

	// ===========================================================================
	// [8] vkCmdBindVertexBuffers3KHR — bind the vertex buffer by device address.
	//     This is the address-based replacement for vkCmdBindVertexBuffers.
	//     setStride=VK_FALSE means the stride comes from the pipeline state.
	// ===========================================================================
	{
		VkBindVertexBuffer3InfoKHR vtx_info{VK_STRUCTURE_TYPE_BIND_VERTEX_BUFFER_3_INFO_KHR};
		// setStride=VK_FALSE: stride comes from the pipeline vertex binding (sizeof vec3).
		// The stride field in the range must be 0 when setStride is VK_FALSE.
		vtx_info.setStride    = VK_FALSE;
		vtx_info.addressRange = {vertex_buffer.address, vertex_buffer.size, 0};
		vtx_info.addressFlags = 0;
		fn_vkCmdBindVertexBuffers3KHR(cmd, 0, 1, &vtx_info);
	}

	// ===========================================================================
	// [9] vkCmdDrawIndexedIndirectCount2KHR — GPU-driven indexed draw.
	//     Both the draw-command array and the draw count are specified by device
	//     address; neither requires a VkBuffer handle.
	//     This is the address-based replacement for vkCmdDrawIndexedIndirectCount.
	// ===========================================================================
	{
		VkDrawIndirectCount2InfoKHR draw_info{VK_STRUCTURE_TYPE_DRAW_INDIRECT_COUNT_2_INFO_KHR};
		draw_info.addressRange = {
		    draw_cmds_buffer.address,
		    draw_cmds_buffer.size,
		    sizeof(VkDrawIndexedIndirectCommand)};
		draw_info.addressFlags      = VK_ADDRESS_COMMAND_STORAGE_BUFFER_USAGE_BIT_KHR;
		draw_info.countAddressRange = {draw_count_buffer.address, sizeof(uint32_t)};
		draw_info.countAddressFlags = VK_ADDRESS_COMMAND_STORAGE_BUFFER_USAGE_BIT_KHR;
		draw_info.maxDrawCount      = kObjectCount;
		fn_vkCmdDrawIndexedIndirectCount2KHR(cmd, &draw_info);
	}

	draw_ui(cmd);
	vkCmdEndRenderPass(cmd);
	VK_CHECK(vkEndCommandBuffer(cmd));

	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, wait_fences[current_buffer]));

	ApiVulkanSample::submit_frame();
}

void DeviceAddressCommands::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("VK_KHR_device_address_commands"))
	{
		drawer.text("Objects: %u", kObjectCount);
		drawer.text("Max vis. dist: %.0f units", kMaxVisDist);
		drawer.text("");
		drawer.text("Commands used this frame:");
		drawer.text("  vkCmdUpdateMemoryKHR  (VP matrix)");
		drawer.text("  vkCmdFillMemoryKHR    (draw count)");
		drawer.text("  VkMemoryRangeBarrierKHR x5");
		drawer.text("  vkCmdBindIndexBuffer3KHR");
		drawer.text("  vkCmdBindVertexBuffers3KHR");
		drawer.text("  vkCmdDrawIndexedIndirectCount2KHR");
	}
}

std::unique_ptr<vkb::VulkanSampleC> create_device_address_commands()
{
	return std::make_unique<DeviceAddressCommands>();
}
