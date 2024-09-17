/* Copyright (c) 2024, Arm Limited and Contributors
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

#include "hack_emulated_uniform_buffer.h"

#include "benchmark_mode/benchmark_mode.h"

hack_emulated_uniform_buffer::hack_emulated_uniform_buffer()
{
  // Force HLSL to not have to implement all shaders twice.
  set_shading_language(vkb::ShadingLanguage::HLSL);

  title = "Hack: Emulated uniform buffers";
}

hack_emulated_uniform_buffer::~hack_emulated_uniform_buffer()
{
  if (has_device())
  {
    // Clean up used Vulkan resources
    // Note : Inherited destructor cleans up resources stored in base class
    vkDestroyPipeline(get_device().get_handle(), pipeline, nullptr);

    vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);
    vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, nullptr);
  }
}

void hack_emulated_uniform_buffer::draw(VkCommandBuffer& commandBuffer)
{
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  VkDeviceSize offsets[1] = { 0 };
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertex_buffer->get(), offsets);
  vkCmdBindIndexBuffer(commandBuffer, index_buffer->get_handle(), 0, VK_INDEX_TYPE_UINT32);

  // Render multiple objects using different model matrices by dynamically offsetting into one uniform buffer
  for (uint32_t j = 0; j < OBJECT_INSTANCES; j++)
  {
    // One dynamic offset per dynamic descriptor to offset into the ubo containing all model matrices
    uint32_t dynamic_offset = j * static_cast<uint32_t>(dynamic_alignment);
    // Bind the descriptor set for rendering a mesh using the dynamic offset
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set[j], 0, nullptr);

    vkCmdDrawIndexed(commandBuffer, index_count, 1, 0, 0, 0);
  }
}

void hack_emulated_uniform_buffer::setup_descriptor_pool()
{
  // Example uses one ubo and one image sampler
  std::vector<VkDescriptorPoolSize> pool_sizes =
  {
      vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, OBJECT_INSTANCES),
      vkb::initializers::descriptor_pool_size(emulated_type, OBJECT_INSTANCES)
  };

  VkDescriptorPoolCreateInfo descriptor_pool_create_info =
    vkb::initializers::descriptor_pool_create_info(
      static_cast<uint32_t>(pool_sizes.size()),
      pool_sizes.data(),
      OBJECT_INSTANCES * 2);

  VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void hack_emulated_uniform_buffer::setup_descriptor_set_layout()
{
  std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings =
  {
      vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
      vkb::initializers::descriptor_set_layout_binding(emulated_type, VK_SHADER_STAGE_VERTEX_BIT, 1)
  };

  VkDescriptorSetLayoutCreateInfo descriptor_layout =
    vkb::initializers::descriptor_set_layout_create_info(
      set_layout_bindings.data(),
      static_cast<uint32_t>(set_layout_bindings.size()));

  VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout, nullptr, &descriptor_set_layout));

  VkPipelineLayoutCreateInfo pipeline_layout_create_info =
    vkb::initializers::pipeline_layout_create_info(
      &descriptor_set_layout,
      1);

  VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout));
}

void hack_emulated_uniform_buffer::setup_descriptor_set()
{
  VkDescriptorSetAllocateInfo alloc_info =
    vkb::initializers::descriptor_set_allocate_info(
      descriptor_pool,
      &descriptor_set_layout,
      1);

  VkDescriptorBufferInfo view_buffer_descriptor = create_descriptor(*view_uniform_buffer.view);

  for (uint32_t j = 0; j < OBJECT_INSTANCES; j++)
  {
    VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_set[j]));

    VkWriteDescriptorSet cubeDesc = {};
    VkDescriptorBufferInfo aaa = create_descriptor(*emulated_uniform_buffer.buffer, dynamic_alignment * OBJECT_INSTANCES, j * dynamic_alignment);

    if (emulated_type == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER)
    {
      VkBufferViewCreateInfo viewCreateInfo = {};
      viewCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
      viewCreateInfo.buffer = emulated_uniform_buffer.buffer->get_handle();
      viewCreateInfo.format = VkFormat::VK_FORMAT_R32_SFLOAT;
      viewCreateInfo.offset = j * dynamic_alignment;
      viewCreateInfo.range = dynamic_alignment;

      // Pass the actual dynamic alignment as the descriptor's size
      vkCreateBufferView(get_device().get_handle(), &viewCreateInfo, nullptr, &buffer_views[j]);

      cubeDesc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      cubeDesc.dstSet = descriptor_set[j];
      cubeDesc.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
      cubeDesc.dstBinding = 1;
      cubeDesc.pTexelBufferView = &buffer_views[j];
      cubeDesc.descriptorCount = 1;
    }
    else
    {
      cubeDesc = vkb::initializers::write_descriptor_set(descriptor_set[j], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, &aaa);
    }


    std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
      // Binding 0 : Projection/View matrix uniform buffer
      vkb::initializers::write_descriptor_set(descriptor_set[j], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &view_buffer_descriptor),
      // Binding 1 : Instance matrix as texel buffer
      cubeDesc,
    };

    vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, NULL);
  }
}

void hack_emulated_uniform_buffer::prepare_pipelines()
{
  // Load shaders
  std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages;

  shader_stages[0] = load_shader("hackathon", emulated_type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ? "emulated_uniform.structuredbuffer.vert" : "emulated_uniform.buffer.vert", VK_SHADER_STAGE_VERTEX_BIT);
  shader_stages[1] = load_shader("hackathon", "emulated_uniform.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

  VkGraphicsPipelineCreateInfo pipeline_create_info =
    vkb::initializers::pipeline_create_info(
      pipeline_layout,
      render_pass,
      0);

  pipeline_create_info.pVertexInputState = &vertex_input_state;
  pipeline_create_info.pInputAssemblyState = &input_assembly_state;
  pipeline_create_info.pRasterizationState = &rasterization_state;
  pipeline_create_info.pColorBlendState = &color_blend_state;
  pipeline_create_info.pMultisampleState = &multisample_state;
  pipeline_create_info.pViewportState = &viewport_state;
  pipeline_create_info.pDepthStencilState = &depth_stencil_state;
  pipeline_create_info.pDynamicState = &dynamic_state;
  pipeline_create_info.stageCount = static_cast<uint32_t>(shader_stages.size());
  pipeline_create_info.pStages = shader_stages.data();

  VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipeline));
}

// Prepare and initialize uniform buffer containing shader uniforms
void hack_emulated_uniform_buffer::prepare_emulated_uniform_buffer()
{
  // Allocate data for the dynamic uniform buffer object
  // We allocate this manually as the alignment of the offset differs between GPUs

  // Calculate required alignment based on minimum device offset alignment
  size_t min_ubo_alignment = static_cast<size_t>(get_device().get_gpu().get_properties().limits.minUniformBufferOffsetAlignment);
  dynamic_alignment = sizeof(glm::mat4);
  if (min_ubo_alignment > 0)
  {
    dynamic_alignment = (dynamic_alignment + min_ubo_alignment - 1) & ~(min_ubo_alignment - 1);
  }

  size_t buffer_size;
  prepare_aligned_cubes(dynamic_alignment, &buffer_size);

  std::cout << "minUniformBufferOffsetAlignment = " << min_ubo_alignment << std::endl;
  std::cout << "dynamicAlignment = " << dynamic_alignment << std::endl;

  // Vertex shader uniform buffer block

  emulated_uniform_buffer.buffer = std::make_unique<vkb::core::BufferC>(get_device(),
    buffer_size,
    emulated_type == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER ? VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT : VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    VMA_MEMORY_USAGE_CPU_TO_GPU);

  update_emulated_uniform_buffer();
}

void hack_emulated_uniform_buffer::update_emulated_uniform_buffer()
{
  emulated_uniform_buffer.buffer->update(aligned_cubes, static_cast<size_t>(emulated_uniform_buffer.buffer->get_size()));
  // Flush to make changes visible to the device
  emulated_uniform_buffer.buffer->flush();
}

void hack_emulated_uniform_buffer::hack_prepare()
{
  prepare_emulated_uniform_buffer();
  setup_descriptor_set_layout();
  prepare_pipelines();
  setup_descriptor_pool();
  setup_descriptor_set();
  build_command_buffers();
}

void hack_emulated_uniform_buffer::hack_render(VkCommandBuffer& commandBuffer)
{
  if (!paused)
  {
    update_emulated_uniform_buffer();
  }

  draw(commandBuffer);
}

/// 
std::unique_ptr<vkb::VulkanSampleC> create_hack_emulated_uniform_buffer()
{
  return std::make_unique<hack_emulated_uniform_buffer>();
}
