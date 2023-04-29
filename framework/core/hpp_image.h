/* Copyright (c) 2021-2023, NVIDIA CORPORATION. All rights reserved.
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

#include "core/hpp_vulkan_resource.h"
#include <unordered_set>
#include <vk_mem_alloc.h>

namespace vkb
{
namespace core
{
class HPPDevice;
class HPPImageView;

class HPPImage : public vkb::core::HPPVulkanResource<vk::Image>
{
  public:
	HPPImage(HPPDevice              &device,
	         vk::Image               handle,
	         const vk::Extent3D     &extent,
	         vk::Format              format,
	         vk::ImageUsageFlags     image_usage,
	         vk::SampleCountFlagBits sample_count = vk::SampleCountFlagBits::e1);

	HPPImage(HPPDevice              &device,
	         const vk::Extent3D     &extent,
	         vk::Format              format,
	         vk::ImageUsageFlags     image_usage,
	         VmaMemoryUsage          memory_usage,
	         vk::SampleCountFlagBits sample_count       = vk::SampleCountFlagBits::e1,
	         uint32_t                mip_levels         = 1,
	         uint32_t                array_layers       = 1,
	         vk::ImageTiling         tiling             = vk::ImageTiling::eOptimal,
	         vk::ImageCreateFlags    flags              = {},
	         uint32_t                num_queue_families = 0,
	         const uint32_t         *queue_families     = nullptr);

	HPPImage(const HPPImage &) = delete;

	HPPImage(HPPImage &&other);

	~HPPImage() override;

	HPPImage &operator=(const HPPImage &) = delete;

	HPPImage &operator=(HPPImage &&) = delete;

	VmaAllocation get_memory() const;

	/**
	 * @brief Maps vulkan memory to an host visible address
	 * @return Pointer to host visible memory
	 */
	uint8_t *map();

	/**
	 * @brief Unmaps vulkan memory from the host visible address
	 */
	void unmap();

	vk::ImageType                                  get_type() const;
	const vk::Extent3D                            &get_extent() const;
	vk::Format                                     get_format() const;
	vk::SampleCountFlagBits                        get_sample_count() const;
	vk::ImageUsageFlags                            get_usage() const;
	vk::ImageTiling                                get_tiling() const;
	vk::ImageSubresource                           get_subresource() const;
	uint32_t                                       get_array_layer_count() const;
	std::unordered_set<vkb::core::HPPImageView *> &get_views();

  private:
	VmaAllocation                                 memory = VK_NULL_HANDLE;
	vk::ImageType                                 type;
	vk::Extent3D                                  extent;
	vk::Format                                    format;
	vk::ImageUsageFlags                           usage;
	vk::SampleCountFlagBits                       sample_count;
	vk::ImageTiling                               tiling;
	vk::ImageSubresource                          subresource;
	uint32_t                                      array_layer_count = 0;
	std::unordered_set<vkb::core::HPPImageView *> views;        /// HPPImage views referring to this image
	uint8_t                                      *mapped_data = nullptr;
	bool                                          mapped      = false;        /// Whether it was mapped with vmaMapMemory
};
}        // namespace core
}        // namespace vkb
