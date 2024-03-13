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

#include "image_compression_control.h"

#include "common/vk_common.h"
#include "gltf_loader.h"
#include "gui.h"
#include "platform/platform.h"
#include "rendering/postprocessing_renderpass.h"
#include "rendering/subpasses/forward_subpass.h"
#include "stats/stats.h"

ImageCompressionControlSample::ImageCompressionControlSample()
{
	// Extensions of interest in this sample (optional)
	add_device_extension(VK_EXT_IMAGE_COMPRESSION_CONTROL_EXTENSION_NAME, true);
	add_device_extension(VK_EXT_IMAGE_COMPRESSION_CONTROL_SWAPCHAIN_EXTENSION_NAME, true);

	// Extension dependency requirements (given that instance API version is 1.0.0)
	add_instance_extension(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME, true);

	auto &config = get_configuration();

	// Batch mode will test the toggle between different compression modes
	config.insert<vkb::IntSetting>(0, static_cast<int>(gui_target_compression), 0);
	config.insert<vkb::IntSetting>(1, static_cast<int>(gui_target_compression), 1);
	config.insert<vkb::IntSetting>(2, static_cast<int>(gui_target_compression), 2);
}

void ImageCompressionControlSample::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	if (gpu.is_extension_supported(VK_EXT_IMAGE_COMPRESSION_CONTROL_EXTENSION_NAME))
	{
		auto &compression_control_features = gpu.request_extension_features<VkPhysicalDeviceImageCompressionControlFeaturesEXT>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_COMPRESSION_CONTROL_FEATURES_EXT);

		compression_control_features.imageCompressionControl = VK_TRUE;
	}

	if (gpu.is_extension_supported(VK_EXT_IMAGE_COMPRESSION_CONTROL_SWAPCHAIN_EXTENSION_NAME))
	{
		auto &compression_control_swapchain_features = gpu.request_extension_features<VkPhysicalDeviceImageCompressionControlSwapchainFeaturesEXT>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_COMPRESSION_CONTROL_SWAPCHAIN_FEATURES_EXT);

		compression_control_swapchain_features.imageCompressionControlSwapchain = VK_TRUE;
	}
}

bool ImageCompressionControlSample::prepare(const vkb::ApplicationOptions &options)
{
	if (!VulkanSample::prepare(options))
	{
		return false;
	}

	load_scene("scenes/sponza/Sponza01.gltf");

	auto &camera_node = vkb::add_free_camera(get_scene(), "main_camera", get_render_context().get_surface_extent());
	camera            = dynamic_cast<vkb::sg::PerspectiveCamera *>(&camera_node.get_component<vkb::sg::Camera>());

	vkb::ShaderSource scene_vs("base.vert");
	vkb::ShaderSource scene_fs("base.frag");
	auto              scene_subpass = std::make_unique<vkb::ForwardSubpass>(get_render_context(), std::move(scene_vs), std::move(scene_fs), get_scene(), *camera);
	scene_subpass->set_output_attachments({(int) Attachments::Color});

	// Forward rendering pass
	auto render_pipeline = std::make_unique<vkb::RenderPipeline>();
	render_pipeline->add_subpass(std::move(scene_subpass));
	render_pipeline->set_load_store(scene_load_store);
	set_render_pipeline(std::move(render_pipeline));

	// Post-processing pass (chromatic aberration)
	vkb::ShaderSource postprocessing_vs("postprocessing/postprocessing.vert");
	postprocessing_pipeline = std::make_unique<vkb::PostProcessingPipeline>(get_render_context(), std::move(postprocessing_vs));
	postprocessing_pipeline->add_pass().add_subpass(vkb::ShaderSource("postprocessing/chromatic_aberration.frag"));

	// Trigger recreation of Swapchain and render targets, with initial compression parameters
	update_render_targets();

	get_stats().request_stats({vkb::StatIndex::frame_times,
	                           vkb::StatIndex::gpu_ext_write_bytes});

	create_gui(*window, &get_stats());

	// Hide GUI compression options other than default if the required extension is not supported
	if (!get_device().is_enabled(VK_EXT_IMAGE_COMPRESSION_CONTROL_EXTENSION_NAME))
	{
		for (int i = 0; i < (int) TargetCompression::Count; i++)
		{
			if (static_cast<TargetCompression>(i) != TargetCompression::Default)
			{
				gui_skip_compression_values.insert(static_cast<TargetCompression>(i));
			}
		}
	}

	return true;
}

