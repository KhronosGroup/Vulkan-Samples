/* Copyright (c) 2019-2024, Arm Limited and Contributors
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
#include "core/vulkan_resource.h"

namespace vkb
{
struct Attachment;
class Device;

struct SubpassInfo
{
	std::vector<uint32_t> input_attachments;

	std::vector<uint32_t> output_attachments;

	std::vector<uint32_t> color_resolve_attachments;

	bool disable_depth_stencil_attachment;

	uint32_t depth_stencil_resolve_attachment;

	VkResolveModeFlagBits depth_stencil_resolve_mode;

	std::string debug_name;
};

class RenderPass : public core::VulkanResource<VkRenderPass>
{
  public:
	RenderPass(Device                           &device,
	           const std::vector<Attachment>    &attachments,
	           const std::vector<LoadStoreInfo> &load_store_infos,
	           const std::vector<SubpassInfo>   &subpasses);

	RenderPass(const RenderPass &) = delete;

	RenderPass(RenderPass &&other);

	~RenderPass();

	RenderPass &operator=(const RenderPass &) = delete;

	RenderPass &operator=(RenderPass &&) = delete;

	const uint32_t get_color_output_count(uint32_t subpass_index) const;

	const VkExtent2D get_render_area_granularity() const;

  private:
	size_t subpass_count;

	template <typename T_SubpassDescription, typename T_AttachmentDescription, typename T_AttachmentReference, typename T_SubpassDependency, typename T_RenderPassCreateInfo>
	void create_renderpass(const std::vector<Attachment> &attachments, const std::vector<LoadStoreInfo> &load_store_infos, const std::vector<SubpassInfo> &subpasses);

	std::vector<uint32_t> color_output_count;
};
}        // namespace vkb
