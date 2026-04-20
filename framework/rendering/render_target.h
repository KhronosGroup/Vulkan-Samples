/* Copyright (c) 2019-2026, Arm Limited and Contributors
 * Copyright (c) 2026, NVIDIA CORPORATION. All rights reserved.
 *
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

#include "common/hpp_error.h"
#include "common/hpp_vk_common.h"
#include "core/device.h"
#include "core/hpp_image.h"
#include "core/hpp_image_view.h"
#include "core/image.h"
#include <vulkan/vulkan_hash.hpp>        // provides std::hash specializations for Vulkan handle types, used in std::unordered_map and std::unordered_set

namespace vkb
{
namespace rendering
{
/**
 * @brief Description of render pass attachments.
 * Attachment descriptions can be used to automatically create render target images.
 */
template <vkb::BindingType bindingType>
struct Attachment
{
	using FormatType              = typename std::conditional<bindingType == BindingType::Cpp, vk::Format, VkFormat>::type;
	using ImageLayoutType         = typename std::conditional<bindingType == BindingType::Cpp, vk::ImageLayout, VkImageLayout>::type;
	using ImageUsageFlagsType     = typename std::conditional<bindingType == BindingType::Cpp, vk::ImageUsageFlags, VkImageUsageFlags>::type;
	using SampleCountFlagBitsType = typename std::conditional<bindingType == BindingType::Cpp, vk::SampleCountFlagBits, VkSampleCountFlagBits>::type;

  private:
	constexpr FormatType default_format() const
	{
		if constexpr (bindingType == BindingType::Cpp)
		{
			return vk::Format::eUndefined;
		}
		else
		{
			return VK_FORMAT_UNDEFINED;
		}
	}

	constexpr ImageLayoutType default_image_layout() const
	{
		if constexpr (bindingType == BindingType::Cpp)
		{
			return vk::ImageLayout::eUndefined;
		}
		else
		{
			return VK_IMAGE_LAYOUT_UNDEFINED;
		}
	}

	constexpr ImageUsageFlagsType default_image_usage_flags() const
	{
		if constexpr (bindingType == BindingType::Cpp)
		{
			return vk::ImageUsageFlagBits::eSampled;
		}
		else
		{
			return VK_IMAGE_USAGE_SAMPLED_BIT;
		}
	}

	constexpr SampleCountFlagBitsType default_sample_count_flag_bits() const
	{
		if constexpr (bindingType == BindingType::Cpp)
		{
			return vk::SampleCountFlagBits::e1;
		}
		else
		{
			return VK_SAMPLE_COUNT_1_BIT;
		}
	}

  public:
	FormatType              format         = default_format();
	SampleCountFlagBitsType samples        = default_sample_count_flag_bits();
	ImageUsageFlagsType     usage          = default_image_usage_flags();
	ImageLayoutType         initial_layout = default_image_layout();

	Attachment() = default;
	Attachment(FormatType format, SampleCountFlagBitsType samples, ImageUsageFlagsType usage) :
	    format{format},
	    samples{samples},
	    usage{usage}
	{
	}
};

using AttachmentC   = Attachment<vkb::BindingType::C>;
using AttachmentCpp = Attachment<vkb::BindingType::Cpp>;

/**
 * @brief RenderTarget contains three vectors for: core::Image, core::ImageView and Attachment.
 * The first two are Vulkan images and corresponding image views respectively.
 * Attachment (s) contain a description of the images, which has two main purposes:
 * - RenderPass creation only needs a list of Attachment (s), not the actual images, so we keep
 *   the minimum amount of information necessary
 * - Creation of a RenderTarget becomes simpler, because the caller can just ask for some
 *   Attachment (s) without having to create the images
 */
template <vkb::BindingType bindingType>
class RenderTarget
{
  public:
	using Extent2DType    = typename std::conditional<bindingType == BindingType::Cpp, vk::Extent2D, VkExtent2D>::type;
	using ImageLayoutType = typename std::conditional<bindingType == BindingType::Cpp, vk::ImageLayout, VkImageLayout>::type;

	using ImageType     = typename std::conditional<bindingType == BindingType::Cpp, vkb::core::HPPImage, vkb::core::Image>::type;
	using ImageViewType = typename std::conditional<bindingType == BindingType::Cpp, vkb::core::HPPImageView, vkb::core::ImageView>::type;

  public:
	using CreateFunc = std::function<std::unique_ptr<RenderTarget<bindingType>>(ImageType &&)>;

	static const CreateFunc DEFAULT_CREATE_FUNC;

	RenderTarget(std::vector<ImageType> &&images);
	RenderTarget(std::vector<ImageViewType> &&image_views);

