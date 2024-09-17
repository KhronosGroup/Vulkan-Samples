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

#pragma once

#include "hack_base.h"

class hack_array_buffer : public hack_base
{
protected:
  struct ArrayBuffers
  {
    std::unique_ptr<vkb::core::BufferC> array;
  } array_buffers;

public:
  hack_array_buffer();
  virtual ~hack_array_buffer();

  void draw(VkCommandBuffer &commandBuffer);

  void setup_descriptor_pool();
  void setup_descriptor_set_layout();
  void setup_descriptor_set();

  void prepare_pipelines();

  void prepare_array_buffer();
  void update_array_buffer();

  virtual void hack_prepare() override;
  virtual void hack_render(VkCommandBuffer &commandBuffer) override;

private:
  VkPipeline            pipeline;
  VkPipelineLayout      pipeline_layout;
  VkDescriptorSet       descriptor_set;
  VkDescriptorSetLayout descriptor_set_layout;
};

std::unique_ptr<vkb::VulkanSampleC> create_hack_array_buffer();
