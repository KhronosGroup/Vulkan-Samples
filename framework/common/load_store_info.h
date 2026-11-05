/* Copyright (c) 2026, NVIDIA CORPORATION. All rights reserved.
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

#include "vk_common.h"
#include <vulkan/vulkan.hpp>

namespace vkb
{
namespace common
{
/**
 * @brief Load and store info for a render pass attachment.
 */
template <vkb::BindingType bindingType>
struct LoadStoreInfo
{
	using AttachmentLoadOpType =
	    typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::AttachmentLoadOp, VkAttachmentLoadOp>::type;
	using AttachmentStoreOpType = typename std::
	    conditional<bindingType == vkb::BindingType::Cpp, vk::AttachmentStoreOp, VkAttachmentStoreOp>::type;

  private:
	constexpr AttachmentLoadOpType load_op_default() const
	{
		if constexpr (bindingType == vkb::BindingType::Cpp)
		{
			return vk::AttachmentLoadOp::eClear;
		}
		else
		{
			return static_cast<VkAttachmentLoadOp>(LoadStoreInfo<vkb::BindingType::Cpp>{}.load_op);
		}
	}

	constexpr AttachmentStoreOpType store_op_default() const
	{
		if constexpr (bindingType == vkb::BindingType::Cpp)
		{
			return vk::AttachmentStoreOp::eStore;
		}
		else
		{
			return static_cast<VkAttachmentStoreOp>(LoadStoreInfo<vkb::BindingType::Cpp>{}.store_op);
		}
	}

  public:
	AttachmentLoadOpType  load_op  = load_op_default();
	AttachmentStoreOpType store_op = store_op_default();
};

using LoadStoreInfoC   = LoadStoreInfo<vkb::BindingType::C>;
using LoadStoreInfoCpp = LoadStoreInfo<vkb::BindingType::Cpp>;
}        // namespace common
}        // namespace vkb