void ImageCompressionControlSample::create_render_context()
{
	/**
	 * The framework expects a prioritized list of surface formats. For this sample, include
	 * only those that can be compressed.
	 */
	std::vector<VkSurfaceFormatKHR> surface_formats_that_support_compression;

	/**
	 * To query for compression support, VK_EXT_image_compression_control_swapchain allows to
	 * add a VkImageCompressionPropertiesEXT to the pNext chain of VkSurfaceFormat2KHR when
	 * calling vkGetPhysicalDeviceSurfaceFormats2KHR.
	 * See the implementation of this helper function in vkb::Swapchain.
	 */
	auto surface_compression_properties_list = vkb::Swapchain::query_supported_fixed_rate_compression(get_device(), get_surface());

	LOGI("The following surface formats support compression:");
	for (auto &surface_compression_properties : surface_compression_properties_list)
	{
		if (surface_compression_properties.compression_properties.imageCompressionFixedRateFlags != VK_IMAGE_COMPRESSION_FIXED_RATE_NONE_EXT)
		{
			VkSurfaceFormatKHR surface_format = {surface_compression_properties.surface_format.surfaceFormat.format,
			                                     surface_compression_properties.surface_format.surfaceFormat.colorSpace};

			LOGI("  \t{}:\t{}",
			     vkb::to_string(surface_format),
			     vkb::image_compression_fixed_rate_flags_to_string(surface_compression_properties.compression_properties.imageCompressionFixedRateFlags));

			surface_formats_that_support_compression.push_back(surface_format);
		}
	}

	if (surface_formats_that_support_compression.empty())
	{
		LOGI("  \tNo surface formats support fixed-rate compression");

		// Fall-back to default surface format priority list
		VulkanSample::create_render_context();
	}
	else
	{
		// Filter default list to those formats that support compression
		std::vector<VkSurfaceFormatKHR> new_surface_priority_list;

		for (size_t i = 0; i < get_surface_priority_list().size(); i++)
		{
			auto it = std::find_if(surface_formats_that_support_compression.begin(), surface_formats_that_support_compression.end(),
			                       [&](VkSurfaceFormatKHR &sf) { return get_surface_priority_list()[i].format == sf.format &&
				                                                        get_surface_priority_list()[i].colorSpace == sf.colorSpace; });
			if (it != surface_formats_that_support_compression.end())
			{
				new_surface_priority_list.push_back(*it);

				surface_formats_that_support_compression.erase(it);
			}
		}

		// In case there is no overlap, append any formats that support compression but were not in the default list
		for (auto &remaining_format : surface_formats_that_support_compression)
		{
			new_surface_priority_list.push_back(remaining_format);
		}

		VulkanSample<vkb::BindingType::C>::create_render_context(new_surface_priority_list);
	}

	/**
	 * At this point, a Swapchain has been created using the first supported format in the list above. Save a list of
	 * its corresponding supported compression rates (if any).
	 */
	const auto &selected_surface_format = get_render_context().get_swapchain().get_surface_format();
	for (size_t i = 0; i < surface_compression_properties_list.size(); i++)
	{
		if (selected_surface_format.format == surface_compression_properties_list[i].surface_format.surfaceFormat.format &&
		    selected_surface_format.colorSpace == surface_compression_properties_list[i].surface_format.surfaceFormat.colorSpace)
		{
			supported_fixed_rate_flags_swapchain = vkb::fixed_rate_compression_flags_to_vector(
			    surface_compression_properties_list[i].compression_properties.imageCompressionFixedRateFlags);
			break;
		}
	}
}

