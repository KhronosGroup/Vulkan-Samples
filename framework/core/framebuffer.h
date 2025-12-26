/* Copyright (c) 2019-2025, Arm Limited and Contributors
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
class RenderPass;
class RenderTarget;

namespace core
{
template <vkb::BindingType bindingType>
class Device;
using DeviceC = Device<vkb::BindingType::C>;
}        // namespace core

class Framebuffer
{
  public:
	Framebuffer(vkb::core::DeviceC &device, const RenderTarget &render_target, const RenderPass &render_pass);

	Framebuffer(const Framebuffer &) = delete;

	Framebuffer(Framebuffer &&other);

	~Framebuffer();

	Framebuffer &operator=(const Framebuffer &) = delete;

	Framebuffer &operator=(Framebuffer &&) = delete;

	VkFramebuffer get_handle() const;

	const VkExtent2D &get_extent() const;

  private:
	vkb::core::DeviceC &device;

	VkFramebuffer handle{VK_NULL_HANDLE};

	VkExtent2D extent{};
};
}        // namespace vkb
