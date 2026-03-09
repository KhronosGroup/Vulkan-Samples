/* Copyright (c) 2019-2026, Arm Limited and Contributors
 * Copyright (c) 2025-2026, NVIDIA CORPORATION. All rights reserved.
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

#include "common/hpp_vk_common.h"
#include "common/vk_common.h"
#include "core/command_buffer.h"
#include "rendering/hpp_render_target.h"
#include "rendering/render_target.h"
#include "rendering/subpass.h"
#include <vulkan/vulkan.hpp>

namespace vkb
{
namespace rendering
{
/**
 * @brief A RenderPipeline is a sequence of Subpass objects.
 * Subpass holds shaders and can draw the core::sg::Scene.
 * More subpasses can be added to the sequence if required.
 * For example, postprocessing can be implemented with two pipelines which
 * share render targets.
 *
 * GeometrySubpass -> Processes Scene for Shaders, use by itself if shader requires no lighting
 * ForwardSubpass -> Binds lights at the beginning of a GeometrySubpass to create Forward Rendering, should be used with most default shaders
 * LightingSubpass -> Holds a Global Light uniform, Can be combined with GeometrySubpass to create Deferred Rendering
 */
template <vkb::BindingType bindingType>
class RenderPipeline
{
  public:
	using ClearValueType      = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::ClearValue, VkClearValue>::type;
	using SubpassContentsType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::SubpassContents, VkSubpassContents>::type;

	using LoadStoreInfoType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::common::HPPLoadStoreInfo, vkb::LoadStoreInfo>::type;
	using RenderTargetType  = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::rendering::HPPRenderTarget, vkb::RenderTarget>::type;

  public:
	RenderPipeline(std::vector<std::unique_ptr<vkb::rendering::Subpass<bindingType>>> &&subpasses = {});
	virtual ~RenderPipeline() = default;

	RenderPipeline(const RenderPipeline &)            = delete;
	RenderPipeline(RenderPipeline &&)                 = default;
	RenderPipeline &operator=(const RenderPipeline &) = delete;
	RenderPipeline &operator=(RenderPipeline &&)      = default;

	/**
	 * @brief Appends a subpass to the pipeline
	 * @param subpass Subpass to append
	 */
	void add_subpass(std::unique_ptr<vkb::rendering::Subpass<bindingType>> &&subpass);

	/**
	 * @brief Record draw commands for each Subpass
	 */
	void draw(vkb::core::CommandBuffer<bindingType> &command_buffer, RenderTargetType &render_target, SubpassContentsType contents = {});

	/**
	 * @return Subpass currently being recorded, or the first one if drawing has not started
	 */
	std::unique_ptr<vkb::rendering::Subpass<bindingType>> &get_active_subpass();

	/**
	 * @return Clear values
	 */
	const std::vector<ClearValueType> &get_clear_value() const;

	/**
	 * @return Load store info
	 */
	const std::vector<LoadStoreInfoType> &get_load_store() const;

	std::vector<std::unique_ptr<vkb::rendering::Subpass<bindingType>>> &get_subpasses();

	/**
	 * @brief Prepares the subpasses
	 */
	void prepare();

	/**
	 * @param clear_values Clear values to set
	 */
	void set_clear_value(const std::vector<ClearValueType> &clear_values);

	/**
	 * @param load_store Load store info to set
	 */
	void set_load_store(const std::vector<LoadStoreInfoType> &load_store);

  private:
	void draw_impl(vkb::core::CommandBufferCpp     &command_buffer,
	               vkb::rendering::HPPRenderTarget &render_target,
	               vk::SubpassContents              contents);

  private:
	size_t                                                   active_subpass_index = 0;
	std::vector<vk::ClearValue>                              clear_value{vk::ClearColorValue{0.0f, 0.0f, 0.0f, 1.0f}, vk::ClearDepthStencilValue{0.0f, ~0U}};                                                    // Defaults for swapchain and depth attachment
	std::vector<vkb::common::HPPLoadStoreInfo>               load_store{{vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore}, {vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare}};        // Defaults for swapchain and depth attachment
	std::vector<std::unique_ptr<vkb::rendering::SubpassCpp>> subpasses;
};

using RenderPipelineC   = RenderPipeline<vkb::BindingType::C>;
using RenderPipelineCpp = RenderPipeline<vkb::BindingType::Cpp>;

// Member function definitions

template <vkb::BindingType bindingType>
RenderPipeline<bindingType>::RenderPipeline(std::vector<std::unique_ptr<vkb::rendering::Subpass<bindingType>>> &&subpasses_)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		subpasses = std::move(subpasses_);
	}
	else
	{
		subpasses = std::move(reinterpret_cast<std::vector<std::unique_ptr<vkb::rendering::SubpassCpp>> &&>(subpasses_));
	}

	prepare();
}

