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
struct Attachment;
class Device;

struct LoadStoreInfo
{
	VkAttachmentLoadOp load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;

	VkAttachmentStoreOp store_op = VK_ATTACHMENT_STORE_OP_STORE;
};

struct SubpassInfo
{
	std::vector<uint32_t> input_attachments;

	std::vector<uint32_t> output_attachments;
};

class RenderPass
{
  public:
	VkRenderPass get_handle() const;

	RenderPass(Device &                          device,
	           const std::vector<Attachment> &   attachments,
	           const std::vector<LoadStoreInfo> &load_store_infos,
	           const std::vector<SubpassInfo> &  subpasses);

	RenderPass(const RenderPass &) = delete;

	RenderPass(RenderPass &&other);

	~RenderPass();

	RenderPass &operator=(const RenderPass &) = delete;

	RenderPass &operator=(RenderPass &&) = delete;

	const uint32_t get_color_output_count(uint32_t subpass_index) const;

  private:
	Device &device;

	VkRenderPass handle{VK_NULL_HANDLE};

	size_t subpass_count;

	// Store attachments for every subpass
	std::vector<std::vector<VkAttachmentReference>> input_attachments;

	std::vector<std::vector<VkAttachmentReference>> color_attachments;

	std::vector<std::vector<VkAttachmentReference>> depth_stencil_attachments;
};
}        // namespace vkb
