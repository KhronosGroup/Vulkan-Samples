/* Copyright (c) 2024, Arm Limited and Contributors
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

#include "rendering/postprocessing_pipeline.h"
#include "rendering/render_pipeline.h"
#include "scene_graph/components/camera.h"
#include "scene_graph/components/perspective_camera.h"
#include "vulkan_sample.h"

/**
 * @brief Image Compression Control Sample
 *
 * This sample shows how to use the extensions VK_EXT_image_compression_control
 * and VK_EXT_image_compression_control_swapchain to select between
 * different levels of image compression.
 *
 * The UI shows the impact compression has on image size and bandwidth,
 * illustrating the benefits of fixed-rate (visually lossless) compression.
 */
class ImageCompressionControlSample : public vkb::VulkanSample<vkb::BindingType::C>
{
  public:
	ImageCompressionControlSample();

	virtual bool prepare(const vkb::ApplicationOptions &options) override;

	virtual void request_gpu_features(vkb::PhysicalDevice &gpu) override;

	virtual ~ImageCompressionControlSample() = default;

	virtual void update(float delta_time) override;

	virtual void render(vkb::CommandBuffer &command_buffer) override;

	void draw_gui() override;

  private:
	vkb::sg::PerspectiveCamera *camera{nullptr};

	virtual void prepare_render_context() override;

	virtual void create_render_context() override;

	std::unique_ptr<vkb::RenderTarget> create_render_target(vkb::core::Image &&swapchain_image);

	enum class Attachments : int
	{
		Swapchain = 0,
		Depth     = 1,
		Color     = 2,
	};

	/**
	 * @brief Postprocessing pipeline
	 * In this sample, a postprocessing pass applies a simple chromatic aberration effect
	 * to the output of the forward rendering pass. Since this color output needs to
	 * be saved to main memory, it is a simple use case for compressing the image and
	 * saving bandwidth and memory footprint, as illustrated in the sample.
	 */
	std::unique_ptr<vkb::PostProcessingPipeline> postprocessing_pipeline{};

	/**
	 * @brief Load/store operations of the forward rendering pass attachments
	 * Used to specify that the color output must be stored to main memory.
	 */
	std::vector<vkb::LoadStoreInfo> scene_load_store{};

	/**
	 * @brief Color image parameters
	 * These are the properties of the color output image.
	 */
	VkImageCreateInfo color_image_info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};

	/**
	 * @brief Compression scheme
	 * By default, many implementations support lossless compression.
	 * Some also support fixed-rate compression, which is generally visually lossless.
	 * This flag can also be used to disable all compression, which is not recommended.
	 */
	VkImageCompressionFlagsEXT compression_flag{VK_IMAGE_COMPRESSION_DEFAULT_EXT};

	/**
	 * @brief Possible compression schemes
	 */
	enum class TargetCompression : int
	{
		Default   = 0,
		FixedRate = 1,
		None      = 2,

		// Enum value count
		Count = 3
	};

	/**
	 * @brief Compression rate for the color image
	 * If fixed-rate compression is selected, this specifies the bitrate per component.
	 */
	VkImageCompressionFixedRateFlagsEXT compression_fixed_rate_flag_color{};

	/**
	 * @brief Compression rate for the Swapchain image
	 */
	VkImageCompressionFixedRateFlagsEXT compression_fixed_rate_flag_swapchain{};

	/**
	 * @brief Supported values for compression rate
	 * These depend on the properties (format and usage) of the color image.
	 */
	std::vector<VkImageCompressionFixedRateFlagBitsEXT> supported_fixed_rate_flags_color{};

	/**
	 * @brief Supported values for compression rate
	 * These depend on the properties (format and usage) of the Swapchain image.
	 */
	std::vector<VkImageCompressionFixedRateFlagBitsEXT> supported_fixed_rate_flags_swapchain{};

	/**
	 * @brief Possible fixed-rate compression levels
	 */
	enum FixedRateCompressionLevel : int
	{
		High = 0,        // Highest compression available (smallest bitrate)
		Low  = 1,        // Lowest compression available (largest bitrate)
	};

	/**
	 * @brief Selects the compression bitrate given a list of supported values and a compression level
	 */
	VkImageCompressionFixedRateFlagBitsEXT select_fixed_rate_compression_flag(std::vector<VkImageCompressionFixedRateFlagBitsEXT> &supported_fixed_rate_flags,
	                                                                          FixedRateCompressionLevel                            compression_level);

	/**
	 * @brief Size (in MB) of the color image (output of the forward rendering pass)
	 */
	float footprint_color{};

	/**
	 * @brief Size (in MB) of the Swapchain image (output of the postprocessing pass)
	 */
	float footprint_swapchain{};

	/**
	 * @brief Time
	 * Used to animate the chromatic aberration effect.
	 */
	float elapsed_time{};

	/**
	 * @brief Updates compression settings based on user GUI input.
	 *        Triggers recreation of the Swapchain and other render targets.
	 */
	void update_render_targets();

	/**
	 * @brief Unsupported compression schemes
	 * Used to reduce the GUI controls if, for instance, the required
	 * extensions are not supported by the device.
	 */
	std::set<TargetCompression> gui_skip_compression_values;

	/* Helpers for managing GUI input */

	int gui_target_compression{(int) TargetCompression::Default};

	int last_gui_target_compression{gui_target_compression};

	int gui_fixed_rate_compression_level{(int) FixedRateCompressionLevel::High};

	int last_gui_fixed_rate_compression_level{gui_fixed_rate_compression_level};
};

std::unique_ptr<vkb::VulkanSample<vkb::BindingType::C>> create_image_compression_control();