template <vkb::BindingType bindingType>
void RenderPipeline<bindingType>::add_subpass(std::unique_ptr<vkb::rendering::Subpass<bindingType>> &&subpass)
{
	subpass->prepare();
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		subpasses.emplace_back(std::move(subpass));
	}
	else
	{
		subpasses.push_back(std::move(reinterpret_cast<std::unique_ptr<vkb::rendering::SubpassCpp> &&>(subpass)));
	}
}

template <vkb::BindingType bindingType>
void RenderPipeline<bindingType>::draw(vkb::core::CommandBuffer<bindingType> &command_buffer, RenderTargetType &render_target, SubpassContentsType contents)
{
	assert(!subpasses.empty() && "Render pipeline should contain at least one sub-pass");

	// Pad clear values if they're less than render target attachments
	while (clear_value.size() < render_target.get_attachments().size())
	{
		clear_value.push_back({{0.0f, 0.0f, 0.0f, 1.0f}});
	}

	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		draw_impl(command_buffer, render_target, contents);
	}
	else
	{
		draw_impl(reinterpret_cast<vkb::core::CommandBufferCpp &>(command_buffer),
		          reinterpret_cast<vkb::rendering::HPPRenderTarget &>(render_target),
		          static_cast<vk::SubpassContents>(contents));
	}

	active_subpass_index = 0;
}

template <vkb::BindingType bindingType>
void RenderPipeline<bindingType>::draw_impl(vkb::core::CommandBufferCpp     &command_buffer,
                                            vkb::rendering::HPPRenderTarget &render_target,
                                            vk::SubpassContents              contents)
{
	for (size_t i = 0; i < subpasses.size(); ++i)
	{
		active_subpass_index = i;

		auto &subpass = subpasses[i];

		subpass->update_render_target_attachments(render_target);

		if (i == 0)
		{
			command_buffer.begin_render_pass(render_target, load_store, clear_value, subpasses, contents);
		}
		else
		{
			command_buffer.next_subpass();
		}

		if (contents != vk::SubpassContents::eSecondaryCommandBuffers)
		{
			if (subpass->get_debug_name().empty())
			{
				subpass->set_debug_name(fmt::format("RP subpass #{}", i));
			}
			ScopedDebugLabel subpass_debug_label{reinterpret_cast<vkb::core::CommandBufferC const &>(command_buffer), subpass->get_debug_name().c_str()};
		}

		subpass->draw(command_buffer);
	}
}

template <vkb::BindingType bindingType>
const std::vector<typename RenderPipeline<bindingType>::ClearValueType> &RenderPipeline<bindingType>::get_clear_value() const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return clear_value;
	}
	else
	{
		return reinterpret_cast<std::vector<VkClearValue> const &>(clear_value);
	}
}

template <vkb::BindingType bindingType>
const std::vector<typename RenderPipeline<bindingType>::LoadStoreInfoType> &RenderPipeline<bindingType>::get_load_store() const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return load_store;
	}
	else
	{
		return reinterpret_cast<std::vector<vkb::LoadStoreInfo> const &>(load_store);
	}
}

template <vkb::BindingType bindingType>
std::unique_ptr<vkb::rendering::Subpass<bindingType>> &RenderPipeline<bindingType>::get_active_subpass()
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return subpasses[active_subpass_index];
	}
	else
	{
		return reinterpret_cast<std::unique_ptr<vkb::rendering::SubpassC> &>(subpasses[active_subpass_index]);
	}
}

template <vkb::BindingType bindingType>
std::vector<std::unique_ptr<vkb::rendering::Subpass<bindingType>>> &RenderPipeline<bindingType>::get_subpasses()
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return subpasses;
	}
	else
	{
		return reinterpret_cast<std::vector<std::unique_ptr<vkb::rendering::SubpassC>> &>(subpasses);
	}
}

template <vkb::BindingType bindingType>
void RenderPipeline<bindingType>::prepare()
{
	for (auto &subpass : subpasses)
	{
		subpass->prepare();
	}
}

template <vkb::BindingType bindingType>
void RenderPipeline<bindingType>::set_clear_value(const std::vector<ClearValueType> &cv)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		clear_value = cv;
	}
	else
	{
		clear_value = reinterpret_cast<std::vector<vk::ClearValue> const &>(cv);
	}
}

template <vkb::BindingType bindingType>
void RenderPipeline<bindingType>::set_load_store(const std::vector<LoadStoreInfoType> &ls)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		load_store = ls;
	}
	else
	{
		load_store = reinterpret_cast<std::vector<vkb::common::HPPLoadStoreInfo> const &>(ls);
	}
}

}        // namespace rendering
}        // namespace vkb