void ImageCompressionControlSample::prepare_render_context()
{
	get_render_context().prepare(1, std::bind(&ImageCompressionControlSample::create_render_target, this, std::placeholders::_1));
}

std::unique_ptr<vkb::RenderTarget> ImageCompressionControlSample::create_render_target(vkb::core::Image &&swapchain_image)
{
	/**
	 * The render passes will use 3 attachments: Color, Depth and Swapchain.
	 * This sample allows to control the compression of the color and Swapchain attachments.
	 * The Swapchain has already been created by the RenderContext. Here, create color and depth images.
	 */
	color_image_info.imageType = VK_IMAGE_TYPE_2D;
	color_image_info.extent    = swapchain_image.get_extent();
	color_image_info.tiling    = VK_IMAGE_TILING_OPTIMAL;
	color_image_info.usage     = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	// The first time this function is called, choose a compressible format for the color attachment.
	if (VK_FORMAT_UNDEFINED == color_image_info.format)
	{
		// Query fixed-rate compression support for a few candidate formats
		std::vector<VkFormat> format_list{VK_FORMAT_R8G8B8_UNORM, VK_FORMAT_R8G8B8_SNORM, VK_FORMAT_R8G8B8_SRGB,
		                                  VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_B8G8R8_SNORM, VK_FORMAT_B8G8R8_SRGB,
		                                  VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_SNORM, VK_FORMAT_R8G8B8A8_SRGB,
		                                  VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SNORM, VK_FORMAT_B8G8R8A8_SRGB};

		VkFormat chosen_format{VK_FORMAT_UNDEFINED};
		if (get_device().is_enabled(VK_EXT_IMAGE_COMPRESSION_CONTROL_EXTENSION_NAME))
		{
			for (auto &candidate_format : format_list)
			{
				color_image_info.format = candidate_format;

				/**
				 * To query for compression support, VK_EXT_image_compression_control allows to
				 * add a VkImageCompressionPropertiesEXT to the pNext chain of VkImageFormatProperties2 when
				 * calling vkGetPhysicalDeviceImageFormatProperties2KHR.
				 * See the implementation of this helper function in vkb::core::Image.
				 */
				VkImageCompressionPropertiesEXT supported_compression_properties = vkb::query_supported_fixed_rate_compression(get_device().get_gpu().get_handle(), color_image_info);

				auto fixed_rate_flags = vkb::fixed_rate_compression_flags_to_vector(supported_compression_properties.imageCompressionFixedRateFlags);

				VkImageFormatProperties format_properties;
				auto                    result = vkGetPhysicalDeviceImageFormatProperties(get_device().get_gpu().get_handle(),
				                                                                          color_image_info.format,
				                                                                          color_image_info.imageType,
				                                                                          color_image_info.tiling,
				                                                                          color_image_info.usage,
				                                                                          0,        // no create flags
				                                                                          &format_properties);

				// Pick the first format that supports at least 2 or more levels of fixed-rate compression, otherwise
				// pick the first format that supports at least 1 level
				if (result != VK_ERROR_FORMAT_NOT_SUPPORTED &&
				    fixed_rate_flags.size() > 0)
				{
					if ((VK_FORMAT_UNDEFINED == chosen_format) || (fixed_rate_flags.size() > 1))
					{
						chosen_format                    = candidate_format;
						supported_fixed_rate_flags_color = fixed_rate_flags;
					}

					if (fixed_rate_flags.size() > 1)
					{
						break;
					}
				}
			}
		}

		// Fallback if no fixed-rate compressible format was found
		color_image_info.format = (VK_FORMAT_UNDEFINED != chosen_format) ? chosen_format : swapchain_image.get_format();
		LOGI("Chosen color format: {}", vkb::to_string(color_image_info.format));

		// Hide GUI fixed-rate compression option if chosen format does not support it
		if (supported_fixed_rate_flags_color.empty())
		{
			gui_skip_compression_values.insert(TargetCompression::FixedRate);

			LOGW("Color image does not support fixed-rate compression. Possible reasons:");
			LOGW("\t- Its format may not be supported (format = {})", vkb::to_string(color_image_info.format));
			if (color_image_info.usage & VK_IMAGE_USAGE_STORAGE_BIT)
			{
				LOGW("\t- It is a storage image (usage = {})", vkb::image_usage_to_string(color_image_info.usage));
			}
			if (color_image_info.samples > 1)
			{
				LOGW("\t- It is a multi-sampled image (sample count = {})", vkb::to_string(color_image_info.samples));
			}
		}
	}

	vkb::core::Image depth_image{get_device(),
	                             vkb::core::ImageBuilder(color_image_info.extent)
	                                 .with_format(vkb::get_suitable_depth_format(get_device().get_gpu().get_handle()))
	                                 .with_usage(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
	                                 .with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY)};

	vkb::core::ImageBuilder color_image_builder(color_image_info.extent);
	color_image_builder.with_format(color_image_info.format)
	    .with_usage(color_image_info.usage)
	    .with_tiling(color_image_info.tiling)
	    .with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY);

	// Prepare compression control structure
	VkImageCompressionControlEXT color_compression_control{VK_STRUCTURE_TYPE_IMAGE_COMPRESSION_CONTROL_EXT};

	if (VK_IMAGE_COMPRESSION_DEFAULT_EXT != compression_flag)
	{
		color_compression_control.flags = compression_flag;

		if (VK_IMAGE_COMPRESSION_FIXED_RATE_EXPLICIT_EXT == compression_flag &&
		    VK_IMAGE_COMPRESSION_FIXED_RATE_NONE_EXT != compression_fixed_rate_flag_color)
		{
			color_compression_control.compressionControlPlaneCount = 1;
			color_compression_control.pFixedRateFlags              = &compression_fixed_rate_flag_color;
		}

		color_image_builder.with_extension<VkImageCompressionControlEXT>(color_compression_control);
	}

	vkb::core::Image color_image{get_device(), color_image_builder};

	if (VK_IMAGE_COMPRESSION_FIXED_RATE_EXPLICIT_EXT == compression_flag)
	{
		/**
		 * To verify that the requested compression was indeed applied, the framework core::Image class
		 * provides a helper function that uses the new structures introduced by VK_EXT_image_compression_control.
		 * It calls vkGetImageSubresourceLayout2EXT with a VkImageSubresource2EXT that includes a
		 * VkImageCompressionPropertiesEXT structure in its pNext chain.
		 * This structure is then filled with the applied compression flags.
		 */
		LOGI("Applied fixed-rate compression for color ({}): {}", vkb::to_string(color_image_info.format),
		     vkb::image_compression_fixed_rate_flags_to_string(color_image.get_applied_compression().imageCompressionFixedRateFlags));
	}

	// Update memory footprint values in GUI (displayed in MB)
	const float bytes_in_mb = 1024 * 1024;
	footprint_swapchain     = swapchain_image.get_image_required_size() / bytes_in_mb;
	footprint_color         = color_image.get_image_required_size() / bytes_in_mb;

	scene_load_store.clear();
	std::vector<vkb::core::Image> images;

	// Attachment 0 - Swapchain - Not used in the scene render pass, output of postprocessing
	images.push_back(std::move(swapchain_image));
	scene_load_store.push_back({VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE});

	// Attachment 1 - Attachments::Depth - Transient, used only in the scene render pass
	images.push_back(std::move(depth_image));
	scene_load_store.push_back({VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE});

	// Attachment 2 - Attachments::Color - Output of the scene render pass, input of postprocessing
	images.push_back(std::move(color_image));
	scene_load_store.push_back({VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE});

	return std::make_unique<vkb::RenderTarget>(std::move(images));
}

