/* Copyright (c) 2023-2024, NVIDIA CORPORATION. All rights reserved.
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

#include "core/hpp_image.h"
#include "core/vulkan_resource.h"

namespace vkb
{
namespace core
{
class HPPImageView : public vkb::core::VulkanResource<vkb::BindingType::Cpp, vk::ImageView>
{
  public:
	HPPImageView(vkb::core::HPPImage &image,
	             vk::ImageViewType    view_type,
	             vk::Format           format           = vk::Format::eUndefined,
	             uint32_t             base_mip_level   = 0,
	             uint32_t             base_array_layer = 0,
	             uint32_t             n_mip_levels     = 0,
	             uint32_t             n_array_layers   = 0);

	HPPImageView(HPPImageView &) = delete;
	HPPImageView(HPPImageView &&other);

	~HPPImageView() override;

	HPPImageView &operator=(const HPPImageView &) = delete;
	HPPImageView &operator=(HPPImageView &&)      = delete;

	vk::Format                 get_format() const;
	vkb::core::HPPImage const &get_image() const;
	void                       set_image(vkb::core::HPPImage &image);
	vk::ImageSubresourceLayers get_subresource_layers() const;
	vk::ImageSubresourceRange  get_subresource_range() const;

  private:
	vkb::core::HPPImage      *image = nullptr;
	vk::Format                format;
	vk::ImageSubresourceRange subresource_range;
};
}        // namespace core
}        // namespace vkb