	RenderTarget(const RenderTarget &)                          = delete;
	RenderTarget(RenderTarget &&)                               = delete;
	RenderTarget &operator=(const RenderTarget &other) noexcept = delete;
	RenderTarget &operator=(RenderTarget &&other) noexcept      = delete;

	std::vector<Attachment<bindingType>> const &get_attachments() const;
	Extent2DType const                         &get_extent() const;
	std::vector<uint32_t> const                &get_input_attachments() const;
	ImageLayoutType                             get_layout(uint32_t attachment) const;
	std::vector<uint32_t> const                &get_output_attachments() const;
	std::vector<ImageViewType> const           &get_views() const;

	/**
	 * @brief Sets the current input attachments overwriting the current ones
	 *        Should be set before beginning the render pass and before starting a new subpass
	 * @param input Set of attachment reference number to use as input
	 */
	void set_input_attachments(std::vector<uint32_t> const &input);

	void set_layout(uint32_t attachment, ImageLayoutType layout);

	/**
	 * @brief Sets the current output attachments overwriting the current ones
	 *        Should be set before beginning the render pass and before starting a new subpass
	 * @param output Set of attachment reference number to use as output
	 */
	void set_output_attachments(std::vector<uint32_t> const &output);

  private:
	static std::unique_ptr<RenderTarget<BindingType::Cpp>> default_create_func_impl(vkb::core::HPPImage &&swapchain_image);

	void init(std::vector<vkb::core::HPPImage> &&images);
	void init(std::vector<vkb::core::HPPImageView> &&image_views);

  private:
	vkb::core::DeviceCpp const          *device;
	vk::Extent2D                         extent;
	std::vector<AttachmentCpp>           attachments;
	std::vector<vkb::core::HPPImage>     images;
	std::vector<uint32_t>                input_attachments  = {};         // By default there are no input attachments
	std::vector<uint32_t>                output_attachments = {0};        // By default the output attachments is attachment 0
	std::vector<vkb::core::HPPImageView> views;
};

using RenderTargetC   = RenderTarget<vkb::BindingType::C>;
using RenderTargetCpp = RenderTarget<vkb::BindingType::Cpp>;

// Member function definitions

template <vkb::BindingType bindingType>
inline typename RenderTarget<bindingType>::CreateFunc const RenderTarget<bindingType>::DEFAULT_CREATE_FUNC = [](ImageType &&swapchain_image) -> std::unique_ptr<RenderTarget<bindingType>> {
	if constexpr (bindingType == BindingType::Cpp)
	{
		return default_create_func_impl(std::move(swapchain_image));
	}
	else
	{
		return std::unique_ptr<RenderTargetC>(reinterpret_cast<RenderTargetC *>(default_create_func_impl(std::move(reinterpret_cast<vkb::core::HPPImage &&>(swapchain_image))).release()));
	}
};

template <vkb::BindingType bindingType>
inline std::unique_ptr<RenderTargetCpp> RenderTarget<bindingType>::default_create_func_impl(vkb::core::HPPImage &&swapchain_image)
{
	vk::Format depth_format = vkb::common::get_suitable_depth_format(swapchain_image.get_device().get_gpu().get_handle());

	vkb::core::HPPImage depth_image{swapchain_image.get_device(), swapchain_image.get_extent(),
	                                depth_format,
	                                vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransientAttachment,
	                                VMA_MEMORY_USAGE_GPU_ONLY};

	std::vector<vkb::core::HPPImage> images;
	images.push_back(std::move(swapchain_image));
	images.push_back(std::move(depth_image));

	return std::make_unique<RenderTargetCpp>(std::move(images));
};

template <vkb::BindingType bindingType>
inline RenderTarget<bindingType>::RenderTarget(std::vector<ImageType> &&images)
{
	assert(!images.empty() && "Should specify at least 1 image");

	if constexpr (bindingType == BindingType::Cpp)
	{
		init(std::move(images));
	}
	else
	{
		init(reinterpret_cast<std::vector<vkb::core::HPPImage> &&>(images));
	}
}