void ImageCompressionControlSample::update(float delta_time)
{
	elapsed_time += delta_time;

	if ((gui_target_compression != last_gui_target_compression) ||
	    (gui_fixed_rate_compression_level != last_gui_fixed_rate_compression_level))
	{
		update_render_targets();

		last_gui_target_compression           = gui_target_compression;
		last_gui_fixed_rate_compression_level = gui_fixed_rate_compression_level;
	}

	VulkanSample::update(delta_time);
}

void ImageCompressionControlSample::update_render_targets()
{
	/**
	 * Define the compression flags that will be used to select the compression
	 * level of the color and Swapchain images. In the framework, this will be
	 * handled by the vkb::core::Image and vkb::Swapchain constructors.
	 * The extensions introduce a new VkImageCompressionControlEXT struct, which
	 * collects compression settings, and that can be provided to the pNext
	 * list of VkImageCreateInfo and VkSwapchainCreateInfoKHR.
	 */
	switch (gui_target_compression)
	{
		case TargetCompression::FixedRate:
		{
			compression_flag = VK_IMAGE_COMPRESSION_FIXED_RATE_EXPLICIT_EXT;
			break;
		}
		case TargetCompression::None:
		{
			compression_flag = VK_IMAGE_COMPRESSION_DISABLED_EXT;
			break;
		}
		case TargetCompression::Default:
		default:
		{
			compression_flag = VK_IMAGE_COMPRESSION_DEFAULT_EXT;
			break;
		}
	}

	/**
	 * Select the minimum (higher compression) or maximum (lower compression) bitrate supported,
	 * which might be different for the color and the Swapchain.
	 */
	auto fixed_rate_compression_level     = static_cast<FixedRateCompressionLevel>(gui_fixed_rate_compression_level);
	compression_fixed_rate_flag_color     = select_fixed_rate_compression_flag(supported_fixed_rate_flags_color, fixed_rate_compression_level);
	compression_fixed_rate_flag_swapchain = select_fixed_rate_compression_flag(supported_fixed_rate_flags_swapchain, fixed_rate_compression_level);

	// Recreate the Swapchain, which also triggers recreation of render targets
	get_device().wait_idle();
	get_render_context().update_swapchain(compression_flag, compression_fixed_rate_flag_swapchain);
}

