/* Copyright (c) 2019-2020, Arm Limited and Contributors
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

#include "descriptor_set.h"

#include "common/logging.h"
#include "descriptor_pool.h"
#include "descriptor_set_layout.h"
#include "device.h"

namespace vkb
{
DescriptorSet::DescriptorSet(Device &                                  device,
                             DescriptorSetLayout &                     descriptor_set_layout,
                             DescriptorPool &                          descriptor_pool,
                             const BindingMap<VkDescriptorBufferInfo> &buffer_infos,
                             const BindingMap<VkDescriptorImageInfo> & image_infos) :
    device{device},
    descriptor_set_layout{descriptor_set_layout},
    descriptor_pool{descriptor_pool},
    handle{descriptor_pool.allocate()}
{
	if (!buffer_infos.empty() || !image_infos.empty())
	{
		update(buffer_infos, image_infos);
	}
}

void DescriptorSet::update(const BindingMap<VkDescriptorBufferInfo> &buffer_infos, const BindingMap<VkDescriptorImageInfo> &image_infos)
{
	this->buffer_infos = buffer_infos;
	this->image_infos  = image_infos;

	std::vector<VkWriteDescriptorSet> set_updates;

	// Iterate over all buffer bindings
	for (auto &binding_it : buffer_infos)
	{
		auto  binding         = binding_it.first;
		auto &buffer_bindings = binding_it.second;

		if (auto binding_info = descriptor_set_layout.get_layout_binding(binding))
		{
			// Iterate over all binding buffers in array
			for (auto &element_it : buffer_bindings)
			{
				auto  arrayElement = element_it.first;
				auto &buffer_info  = element_it.second;

				VkWriteDescriptorSet write_descriptor_set{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

				write_descriptor_set.dstBinding      = binding;
				write_descriptor_set.descriptorType  = binding_info->descriptorType;
				write_descriptor_set.pBufferInfo     = &buffer_info;
				write_descriptor_set.dstSet          = handle;
				write_descriptor_set.dstArrayElement = arrayElement;
				write_descriptor_set.descriptorCount = 1;

				set_updates.push_back(write_descriptor_set);
			}
		}
		else
		{
			LOGE("Shader layout set does not use buffer binding at #{}", binding);
		}
	}

	// Iterate over all image bindings
	for (auto &binding_it : image_infos)
	{
		auto  binding_index     = binding_it.first;
		auto &binding_resources = binding_it.second;

		if (auto binding_info = descriptor_set_layout.get_layout_binding(binding_index))
		{
			// Iterate over all binding images in array
			for (auto &element_it : binding_resources)
			{
				auto  arrayElement = element_it.first;
				auto &image_info   = element_it.second;

				VkWriteDescriptorSet write_descriptor_set{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

				write_descriptor_set.dstBinding      = binding_index;
				write_descriptor_set.descriptorType  = binding_info->descriptorType;
				write_descriptor_set.pImageInfo      = &image_info;
				write_descriptor_set.dstSet          = handle;
				write_descriptor_set.dstArrayElement = arrayElement;
				write_descriptor_set.descriptorCount = 1;

				set_updates.push_back(write_descriptor_set);
			}
		}
		else
		{
			LOGE("Shader layout set does not use image binding at #{}", binding_index);
		}
	}

	vkUpdateDescriptorSets(device.get_handle(), to_u32(set_updates.size()), set_updates.data(), 0, nullptr);
}

DescriptorSet::DescriptorSet(DescriptorSet &&other) :
    device{other.device},
    descriptor_set_layout{other.descriptor_set_layout},
    descriptor_pool{other.descriptor_pool},
    buffer_infos{std::move(other.buffer_infos)},
    image_infos{std::move(other.image_infos)},
    handle{other.handle}
{
	other.handle = VK_NULL_HANDLE;
}

VkDescriptorSet DescriptorSet::get_handle() const
{
	return handle;
}

const DescriptorSetLayout &DescriptorSet::get_layout() const
{
	return descriptor_set_layout;
}

BindingMap<VkDescriptorBufferInfo> &DescriptorSet::get_buffer_infos()
{
	return buffer_infos;
}

BindingMap<VkDescriptorImageInfo> &DescriptorSet::get_image_infos()
{
	return image_infos;
}

}        // namespace vkb
