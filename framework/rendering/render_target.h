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

#pragma once

#include "common/helpers.h"
#include "common/vk_common.h"
#include "core/image.h"
#include "core/image_view.h"

namespace vkb
{
class Device;

/**
 * @brief Description of render pass attachments.
 * Attachment descriptions can be used to automatically create render target images.
 */
struct Attachment
{
	VkFormat format{VK_FORMAT_UNDEFINED};

	VkSampleCountFlagBits samples{VK_SAMPLE_COUNT_1_BIT};

	VkImageUsageFlags usage{VK_IMAGE_USAGE_SAMPLED_BIT};

	VkImageLayout initial_layout{VK_IMAGE_LAYOUT_UNDEFINED};

	Attachment() = default;

	Attachment(VkFormat format, VkSampleCountFlagBits samples, VkImageUsageFlags usage);
};

/**
 * @brief RenderTarget contains three vectors for: core::Image, core::ImageView and Attachment.
 * The first two are Vulkan images and corresponding image views respectively.
 * Attachment (s) contain a description of the images, which has two main purposes:
 * - RenderPass creation only needs a list of Attachment (s), not the actual images, so we keep
 *   the minimum amount of information necessary
 * - Creation of a RenderTarget becomes simpler, because the caller can just ask for some
 *   Attachment (s) without having to create the images
 */
class RenderTarget
{
  public:
	using CreateFunc = std::function<std::unique_ptr<RenderTarget>(core::Image &&)>;

	static const CreateFunc DEFAULT_CREATE_FUNC;

	explicit RenderTarget(std::vector<core::Image> &&images);

	explicit RenderTarget(std::vector<core::ImageView> &&image_views);

	RenderTarget(const RenderTarget &) = delete;

	RenderTarget(RenderTarget &&) = delete;

	RenderTarget &operator=(const RenderTarget &other) noexcept = delete;

	RenderTarget &operator=(RenderTarget &&other) noexcept = delete;

	const VkExtent2D &get_extent() const;

	const std::vector<core::ImageView> &get_views() const;

	const std::vector<Attachment> &get_attachments() const;

	/**
	 * @brief Sets the current input attachments overwriting the current ones
	 *        Should be set before beginning the render pass and before starting a new subpass
	 * @param input Set of attachment reference number to use as input
	 */
	void set_input_attachments(std::vector<uint32_t> &input);

	const std::vector<uint32_t> &get_input_attachments() const;

	/**
	 * @brief Sets the current output attachments overwriting the current ones
	 *        Should be set before beginning the render pass and before starting a new subpass
	 * @param output Set of attachment reference number to use as output
	 */
	void set_output_attachments(std::vector<uint32_t> &output);

	const std::vector<uint32_t> &get_output_attachments() const;

	void set_layout(uint32_t attachment, VkImageLayout layout);

	VkImageLayout get_layout(uint32_t attachment) const;

  private:
	Device &device;

	VkExtent2D extent{};

	std::vector<core::Image> images;

	std::vector<core::ImageView> views;

	std::vector<Attachment> attachments;

	/// By default there are no input attachments
	std::vector<uint32_t> input_attachments = {};

	/// By default the output attachments is attachment 0
	std::vector<uint32_t> output_attachments = {0};
};
}        // namespace vkb
