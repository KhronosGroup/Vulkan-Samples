/* Copyright (c) 2025, Arm Limited and Contributors
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

#include "device_fault.h"

DeviceFault::DeviceFault()
{
    title = "Device Fault";

    // Need to enable buffer device address extension.
    add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    add_device_extension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);

    // Provides support for VkAllocateMemoryFlagsInfo. Otherwise, core in Vulkan 1.1.
    add_device_extension(VK_KHR_DEVICE_GROUP_EXTENSION_NAME);
    // Required by VK_KHR_device_group.
    add_instance_extension(VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME);

    // Debug utils extension
    add_instance_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    // Provides additional info when VK_DEVICE_LOST error happens.
    add_device_extension(VK_EXT_DEVICE_FAULT_EXTENSION_NAME);

    // Gives more information about GPU Virtual Address space
    add_device_extension(VK_EXT_DEVICE_ADDRESS_BINDING_REPORT_EXTENSION_NAME, true);
}

DeviceFault::~DeviceFault()
{
    if (has_device())
    {
        VkDevice vk_device = get_device().get_handle();
        vkDestroyPipelineLayout(vk_device, pipelines.compute_pipeline_layout, nullptr);
        vkDestroyPipelineLayout(vk_device, pipelines.graphics_pipeline_layout, nullptr);
        vkDestroyPipeline(vk_device, pipelines.bindless_vbo_pipeline, nullptr);
        vkDestroyPipeline(vk_device, pipelines.compute_update_pipeline, nullptr);

        for (auto &buffer : test_buffers)
        {
            vkDestroyBuffer(vk_device, buffer.buffer, nullptr);
            vkFreeMemory(vk_device, buffer.memory, nullptr);
        }
        vkDestroyBuffer(vk_device, pointer_buffer.buffer, nullptr);
        vkFreeMemory(vk_device, pointer_buffer.memory, nullptr);
        vkDestroyDebugUtilsMessengerEXT(get_instance().get_handle(), debug_utils_messenger, nullptr);
    }
}


void DeviceFault::build_command_buffers()
{
}

void DeviceFault::on_update_ui_overlay(vkb::Drawer &drawer)
{
    if (drawer.header("Settings"))
    {
        if (drawer.button("Trigger Device Fault with Invalid Address"))
        {
            triggerDeviceFaultInvalidAddress = true;
        }
    }
}

void DeviceFault::check_device_fault(VkResult result)
{
    VkDevice vk_device = get_device().get_handle();

    if (result != VK_ERROR_DEVICE_LOST) {
        return;
    }

    // First query just the counts
    VkDeviceFaultCountsEXT faultCount = { VK_STRUCTURE_TYPE_DEVICE_FAULT_COUNTS_EXT };
    VkResult countResult = vkGetDeviceFaultInfoEXT(vk_device, &faultCount, nullptr);

    if (countResult != VK_SUCCESS && countResult != VK_ERROR_DEVICE_LOST) {
        LOGE("Failed to query fault counts: ", std::to_string(countResult));
        return;
    }

    // Only allocate and query full fault info if we have faults
    if (faultCount.addressInfoCount > 0 || faultCount.vendorInfoCount > 0) {
        LOGE("Device Fault encountered.");

        std::vector<VkDeviceFaultAddressInfoEXT> addressInfos(faultCount.addressInfoCount);
        std::vector<VkDeviceFaultVendorInfoEXT> vendorInfos(faultCount.vendorInfoCount);
        std::vector<char> vendorBinaryData(faultCount.vendorBinarySize);

        VkDeviceFaultInfoEXT faultInfo = { VK_STRUCTURE_TYPE_DEVICE_FAULT_INFO_EXT };

        faultInfo.pAddressInfos = addressInfos.data();
        faultInfo.pVendorInfos = vendorInfos.data();
        faultInfo.pVendorBinaryData = vendorBinaryData.data();

        VkResult faultResult = vkGetDeviceFaultInfoEXT(vk_device, &faultCount, &faultInfo);

        // Log the description and address info if it was able to catch the fault.
        if (faultResult == VK_SUCCESS) {

            // Some vendors may provide additional information
            LOGE("Vendor Fault Description: {}", faultInfo.pVendorInfos ? faultInfo.pVendorInfos->description : "No Vendor Information available.")
            // Log each address info
            for (uint32_t i = 0; i < faultCount.addressInfoCount; i++) {
                LOGE("Fault Address Info Address Type: {}", std::to_string(addressInfos[i].addressType));
				LOGE("Fault Address Info Reported Address -> Decimal: {} | Hex: 0x{:X}", addressInfos[i].reportedAddress, static_cast<uint64_t>(addressInfos[i].reportedAddress));
            }
        }
    } else {
        LOGI("No device faults detected.");
    }
}

void DeviceFault::handle_address_binding(const VkDeviceAddressBindingCallbackDataEXT& callbackData)
{
    if (!deviceBindingReportEnabled) {
        return;
    }

    // Report current memory operations with a label and an address
    switch (callbackData.bindingType) {
        case VK_DEVICE_ADDRESS_BINDING_TYPE_BIND_EXT:
			LOGI("{} : Address Bound -> Decimal: {} | Hex: 0x{:X} | Size = {} Bytes", current_memory_label, std::to_string(callbackData.baseAddress), static_cast<uint64_t>(callbackData.baseAddress), callbackData.size);
            break;

        case VK_DEVICE_ADDRESS_BINDING_TYPE_UNBIND_EXT:
			LOGI("{} : Address Unbound -> Decimal: {} | Hex: 0x{:X} | Size = {} Bytes", current_memory_label, std::to_string(callbackData.baseAddress), static_cast<uint64_t>(callbackData.baseAddress), callbackData.size);
            break;

        default:
            LOGE("No address binding/unbinding information!");
            break;
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL DeviceFault::debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                           VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                           const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                           void* pUserData)
{
    // Check if this is an address binding callback
    auto* bindingData = static_cast<const VkDeviceAddressBindingCallbackDataEXT*>(pCallbackData->pNext);
    if (bindingData && bindingData->sType == VK_STRUCTURE_TYPE_DEVICE_ADDRESS_BINDING_CALLBACK_DATA_EXT)
    {
        auto* app = static_cast<DeviceFault*>(pUserData);
        app->handle_address_binding(*bindingData);
    }
    return VK_FALSE;
}

bool DeviceFault::prepare(const vkb::ApplicationOptions &options)
{
    if (!ApiVulkanSample::prepare(options))
    {
        return false;
    }

    // Set up debug utils messenger with proper user data pointer
    VkDebugUtilsMessengerCreateInfoEXT debug_utils_create_info{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    //debug_utils_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
   //                                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
   //                                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
    //debug_utils_create_info.messageType =  VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
    //                                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
    //                                       VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;

    debug_utils_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    debug_utils_create_info.messageType =  VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;

    debug_utils_create_info.pfnUserCallback = debug_callback;
    debug_utils_create_info.pUserData = this; // Pass 'this' pointer to access instance methods

    vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(get_instance().get_handle(), "vkCreateDebugUtilsMessengerEXT"));

    if (!vkCreateDebugUtilsMessengerEXT)
    {
        LOGE("Failed to get vkCreateDebugUtilsMessengerEXT function pointer");
        return false;
    }

    VkResult res = (vkCreateDebugUtilsMessengerEXT(
            get_instance().get_handle(),
            &debug_utils_create_info,
            nullptr,
            &debug_utils_messenger));

    create_vbo_buffers();
    index_buffer = create_index_buffer();

    create_pipelines();

    prepared = true;
    return true;
}

struct PushCompute
{
    // This type is 8 bytes, and maps to a buffer_reference in Vulkan GLSL.
    VkDeviceAddress table;
    float           fract_time;
    VkBool32        trigger_device_fault_invalid_address;
};

struct PushVertex
{
    glm::mat4       view_projection;
    VkDeviceAddress table;
};

VkPipelineLayout DeviceFault::create_pipeline_layout(bool graphics)
{
    // For simplicity, we avoid any use of descriptor sets here.
    // We can just push a single pointer instead, which references all the buffers we need to work with.
    VkPipelineLayout layout{};

    VkPipelineLayoutCreateInfo layout_create_info = vkb::initializers::pipeline_layout_create_info(nullptr, 0);

    const std::vector<VkPushConstantRange> ranges = {
            vkb::initializers::push_constant_range(graphics ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_COMPUTE_BIT,
                                                   graphics ? sizeof(PushVertex) : sizeof(PushCompute), 0),
    };
    layout_create_info.pushConstantRangeCount = static_cast<uint32_t>(ranges.size());
    layout_create_info.pPushConstantRanges    = ranges.data();
    VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &layout_create_info, nullptr, &layout));
    return layout;
}

void DeviceFault::create_compute_pipeline()
{
    pipelines.compute_pipeline_layout = create_pipeline_layout(false);
    VkComputePipelineCreateInfo info  = vkb::initializers::compute_pipeline_create_info(pipelines.compute_pipeline_layout);
    info.stage                        = load_shader("device_fault", "update_vbo.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);
    VK_CHECK(vkCreateComputePipelines(get_device().get_handle(), VK_NULL_HANDLE, 1, &info, nullptr, &pipelines.compute_update_pipeline));
}

void DeviceFault::create_graphics_pipeline()
{
    pipelines.graphics_pipeline_layout = create_pipeline_layout(true);
    VkGraphicsPipelineCreateInfo info  = vkb::initializers::pipeline_create_info(pipelines.graphics_pipeline_layout, render_pass);

    // No VBOs, everything is fetched from buffer device addresses.
    VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();

    // Going to render a simple quad mesh here with index buffer strip and primitive restart,
    // otherwise nothing interesting here.
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state =
            vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, 0, VK_TRUE);

    VkPipelineRasterizationStateCreateInfo rasterization_state =
            vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE, 0);
    VkPipelineColorBlendAttachmentState blend_attachment_state =
            vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE);
    VkPipelineColorBlendStateCreateInfo color_blend_state =
            vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_attachment_state);
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state =
            vkb::initializers::pipeline_depth_stencil_state_create_info(VK_FALSE, VK_FALSE, VK_COMPARE_OP_GREATER);
    VkPipelineViewportStateCreateInfo viewport_state =
            vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);
    VkPipelineMultisampleStateCreateInfo multisample_state =
            vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);
    std::vector<VkDynamicState>      dynamic_state_enables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic_state =
            vkb::initializers::pipeline_dynamic_state_create_info(dynamic_state_enables);

    info.pVertexInputState   = &vertex_input_state;
    info.pInputAssemblyState = &input_assembly_state;
    info.pRasterizationState = &rasterization_state;
    info.pColorBlendState    = &color_blend_state;
    info.pDepthStencilState  = &depth_stencil_state;
    info.pViewportState      = &viewport_state;
    info.pMultisampleState   = &multisample_state;
    info.pDynamicState       = &dynamic_state;

    VkPipelineShaderStageCreateInfo stages[2];
    info.pStages    = stages;
    info.stageCount = 2;

    stages[0] = load_shader("device_fault", "render.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    stages[1] = load_shader("device_fault", "render.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), VK_NULL_HANDLE, 1, &info, nullptr, &pipelines.bindless_vbo_pipeline));
}

void DeviceFault::create_pipelines()
{
    set_memory_debug_label("Creating Compute Pipeline");
    create_compute_pipeline();

    set_memory_debug_label("Creating Graphics Pipeline");
    create_graphics_pipeline();
}

// A straight forward way of creating a "tessellated" quad mesh.
// Choose a low resolution per mesh so it's more visible in the vertex shader what is happening.
static constexpr unsigned mesh_width             = 16;
static constexpr unsigned mesh_height            = 16;
static constexpr unsigned mesh_strips            = mesh_height - 1;
static constexpr unsigned mesh_indices_per_strip = 2 * mesh_width;
static constexpr unsigned mesh_num_indices       = mesh_strips * (mesh_indices_per_strip + 1);        // Add one index to handle primitive restart.

std::unique_ptr<vkb::core::BufferC> DeviceFault::create_index_buffer()
{
    constexpr size_t size = mesh_num_indices * sizeof(uint16_t);

    // Build a simple subdivided quad mesh. We can tweak the vertices later in compute to create a simple cloth-y/wave-like effect.

    auto index_buffer = std::make_unique<vkb::core::BufferC>(get_device(),
                                                             size,
                                                             VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                             VMA_MEMORY_USAGE_GPU_ONLY);

    auto  staging_buffer = vkb::core::BufferC::create_staging_buffer(get_device(), size, nullptr);

    auto *buffer         = reinterpret_cast<uint16_t *>(staging_buffer.map());
    for (unsigned strip = 0; strip < mesh_strips; strip++)
    {
        for (unsigned x = 0; x < mesh_width; x++)
        {
            *buffer++ = strip * mesh_width + x;
            *buffer++ = (strip + 1) * mesh_width + x;
        }
        *buffer++ = 0xffff;
    }

    staging_buffer.flush();
    staging_buffer.unmap();

    auto cmd = get_device().get_command_pool().request_command_buffer();
    cmd->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    cmd->copy_buffer(staging_buffer, *index_buffer, size);

    vkb::BufferMemoryBarrier memory_barrier;
    memory_barrier.src_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
    memory_barrier.dst_access_mask = VK_ACCESS_INDEX_READ_BIT;
    memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
    memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
    cmd->buffer_memory_barrier(*index_buffer, 0, VK_WHOLE_SIZE, memory_barrier);
    cmd->end();

    auto const &graphicsQueue = get_device().get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);
    graphicsQueue.submit(*cmd, VK_NULL_HANDLE);
    graphicsQueue.wait_idle();
    return index_buffer;
}

void DeviceFault::create_vbo_buffers()
{
    test_buffers.resize(64);
    for (auto &buffer : test_buffers)
    {
        buffer = create_vbo_buffer();
    }

    pointer_buffer = create_pointer_buffer();
}

DeviceFault::TestBuffer DeviceFault::create_vbo_buffer()
{
    TestBuffer buffer;

    // Here we represent each "meshlet" as its own buffer to demonstrate maximum allocation flexibility.
    VkDevice         device    = get_device().get_handle();
    constexpr size_t mesh_size = mesh_width * mesh_height * sizeof(glm::vec2);

    // To be able to query the buffer device address, we must use the SHADER_DEVICE_ADDRESS_BIT usage flag.
    // STORAGE_BUFFER is also required.
    VkBufferCreateInfo create_info = vkb::initializers::buffer_create_info(
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR, mesh_size);

    VK_CHECK(vkCreateBuffer(device, &create_info, nullptr, &buffer.buffer));

    VkMemoryAllocateInfo memory_allocation_info = vkb::initializers::memory_allocate_info();
    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(device, buffer.buffer, &memory_requirements);

    // Another change is that the memory we allocate must be marked as buffer device address capable.
    VkMemoryAllocateFlagsInfoKHR flags_info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR};
    flags_info.flags             = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
    memory_allocation_info.pNext = &flags_info;

    memory_allocation_info.allocationSize  = memory_requirements.size;
    memory_allocation_info.memoryTypeIndex = get_device().get_gpu().get_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    set_memory_debug_label("Allocating Vertex Buffer Object");
    VK_CHECK(vkAllocateMemory(get_device().get_handle(), &memory_allocation_info, nullptr, &buffer.memory));

    set_memory_debug_label("Binding Vertex Buffer Object");
    VK_CHECK(vkBindBufferMemory(get_device().get_handle(), buffer.buffer, buffer.memory, 0));

    // Once we've bound the buffer, we query the buffer device address.
    // We can now place this address (or any offset of said address) into a buffer and access data as a raw pointer in shaders.
    VkBufferDeviceAddressInfoKHR address_info{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR};
    address_info.buffer = buffer.buffer;
    buffer.gpu_address  = vkGetBufferDeviceAddressKHR(device, &address_info);

    // The buffer content will be computed at runtime, so don't upload anything.

    return buffer;
}

DeviceFault::TestBuffer DeviceFault::create_pointer_buffer()
{
    // Just like create_vbo_buffer(), we create a buffer which holds other pointers.
    TestBuffer buffer;

    VkDevice device      = get_device().get_handle();
    size_t   buffer_size = test_buffers.size() * sizeof(VkDeviceAddress); // 64 * 8

    // We use TRANSFER_DST since we will upload to the buffer later.
    VkBufferCreateInfo create_info = vkb::initializers::buffer_create_info(
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR, buffer_size);

    VK_CHECK(vkCreateBuffer(device, &create_info, nullptr, &buffer.buffer));

    VkMemoryAllocateInfo         memory_allocation_info = vkb::initializers::memory_allocate_info();
    VkMemoryAllocateFlagsInfoKHR flags_info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR};
    VkMemoryRequirements         memory_requirements;
    vkGetBufferMemoryRequirements(device, buffer.buffer, &memory_requirements);

    flags_info.flags             = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
    memory_allocation_info.pNext = &flags_info;

    memory_allocation_info.allocationSize  = memory_requirements.size;
    memory_allocation_info.memoryTypeIndex = get_device().get_gpu().get_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    set_memory_debug_label("Allocating Pointer Buffer ");
    VK_CHECK(vkAllocateMemory(get_device().get_handle(), &memory_allocation_info, nullptr, &buffer.memory));

    // DONE: LOGI("PointerBuffer::BindMemory::Binding");
    set_memory_debug_label("Binding Pointer Buffer ");
    VK_CHECK(vkBindBufferMemory(get_device().get_handle(), buffer.buffer, buffer.memory, 0));

    VkBufferDeviceAddressInfoKHR address_info{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR};
    address_info.buffer = buffer.buffer;
    buffer.gpu_address  = vkGetBufferDeviceAddressKHR(device, &address_info);

    return buffer;
}

void DeviceFault::update_pointer_buffer(VkCommandBuffer cmd)
{
    // Wait with updating the pointer buffer until previous frame's vertex shading is complete.
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                         0, nullptr, 0, nullptr, 0, nullptr);

    std::vector<VkDeviceAddress> pointers;
    pointers.reserve(test_buffers.size());
    for (auto &test_buffer : test_buffers)
    {
        pointers.push_back(test_buffer.gpu_address);
    }

    // Simple approach. A proxy for a compute shader which culls meshlets.
    vkCmdUpdateBuffer(cmd, pointer_buffer.buffer, 0, test_buffers.size() * sizeof(VkDeviceAddress), pointers.data());

    VkMemoryBarrier global_memory_barrier = vkb::initializers::memory_barrier();
    global_memory_barrier.srcAccessMask   = VK_ACCESS_TRANSFER_WRITE_BIT;
    global_memory_barrier.dstAccessMask   = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0,
                         1, &global_memory_barrier, 0, nullptr, 0, nullptr);
}

void DeviceFault::update_meshlets(VkCommandBuffer cmd)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipelines.compute_update_pipeline);

    PushCompute push_compute{};

    // Here we push a pointer to a buffer, which holds pointers to all the VBO "meshlets".
    push_compute.table = pointer_buffer.gpu_address;

    // So we can create a wave-like animation.
    push_compute.fract_time = accumulated_time;

    // So we can trigger device fault using invalid address
    push_compute.trigger_device_fault_invalid_address = triggerDeviceFaultInvalidAddress;

    vkCmdPushConstants(cmd, pipelines.compute_pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT,
                       0, sizeof(push_compute), &push_compute);

    // Write-after-read hazard is implicitly handled by the earlier pointer buffer update where
    // we did VERTEX -> TRANSFER -> COMPUTE chain of barriers.
    // Update all meshlets.
    vkCmdDispatch(cmd, mesh_width / 8, mesh_height / 8, static_cast<uint32_t>(test_buffers.size()));

    VkMemoryBarrier global_memory_barrier = vkb::initializers::memory_barrier();
    global_memory_barrier.srcAccessMask   = VK_ACCESS_SHADER_WRITE_BIT;
    global_memory_barrier.dstAccessMask   = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                         0, 1, &global_memory_barrier, 0, nullptr, 0, nullptr);
}

void DeviceFault::render(float delta_time)
{
    // Since it will be reporting each buffers binding information in every frame,
    // we don't want to print everything for better visibility when VK_ERROR_DEVICE_LOST occurs.
    deviceBindingReportEnabled = false;

    ApiVulkanSample::prepare_frame();
    VK_CHECK(vkWaitForFences(get_device().get_handle(), 1, &wait_fences[current_buffer], VK_TRUE, UINT64_MAX));
    VK_CHECK(vkResetFences(get_device().get_handle(), 1, &wait_fences[current_buffer]));

    VkViewport viewport = {0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f};
    VkRect2D   scissor  = {{0, 0}, {width, height}};

    recreate_current_command_buffer();
    auto cmd         = draw_cmd_buffers[current_buffer];
    auto begin_info  = vkb::initializers::command_buffer_begin_info();
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &begin_info);

    // First thing is to update the pointer buffer.
    // We could use a compute shader here if we're doing
    // GPU-driven rendering for example.
    update_pointer_buffer(cmd);

    // Arbitrary value between 0 and 1 to create some animation.
    accumulated_time += 0.2f * delta_time;
    accumulated_time = glm::fract(accumulated_time);

    // Update VBOs through buffer_device_address.
    update_meshlets(cmd);

    VkRenderPassBeginInfo render_pass_begin    = vkb::initializers::render_pass_begin_info();
    render_pass_begin.renderPass               = render_pass;
    render_pass_begin.renderArea.extent.width  = width;
    render_pass_begin.renderArea.extent.height = height;
    render_pass_begin.clearValueCount          = 2;
    VkClearValue clears[2]                     = {};
    clears[0].color.float32[0]                 = 0.033f;
    clears[0].color.float32[1]                 = 0.073f;
    clears[0].color.float32[2]                 = 0.133f;
    render_pass_begin.pClearValues             = clears;
    render_pass_begin.framebuffer              = framebuffers[current_buffer];

    vkCmdBeginRenderPass(cmd, &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.bindless_vbo_pipeline);
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    PushVertex push_vertex{};

    // Create an ad-hoc perspective matrix.
    push_vertex.view_projection =
            glm::perspective(0.5f * glm::pi<float>(), static_cast<float>(width) / static_cast<float>(height), 1.0f, 100.0f) *
            glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Push pointer to array of meshlets.
    // Every instance renders its own meshlet.
    push_vertex.table = pointer_buffer.gpu_address;
    vkCmdPushConstants(cmd, pipelines.graphics_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push_vertex), &push_vertex);
    vkCmdBindIndexBuffer(cmd, index_buffer->get_handle(), 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(cmd, mesh_num_indices, static_cast<uint32_t>(test_buffers.size()), 0, 0, 0);

    draw_ui(cmd);

    vkCmdEndRenderPass(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];

    VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, wait_fences[current_buffer]));

    // A bit of a hack. This is usually seated in ApiVulkanSample::submit_frame(), but that throws immediately if the device enters an error state.
    // So we incorrectly call wait_idle here, so we can get the GPU in error state, and we can query it for device_fault before an exception is thrown.
    VkResult error = get_device().get_queue_by_present(0).wait_idle();

    try
    {
        ApiVulkanSample::submit_frame();
    }
    catch (std::exception const &e)
    {
        vk::DeviceLostError const *device_lost_error = reinterpret_cast<vk::DeviceLostError const *>(&e);
        if (device_lost_error)
        {
            check_device_fault(VK_ERROR_DEVICE_LOST);
        }
    }

}

void DeviceFault::request_gpu_features(vkb::core::PhysicalDeviceC &gpu)
{
    LOGI("Requesting features from GPU.");
    // Need to enable the bufferDeviceAddress feature.
    REQUEST_REQUIRED_FEATURE(gpu,
                             VkPhysicalDeviceBufferDeviceAddressFeaturesKHR,
                             bufferDeviceAddress);

    // Enable the deviceFault feature for handling hardware faults.
    REQUEST_REQUIRED_FEATURE(gpu,
                             VkPhysicalDeviceFaultFeaturesEXT,
                             deviceFault);

    // Enable binding report for getting more information on GPU virtual address spaces
    REQUEST_OPTIONAL_FEATURE(gpu,
                             VkPhysicalDeviceAddressBindingReportFeaturesEXT,
                             reportAddressBinding);
}

std::unique_ptr<ApiVulkanSample> create_device_fault()
{
    return std::make_unique<DeviceFault>();
}

void DeviceFault::set_memory_debug_label(std::string current_address_reporter)
{
    this->current_memory_label = std::move(current_address_reporter);
}