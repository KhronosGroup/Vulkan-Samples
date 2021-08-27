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

#include "framebuffer.h"

#include "device.h"

namespace vkb
{
VkFramebuffer Framebuffer::get_handle() const
{
	return handle;
}

const VkExtent2D &Framebuffer::get_extent() const
{
	return extent;
}

Framebuffer::Framebuffer(Device &device, const RenderTarget &render_target, const RenderPass &render_pass) :
    device{device},
    extent{render_target.get_extent()}
{
	std::vector<VkImageView> attachments;

	for (auto &view : render_target.get_views())
	{
		attachments.emplace_back(view.get_handle());
	}

	VkFramebufferCreateInfo create_info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};

	create_info.renderPass      = render_pass.get_handle();
	create_info.attachmentCount = to_u32(attachments.size());
	create_info.pAttachments    = attachments.data();
	create_info.width           = extent.width;
	create_info.height          = extent.height;
	create_info.layers          = 1;

	auto result = vkCreateFramebuffer(device.get_handle(), &create_info, nullptr, &handle);

	if (result != VK_SUCCESS)
	{
		throw VulkanException{result, "Cannot create Framebuffer"};
	}
}

Framebuffer::Framebuffer(Framebuffer &&other)  noexcept :
    device{other.device},
    handle{other.handle},
    extent{other.extent}
{
	other.handle = VK_NULL_HANDLE;
}

Framebuffer::~Framebuffer()
{
	if (handle != VK_NULL_HANDLE)
	{
		vkDestroyFramebuffer(device.get_handle(), handle, nullptr);
	}
}
}        // namespace vkb