VkImageCompressionFixedRateFlagBitsEXT ImageCompressionControlSample::select_fixed_rate_compression_flag(
    std::vector<VkImageCompressionFixedRateFlagBitsEXT> &supported_fixed_rate_flags,
    FixedRateCompressionLevel                            compression_level)
{
	if (!supported_fixed_rate_flags.empty())
	{
		switch (compression_level)
		{
			case FixedRateCompressionLevel::High:
			{
				return supported_fixed_rate_flags.front();
			}
			case FixedRateCompressionLevel::Low:
			{
				return supported_fixed_rate_flags.back();
			}
			default:
			{
				LOGW("Unknown fixed-rate compression level {}", static_cast<int>(compression_level));
				break;
			}
		}
	}

	return VK_IMAGE_COMPRESSION_FIXED_RATE_NONE_EXT;
}

void ImageCompressionControlSample::render(vkb::CommandBuffer &command_buffer)
{
	// Scene (forward rendering) pass
	VulkanSample::render(command_buffer);

	command_buffer.end_render_pass();

	/**
	 * Post processing pass, which applies a simple chromatic aberration effect.
	 * The effect is animated, using elapsed time, for two reasons:
	 * 1. Allows to visualize the scene with and without the effect.
	 * 2. Reduces the effect of transaction elimination, a useful feature that
	 *    reduces bandwidth but may hide the bandwidth benefits of compression,
	 *    the focus of this sample.
	 */
	auto &postprocessing_pass = postprocessing_pipeline->get_pass(0);
	postprocessing_pass.set_uniform_data(sin(elapsed_time));

	auto &postprocessing_subpass = postprocessing_pass.get_subpass(0);
	postprocessing_subpass.bind_sampled_image("color_sampler", (int) Attachments::Color);

	postprocessing_pipeline->draw(command_buffer, get_render_context().get_active_frame().get_render_target());
}