template <vkb::BindingType bindingType>
inline void RenderTarget<bindingType>::init(std::vector<vkb::core::HPPImage> &&images_)
{
	device = &images_.back().get_device();
	images = std::move(images_);

	// Returns the image extent as a vk::Extent2D structure from a vk::Extent3D
	auto get_image_extent = [](const vkb::core::HPPImage &image) { return vk::Extent2D{image.get_extent().width, image.get_extent().height}; };

	// Constructs a set of unique image extents given a vector of images
	std::unordered_set<vk::Extent2D> unique_extent;
	std::ranges::transform(images, std::inserter(unique_extent, unique_extent.end()), get_image_extent);

	// Allow only one extent size for a render target
	if (unique_extent.size() != 1)
	{
		throw vkb::common::HPPVulkanException{vk::Result::eErrorInitializationFailed, "Extent size is not unique"};
	}

	extent = *unique_extent.begin();

	for (auto &image : images)
	{
		if (image.get_type() != vk::ImageType::e2D)
		{
			throw vkb::common::HPPVulkanException{vk::Result::eErrorInitializationFailed, "Image type is not 2D"};
		}

		views.emplace_back(image, vk::ImageViewType::e2D);

		attachments.emplace_back(image.get_format(), image.get_sample_count(), image.get_usage());
	}
}

template <vkb::BindingType bindingType>
inline RenderTarget<bindingType>::RenderTarget(std::vector<ImageViewType> &&image_views)
{
	assert(!views.empty() && "Should specify at least 1 image view");

	if constexpr (bindingType == BindingType::Cpp)
	{
		init(std::move(image_views));
	}
	else
	{
		init(reinterpret_cast<std::vector<vkb::core::HPPImageView> &&>(image_views));
	}
}

template <vkb::BindingType bindingType>
inline void RenderTarget<bindingType>::init(std::vector<vkb::core::HPPImageView> &&image_views)
{
	device = &image_views.back().get_image().get_device();
	views  = std::move(image_views);

	// Returns the extent of the base mip level pointed at by a view
	auto get_view_extent = [](const vkb::core::HPPImageView &view) {
		const vk::Extent3D mip0_extent = view.get_image().get_extent();
		const uint32_t     mip_level   = view.get_subresource_range().baseMipLevel;
		return vk::Extent2D{mip0_extent.width >> mip_level, mip0_extent.height >> mip_level};
	};

	// Constructs a set of unique image extents given a vector of image views;
	// allow only one extent size for a render target
	std::unordered_set<vk::Extent2D> unique_extent;
	std::ranges::transform(views, std::inserter(unique_extent, unique_extent.end()), get_view_extent);
	if (unique_extent.size() != 1)
	{
		throw vkb::common::HPPVulkanException{vk::Result::eErrorInitializationFailed, "Extent size is not unique"};
	}
	extent = *unique_extent.begin();

	for (auto &view : views)
	{
		const auto &image = view.get_image();
		attachments.emplace_back(AttachmentCpp{image.get_format(), image.get_sample_count(), image.get_usage()});
	}
}

template <vkb::BindingType bindingType>
inline std::vector<Attachment<bindingType>> const &RenderTarget<bindingType>::get_attachments() const
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return attachments;
	}
	else
	{
		return reinterpret_cast<std::vector<AttachmentC> const &>(attachments);
	}
}

template <vkb::BindingType bindingType>
inline typename RenderTarget<bindingType>::Extent2DType const &RenderTarget<bindingType>::get_extent() const
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return extent;
	}
	else
	{
		return reinterpret_cast<VkExtent2D const &>(extent);
	}
}

template <vkb::BindingType bindingType>
inline std::vector<uint32_t> const &RenderTarget<bindingType>::get_input_attachments() const
{
	return input_attachments;
}

template <vkb::BindingType bindingType>
inline typename RenderTarget<bindingType>::ImageLayoutType RenderTarget<bindingType>::get_layout(uint32_t attachment) const
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return attachments[attachment].initial_layout;
	}
	else
	{
		return static_cast<VkImageLayout>(attachments[attachment].initial_layout);
	}
}

template <vkb::BindingType bindingType>
inline std::vector<uint32_t> const &RenderTarget<bindingType>::get_output_attachments() const
{
	return output_attachments;
}

template <vkb::BindingType bindingType>
inline std::vector<typename RenderTarget<bindingType>::ImageViewType> const &RenderTarget<bindingType>::get_views() const
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return views;
	}
	else
	{
		return reinterpret_cast<std::vector<vkb::core::ImageView> const &>(views);
	}
}

template <vkb::BindingType bindingType>
inline void RenderTarget<bindingType>::set_input_attachments(std::vector<uint32_t> const &input)
{
	input_attachments = input;
}

template <vkb::BindingType bindingType>
inline void RenderTarget<bindingType>::set_layout(uint32_t attachment, ImageLayoutType layout)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		attachments[attachment].initial_layout = layout;
	}
	else
	{
		attachments[attachment].initial_layout = static_cast<vk::ImageLayout>(layout);
	}
}

template <vkb::BindingType bindingType>
inline void RenderTarget<bindingType>::set_output_attachments(std::vector<uint32_t> const &output)
{
	output_attachments = output;
}

}        // namespace rendering
}        // namespace vkb
