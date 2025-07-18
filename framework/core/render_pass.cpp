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

#include "render_pass.h"

#include <numeric>
#include <span>

#include "device.h"
#include "rendering/render_target.h"

namespace vkb
{
namespace
{
inline void set_structure_type(VkAttachmentDescription &attachment)
{
	// VkAttachmentDescription has no sType field
}

inline void set_structure_type(VkAttachmentDescription2KHR &attachment)
{
	attachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2_KHR;
}

inline void set_structure_type(VkAttachmentReference &reference)
{
	// VkAttachmentReference has no sType field
}

inline void set_structure_type(VkAttachmentReference2KHR &reference)
{
	reference.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2_KHR;
}

inline void set_structure_type(VkRenderPassCreateInfo &create_info)
{
	create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
}

inline void set_structure_type(VkRenderPassCreateInfo2KHR &create_info)
{
	create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2_KHR;
}

inline void set_structure_type(VkSubpassDescription &description)
{
	// VkSubpassDescription has no sType field
}

inline void set_structure_type(VkSubpassDescription2KHR &description)
{
	description.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2_KHR;
}

inline void set_pointer_next(VkSubpassDescription &subpass_description, VkSubpassDescriptionDepthStencilResolveKHR &depth_resolve, VkAttachmentReference &depth_resolve_attachment)
{
	// VkSubpassDescription cannot have pNext point to a VkSubpassDescriptionDepthStencilResolveKHR containing a VkAttachmentReference
}

inline void set_pointer_next(VkSubpassDescription2KHR &subpass_description, VkSubpassDescriptionDepthStencilResolveKHR &depth_resolve, VkAttachmentReference2KHR &depth_resolve_attachment)
{
	depth_resolve.pDepthStencilResolveAttachment = &depth_resolve_attachment;
	subpass_description.pNext                    = &depth_resolve;
}

inline const VkAttachmentReference2KHR *get_depth_resolve_reference(const VkSubpassDescription &subpass_description)
{
	// VkSubpassDescription cannot have pNext point to a VkSubpassDescriptionDepthStencilResolveKHR containing a VkAttachmentReference2KHR
	return nullptr;
}

inline const VkAttachmentReference2KHR *get_depth_resolve_reference(const VkSubpassDescription2KHR &subpass_description)
{
	auto description_depth_resolve = static_cast<const VkSubpassDescriptionDepthStencilResolveKHR *>(subpass_description.pNext);

	const VkAttachmentReference2KHR *depth_resolve_attachment = nullptr;
	if (description_depth_resolve)
	{
		depth_resolve_attachment = description_depth_resolve->pDepthStencilResolveAttachment;
	}

	return depth_resolve_attachment;
}

inline VkResult create_vk_renderpass(VkDevice device, VkRenderPassCreateInfo &create_info, VkRenderPass *handle)
{
	return vkCreateRenderPass(device, &create_info, nullptr, handle);
}

inline VkResult create_vk_renderpass(VkDevice device, VkRenderPassCreateInfo2KHR &create_info, VkRenderPass *handle)
{
	return vkCreateRenderPass2KHR(device, &create_info, nullptr, handle);
}
}        // namespace

template <typename T>
std::vector<T> get_attachment_descriptions(const std::vector<Attachment> &attachments, const std::vector<LoadStoreInfo> &load_store_infos)
{
	std::vector<T> attachment_descriptions;

	for (size_t i = 0U; i < attachments.size(); ++i)
	{
		T attachment{};
		set_structure_type(attachment);

		attachment.format        = attachments[i].format;
		attachment.samples       = attachments[i].samples;
		attachment.initialLayout = attachments[i].initial_layout;
		attachment.finalLayout =
		    vkb::is_depth_format(attachment.format) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		if (i < load_store_infos.size())
		{
			attachment.loadOp         = load_store_infos[i].load_op;
			attachment.storeOp        = load_store_infos[i].store_op;
			attachment.stencilLoadOp  = load_store_infos[i].load_op;
			attachment.stencilStoreOp = load_store_infos[i].store_op;
		}

		attachment_descriptions.push_back(std::move(attachment));
	}

	return attachment_descriptions;
}

template <typename T_SubpassDescription, typename T_AttachmentDescription, typename T_AttachmentReference>
void set_attachment_layouts(std::vector<T_SubpassDescription> &subpass_descriptions, std::vector<T_AttachmentDescription> &attachment_descriptions)
{
	// Make the initial layout same as in the first subpass using that attachment
	for (auto &subpass : subpass_descriptions)
	{
		for (size_t k = 0U; k < subpass.colorAttachmentCount; ++k)
		{
			auto &reference = subpass.pColorAttachments[k];
			// Set it only if not defined yet
			if (attachment_descriptions[reference.attachment].initialLayout == VK_IMAGE_LAYOUT_UNDEFINED)
			{
				attachment_descriptions[reference.attachment].initialLayout = reference.layout;
			}
		}

		for (size_t k = 0U; k < subpass.inputAttachmentCount; ++k)
		{
			auto &reference = subpass.pInputAttachments[k];
			// Set it only if not defined yet
			if (attachment_descriptions[reference.attachment].initialLayout == VK_IMAGE_LAYOUT_UNDEFINED)
			{
				attachment_descriptions[reference.attachment].initialLayout = reference.layout;
			}
		}

		if (subpass.pDepthStencilAttachment)
		{
			auto &reference = *subpass.pDepthStencilAttachment;
			// Set it only if not defined yet
			if (attachment_descriptions[reference.attachment].initialLayout == VK_IMAGE_LAYOUT_UNDEFINED)
			{
				attachment_descriptions[reference.attachment].initialLayout = reference.layout;
			}
		}

		if (subpass.pResolveAttachments)
		{
			for (size_t k = 0U; k < subpass.colorAttachmentCount; ++k)
			{
				auto &reference = subpass.pResolveAttachments[k];
				// Set it only if not defined yet
				if (attachment_descriptions[reference.attachment].initialLayout == VK_IMAGE_LAYOUT_UNDEFINED)
				{
					attachment_descriptions[reference.attachment].initialLayout = reference.layout;
				}
			}
		}

		if (const auto depth_resolve = get_depth_resolve_reference(subpass))
		{
			// Set it only if not defined yet
			if (attachment_descriptions[depth_resolve->attachment].initialLayout == VK_IMAGE_LAYOUT_UNDEFINED)
			{
				attachment_descriptions[depth_resolve->attachment].initialLayout = depth_resolve->layout;
			}
		}
	}

	// Make the final layout same as the last subpass layout
	{
		auto &subpass = subpass_descriptions.back();

		for (size_t k = 0U; k < subpass.colorAttachmentCount; ++k)
		{
			const auto &reference = subpass.pColorAttachments[k];

			attachment_descriptions[reference.attachment].finalLayout = reference.layout;
		}

		for (size_t k = 0U; k < subpass.inputAttachmentCount; ++k)
		{
			const auto &reference = subpass.pInputAttachments[k];

			attachment_descriptions[reference.attachment].finalLayout = reference.layout;

			// Do not use depth attachment if used as input
			if (vkb::is_depth_format(attachment_descriptions[reference.attachment].format))
			{
				subpass.pDepthStencilAttachment = nullptr;
			}
		}

		if (subpass.pDepthStencilAttachment)
		{
			const auto &reference = *subpass.pDepthStencilAttachment;

			attachment_descriptions[reference.attachment].finalLayout = reference.layout;
		}

		if (subpass.pResolveAttachments)
		{
			for (size_t k = 0U; k < subpass.colorAttachmentCount; ++k)
			{
				const auto &reference = subpass.pResolveAttachments[k];

				attachment_descriptions[reference.attachment].finalLayout = reference.layout;
			}
		}

		if (const auto depth_resolve = get_depth_resolve_reference(subpass))
		{
			attachment_descriptions[depth_resolve->attachment].finalLayout = depth_resolve->layout;
		}
	}
}

/**
 * @brief Assuming there is only one depth attachment
 */
template <typename T_SubpassDescription, typename T_AttachmentDescription>
bool is_depth_a_dependency(std::vector<T_SubpassDescription> &subpass_descriptions, std::vector<T_AttachmentDescription> &attachment_descriptions)
{
	// More than 1 subpass uses depth
	if (std::ranges::count_if(subpass_descriptions,
	                          [](auto const &subpass) {
		                          return subpass.pDepthStencilAttachment != nullptr;
	                          }) > 1)
	{
		return true;
	}

	// Otherwise check if any uses depth as an input
	return std::ranges::any_of(
	    subpass_descriptions,
	    [&attachment_descriptions](auto const &subpass) {
		    return std::ranges::any_of(
		        std::span{subpass.pInputAttachments, subpass.inputAttachmentCount},
		        [&attachment_descriptions](auto const &reference) {
			        return vkb::is_depth_format(attachment_descriptions[reference.attachment].format);
		        });
	    });

	return false;
}

template <typename T>
std::vector<T> get_subpass_dependencies(const size_t subpass_count, bool depth_stencil_dependency)
{
	std::vector<T> dependencies{};

	if (subpass_count > 1)
	{
		for (uint32_t subpass_id = 0; subpass_id < to_u32(subpass_count - 1); ++subpass_id)
		{
			T color_dep{};
			color_dep.srcSubpass      = subpass_id;
			color_dep.dstSubpass      = subpass_id + 1;
			color_dep.srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			color_dep.dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			color_dep.srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			color_dep.dstAccessMask   = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			color_dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
			dependencies.push_back(color_dep);

			if (depth_stencil_dependency)
			{
				T depth_dep{};
				depth_dep.srcSubpass      = subpass_id;
				depth_dep.dstSubpass      = subpass_id + 1;
				depth_dep.srcStageMask    = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
				depth_dep.dstStageMask    = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				depth_dep.srcAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				depth_dep.dstAccessMask   = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				depth_dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
				dependencies.push_back(depth_dep);
			}
		}
	}

	return dependencies;
}

template <typename T>
T get_attachment_reference(const uint32_t attachment, const VkImageLayout layout)
{
	T reference{};
	set_structure_type(reference);

	reference.attachment = attachment;
	reference.layout     = layout;

	return reference;
}

template <typename T_SubpassDescription, typename T_AttachmentDescription, typename T_AttachmentReference, typename T_SubpassDependency, typename T_RenderPassCreateInfo>
void RenderPass::create_renderpass(const std::vector<Attachment> &attachments, const std::vector<LoadStoreInfo> &load_store_infos, const std::vector<SubpassInfo> &subpasses)
{
	if (attachments.size() != load_store_infos.size())
	{
		LOGW("Render Pass creation: size of attachment list and load/store info list does not match: {} vs {}", attachments.size(), load_store_infos.size());
	}

	auto attachment_descriptions = get_attachment_descriptions<T_AttachmentDescription>(attachments, load_store_infos);

	// Store attachments for every subpass
	std::vector<std::vector<T_AttachmentReference>> input_attachments{subpass_count};
	std::vector<std::vector<T_AttachmentReference>> color_attachments{subpass_count};
	std::vector<std::vector<T_AttachmentReference>> depth_stencil_attachments{subpass_count};
	std::vector<std::vector<T_AttachmentReference>> color_resolve_attachments{subpass_count};
	std::vector<std::vector<T_AttachmentReference>> depth_resolve_attachments{subpass_count};

	std::string new_debug_name{};
	const bool  needs_debug_name = get_debug_name().empty();
	if (needs_debug_name)
	{
		new_debug_name = fmt::format("RP with {} subpasses:\n", subpasses.size());
	}

	for (size_t i = 0; i < subpasses.size(); ++i)
	{
		auto &subpass = subpasses[i];

		if (needs_debug_name)
		{
			new_debug_name += fmt::format("\t[{}]: {}\n", i, subpass.debug_name);
		}

		// Fill color attachments references
		for (auto o_attachment : subpass.output_attachments)
		{
			auto  initial_layout = attachments[o_attachment].initial_layout == VK_IMAGE_LAYOUT_UNDEFINED ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : attachments[o_attachment].initial_layout;
			auto &description    = attachment_descriptions[o_attachment];
			if (!is_depth_format(description.format))
			{
				color_attachments[i].push_back(get_attachment_reference<T_AttachmentReference>(o_attachment, initial_layout));
			}
		}

		// Fill input attachments references
		for (auto i_attachment : subpass.input_attachments)
		{
			auto initial_layout = vkb::is_depth_format(attachments[i_attachment].format) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			input_attachments[i].push_back(get_attachment_reference<T_AttachmentReference>(i_attachment, initial_layout));
		}

		for (auto r_attachment : subpass.color_resolve_attachments)
		{
			auto initial_layout = attachments[r_attachment].initial_layout == VK_IMAGE_LAYOUT_UNDEFINED ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : attachments[r_attachment].initial_layout;
			color_resolve_attachments[i].push_back(get_attachment_reference<T_AttachmentReference>(r_attachment, initial_layout));
		}

		if (!subpass.disable_depth_stencil_attachment)
		{
			// Assumption: depth stencil attachment appears in the list before any depth stencil resolve attachment
			auto it = find_if(attachments.begin(), attachments.end(), [](const Attachment attachment) { return is_depth_format(attachment.format); });
			if (it != attachments.end())
			{
				auto i_depth_stencil = vkb::to_u32(std::distance(attachments.begin(), it));
				auto initial_layout  = it->initial_layout == VK_IMAGE_LAYOUT_UNDEFINED ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : it->initial_layout;
				depth_stencil_attachments[i].push_back(get_attachment_reference<T_AttachmentReference>(i_depth_stencil, initial_layout));

				if (subpass.depth_stencil_resolve_mode != VK_RESOLVE_MODE_NONE)
				{
					auto i_depth_stencil_resolve = subpass.depth_stencil_resolve_attachment;
					initial_layout               = attachments[i_depth_stencil_resolve].initial_layout == VK_IMAGE_LAYOUT_UNDEFINED ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : attachments[i_depth_stencil_resolve].initial_layout;
					depth_resolve_attachments[i].push_back(get_attachment_reference<T_AttachmentReference>(i_depth_stencil_resolve, initial_layout));
				}
			}
		}
	}

	std::vector<T_SubpassDescription> subpass_descriptions;
	subpass_descriptions.reserve(subpass_count);
	VkSubpassDescriptionDepthStencilResolveKHR depth_resolve{};
	for (size_t i = 0; i < subpasses.size(); ++i)
	{
		auto &subpass = subpasses[i];

		T_SubpassDescription subpass_description{};
		set_structure_type(subpass_description);
		subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		subpass_description.pInputAttachments    = input_attachments[i].empty() ? nullptr : input_attachments[i].data();
		subpass_description.inputAttachmentCount = to_u32(input_attachments[i].size());

		subpass_description.pColorAttachments    = color_attachments[i].empty() ? nullptr : color_attachments[i].data();
		subpass_description.colorAttachmentCount = to_u32(color_attachments[i].size());

		subpass_description.pResolveAttachments = color_resolve_attachments[i].empty() ? nullptr : color_resolve_attachments[i].data();

		subpass_description.pDepthStencilAttachment = nullptr;
		if (!depth_stencil_attachments[i].empty())
		{
			subpass_description.pDepthStencilAttachment = depth_stencil_attachments[i].data();

			if (!depth_resolve_attachments[i].empty())
			{
				// If the pNext list of VkSubpassDescription2 includes a VkSubpassDescriptionDepthStencilResolve structure,
				// then that structure describes multisample resolve operations for the depth/stencil attachment in a subpass
				depth_resolve.sType            = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE_KHR;
				depth_resolve.depthResolveMode = subpass.depth_stencil_resolve_mode;
				set_pointer_next(subpass_description, depth_resolve, depth_resolve_attachments[i][0]);

				auto &reference = depth_resolve_attachments[i][0];
				// Set it only if not defined yet
				if (attachment_descriptions[reference.attachment].initialLayout == VK_IMAGE_LAYOUT_UNDEFINED)
				{
					attachment_descriptions[reference.attachment].initialLayout = reference.layout;
				}
			}
		}

		subpass_descriptions.push_back(subpass_description);
	}

	// Default subpass
	if (subpasses.empty())
	{
		T_SubpassDescription subpass_description{};
		set_structure_type(subpass_description);
		subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		uint32_t default_depth_stencil_attachment{VK_ATTACHMENT_UNUSED};

		for (uint32_t k = 0U; k < to_u32(attachment_descriptions.size()); ++k)
		{
			if (vkb::is_depth_format(attachments[k].format))
			{
				if (default_depth_stencil_attachment == VK_ATTACHMENT_UNUSED)
				{
					default_depth_stencil_attachment = k;
				}
				continue;
			}

			color_attachments[0].push_back(get_attachment_reference<T_AttachmentReference>(k, VK_IMAGE_LAYOUT_GENERAL));
		}

		subpass_description.pColorAttachments = color_attachments[0].data();

		if (default_depth_stencil_attachment != VK_ATTACHMENT_UNUSED)
		{
			depth_stencil_attachments[0].push_back(get_attachment_reference<T_AttachmentReference>(default_depth_stencil_attachment, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL));

			subpass_description.pDepthStencilAttachment = depth_stencil_attachments[0].data();
		}

		subpass_descriptions.push_back(subpass_description);
	}

	set_attachment_layouts<T_SubpassDescription, T_AttachmentDescription, T_AttachmentReference>(subpass_descriptions, attachment_descriptions);

	color_output_count.reserve(subpass_count);
	for (size_t i = 0; i < subpass_count; i++)
	{
		color_output_count.push_back(to_u32(color_attachments[i].size()));
	}

	const auto &subpass_dependencies = get_subpass_dependencies<T_SubpassDependency>(subpass_count, is_depth_a_dependency(subpass_descriptions, attachment_descriptions));

	T_RenderPassCreateInfo create_info{};
	set_structure_type(create_info);
	create_info.attachmentCount = to_u32(attachment_descriptions.size());
	create_info.pAttachments    = attachment_descriptions.data();
	create_info.subpassCount    = to_u32(subpass_descriptions.size());
	create_info.pSubpasses      = subpass_descriptions.data();
	create_info.dependencyCount = to_u32(subpass_dependencies.size());
	create_info.pDependencies   = subpass_dependencies.data();

	auto result = create_vk_renderpass(get_device().get_handle(), create_info, &get_handle());

	if (result != VK_SUCCESS)
	{
		throw VulkanException{result, "Cannot create RenderPass"};
	}

	if (needs_debug_name)
	{
		set_debug_name(new_debug_name);
	}
}

RenderPass::RenderPass(vkb::core::DeviceC               &device,
                       const std::vector<Attachment>    &attachments,
                       const std::vector<LoadStoreInfo> &load_store_infos,
                       const std::vector<SubpassInfo>   &subpasses) :
    VulkanResource{VK_NULL_HANDLE, &device}, subpass_count{std::max<size_t>(1, subpasses.size())},        // At least 1 subpass
    color_output_count{}
{
	if (device.is_extension_enabled(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME))
	{
		create_renderpass<VkSubpassDescription2KHR, VkAttachmentDescription2KHR, VkAttachmentReference2KHR, VkSubpassDependency2KHR, VkRenderPassCreateInfo2KHR>(
		    attachments, load_store_infos, subpasses);
	}
	else
	{
		create_renderpass<VkSubpassDescription, VkAttachmentDescription, VkAttachmentReference, VkSubpassDependency, VkRenderPassCreateInfo>(
		    attachments, load_store_infos, subpasses);
	}
}

RenderPass::RenderPass(RenderPass &&other) :
    VulkanResource{std::move(other)},
    subpass_count{other.subpass_count},
    color_output_count{other.color_output_count}
{}

RenderPass::~RenderPass()
{
	// Destroy render pass
	if (has_device())
	{
		vkDestroyRenderPass(get_device().get_handle(), get_handle(), nullptr);
	}
}

const uint32_t RenderPass::get_color_output_count(uint32_t subpass_index) const
{
	return color_output_count[subpass_index];
}

VkExtent2D RenderPass::get_render_area_granularity() const
{
	VkExtent2D render_area_granularity = {};
	vkGetRenderAreaGranularity(get_device().get_handle(), get_handle(), &render_area_granularity);

	return render_area_granularity;
}
}        // namespace vkb