namespace
{

/**
 * @brief Helper function to generate a GUI drop-down options menu
 */
template <typename T>
inline T generate_combo(T current_value, const char *combo_label, const std::unordered_map<T, std::string> &enum_to_string, float item_width, const std::set<T> *skip_values = nullptr)
{
	ImGui::PushItemWidth(item_width);

	T new_enum_value = current_value;

	const auto &search_it      = enum_to_string.find(current_value);
	const char *selected_value = search_it != enum_to_string.end() ? search_it->second.c_str() : "";

	if (ImGui::BeginCombo(combo_label, selected_value))
	{
		for (const auto &it : enum_to_string)
		{
			if (skip_values && std::find(skip_values->begin(), skip_values->end(), it.first) != skip_values->end())
			{
				continue;
			}

			const bool is_selected = it.first == current_value;

			if (ImGui::Selectable(it.second.c_str(), is_selected))
			{
				new_enum_value = it.first;
			}

			if (is_selected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	return new_enum_value;
}
}        // namespace

void ImageCompressionControlSample::draw_gui()
{
	const bool landscape = camera->get_aspect_ratio() > 1.0f;
	uint32_t   lines     = 3;

	if (landscape)
	{
		lines--;
	}

	get_gui().show_options_window(
	    [&]() {
		    const auto window_width = ImGui::GetWindowWidth();

		    /**
		     * Select compression scheme from those available. Some options may be hidden if the extension(s) are not supported,
		     * or if the chosen color format does not support fixed-rate compression.
		     */
		    ImGui::Text("Compression:");
		    ImGui::SameLine();

		    const TargetCompression compression = generate_combo(gui_target_compression, "##compression",
		                                                         {{TargetCompression::FixedRate, "Fixed-rate"}, {TargetCompression::None, "None"}, {TargetCompression::Default, "Default"}},
		                                                         window_width * 0.2f,
		                                                         &gui_skip_compression_values);

		    gui_target_compression = compression;

		    if (compression == TargetCompression::FixedRate && supported_fixed_rate_flags_color.size() > 1)
		    {
			    /**
			     * Select level of fixed-rate compression from those available. It is assumed that the Swapchain can only be compressed
			     * if the color attachment can be compressed too, given that we select similar formats and the Swapchain compression control
			     * extension depends on the general image compression control extension.
			     */
			    ImGui::SameLine();
			    ImGui::Text("Level:");

			    ImGui::SameLine();
			    const FixedRateCompressionLevel compression_level = generate_combo(gui_fixed_rate_compression_level, "##compression-level",
			                                                                       {{FixedRateCompressionLevel::High, "High"}, {FixedRateCompressionLevel::Low, "Low"}},
			                                                                       window_width * 0.2f);

			    gui_fixed_rate_compression_level = compression_level;
		    }

		    if (landscape)
		    {
			    ImGui::SameLine();
		    }

		    if (gui_skip_compression_values.size() >= (int) TargetCompression::Count - 1)
		    {
			    // Single or no compression options available on this device
			    ImGui::Text("(Extensions are not supported)");
		    }
		    else
		    {
			    // Indicate if the Swapchain compression matches that of the color attachment
			    ImGui::Text("(Swapchain is %s affected)", get_render_context().get_swapchain().get_applied_compression() == compression_flag ? "also" : "not");
		    }

		    /**
		     * Display the memory footprint of the configurable targets, which will be lower if fixed-rate compression is selected.
		     */
		    ImGui::Text("Color attachment (%.1f MB), Swapchain (%.1f MB)", footprint_color, footprint_swapchain);
	    },
	    lines);
}

std::unique_ptr<vkb::VulkanSample<vkb::BindingType::C>> create_image_compression_control()
{
	return std::make_unique<ImageCompressionControlSample>();
}
