/* Copyright (c) 2019, Arm Limited and Contributors
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

#include "common/helpers.h"
#include "common/vk_common.h"

namespace vkb
{
class Device;
class DescriptorSetLayout;

class DescriptorSet : public NonCopyable
{
  public:
	DescriptorSet(Device &                                  device,
	              DescriptorSetLayout &                     descriptor_set_layout,
	              const BindingMap<VkDescriptorBufferInfo> &buffer_infos = {},
	              const BindingMap<VkDescriptorImageInfo> & image_infos  = {});

	DescriptorSet(DescriptorSet &&other);

	~DescriptorSet();

	void update(const BindingMap<VkDescriptorBufferInfo> &buffer_infos,
	            const BindingMap<VkDescriptorImageInfo> & image_infos);

	VkDescriptorSet get_handle() const;

	const DescriptorSetLayout &get_layout() const;

	BindingMap<VkDescriptorBufferInfo> &get_buffer_infos();

	BindingMap<VkDescriptorImageInfo> &get_image_infos();

  private:
	Device &device;

	DescriptorSetLayout &descriptor_set_layout;

	BindingMap<VkDescriptorBufferInfo> buffer_infos;

	BindingMap<VkDescriptorImageInfo> image_infos;

	VkDescriptorSet handle{VK_NULL_HANDLE};
};
}        // namespace vkb
