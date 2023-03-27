/* Copyright (c) 2023, NVIDIA CORPORATION. All rights reserved.
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

#include "core/framebuffer.h"
#include <vulkan/vulkan.hpp>

namespace vkb
{
namespace core
{
/**
 * @brief facade class around vkb::Framebuffer, providing a vulkan.hpp-based interface
 *
 * See vkb::Framebuffer for documentation
 */
class HPPFramebuffer : private vkb::Framebuffer
{
  public:
	const vk::Extent2D &get_extent() const
	{
		return reinterpret_cast<vk::Extent2D const &>(vkb::Framebuffer::get_extent());
	}

	vk::Framebuffer get_handle() const
	{
		return static_cast<vk::Framebuffer>(vkb::Framebuffer::get_handle());
	}
};
}        // namespace core
}        // namespace vkb
