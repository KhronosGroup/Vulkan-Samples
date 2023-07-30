/* Copyright (c) 2022-2023, NVIDIA CORPORATION. All rights reserved.
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

#include "core/hpp_swapchain.h"

#include "common/logging.h"
#include "core/hpp_device.h"

namespace vkb
{
namespace
{
template <class T>
constexpr const T &clamp(const T &v, const T &lo, const T &hi)
{
	return (v < lo) ? lo : ((hi < v) ? hi : v);
}

vk::Extent2D choose_extent(vk::Extent2D        request_extent,
                           const vk::Extent2D &min_image_extent,
                           const vk::Extent2D &max_image_extent,
                           const vk::Extent2D &current_extent)
{
	if (current_extent.width == 0xFFFFFFFF)
	{
		return request_extent;
	}

	if (request_extent.width < 1 || request_extent.height < 1)
	{
		LOGW("(HPPSwapchain) Image extent ({}, {}) not supported. Selecting ({}, {}).",
		     request_extent.width,
		     request_extent.height,
		     current_extent.width,
		     current_extent.height);
		return current_extent;
	}

	request_extent.width  = clamp(request_extent.width, min_image_extent.width, max_image_extent.width);
	request_extent.height = clamp(request_extent.height, min_image_extent.height, max_image_extent.height);

	return request_extent;
}

vk::PresentModeKHR choose_present_mode(vk::PresentModeKHR                     request_present_mode,
                                       const std::vector<vk::PresentModeKHR> &available_present_modes,
                                       const std::vector<vk::PresentModeKHR> &present_mode_priority_list)
{
	// Try to find the requested present mode in the available present modes
	auto const present_mode_it = std::find(available_present_modes.begin(), available_present_modes.end(), request_present_mode);
	if (present_mode_it == available_present_modes.end())
	{
		// If the requested present mode isn't found, then try to find a mode from the priority list
		auto const chosen_present_mode_it =
		    std::find_if(present_mode_priority_list.begin(),
		                 present_mode_priority_list.end(),
		                 [&available_present_modes](vk::PresentModeKHR present_mode) { return std::find(available_present_modes.begin(), available_present_modes.end(), present_mode) != available_present_modes.end(); });

		// If nothing found, always default to FIFO
		vk::PresentModeKHR const chosen_present_mode = (chosen_present_mode_it != present_mode_priority_list.end()) ? *chosen_present_mode_it : vk::PresentModeKHR::eFifo;

		LOGW("(HPPSwapchain) Present mode '{}' not supported. Selecting '{}'.", vk::to_string(request_present_mode), vk::to_string(chosen_present_mode));
		return chosen_present_mode;
	}
	else
	{
		LOGI("(HPPSwapchain) Present mode selected: {}", to_string(request_present_mode));
		return request_present_mode;
	}
}

vk::SurfaceFormatKHR choose_surface_format(const vk::SurfaceFormatKHR               requested_surface_format,
                                           const std::vector<vk::SurfaceFormatKHR> &available_surface_formats,
                                           const std::vector<vk::SurfaceFormatKHR> &surface_format_priority_list)
{
	// Try to find the requested surface format in the available surface formats
	auto const surface_format_it = std::find(available_surface_formats.begin(), available_surface_formats.end(), requested_surface_format);

	// If the requested surface format isn't found, then try to request a format from the priority list
	if (surface_format_it == available_surface_formats.end())
	{
		auto const chosen_surface_format_it =
		    std::find_if(surface_format_priority_list.begin(),
		                 surface_format_priority_list.end(),
		                 [&available_surface_formats](vk::SurfaceFormatKHR surface_format) { return std::find(available_surface_formats.begin(), available_surface_formats.end(), surface_format) != available_surface_formats.end(); });

		// If nothing found, default to the first available format
		vk::SurfaceFormatKHR const &chosen_surface_format = (chosen_surface_format_it != surface_format_priority_list.end()) ? *chosen_surface_format_it : available_surface_formats[0];

		LOGW("(HPPSwapchain) Surface format ({}) not supported. Selecting ({}).",
		     vk::to_string(requested_surface_format.format) + ", " + vk::to_string(requested_surface_format.colorSpace),
		     vk::to_string(chosen_surface_format.format) + ", " + vk::to_string(chosen_surface_format.colorSpace));
		return chosen_surface_format;
	}
	else
	{
		LOGI("(HPPSwapchain) Surface format selected: {}",
		     vk::to_string(requested_surface_format.format) + ", " + vk::to_string(requested_surface_format.colorSpace));
		return requested_surface_format;
	}
}

vk::SurfaceTransformFlagBitsKHR choose_transform(vk::SurfaceTransformFlagBitsKHR request_transform,
                                                 vk::SurfaceTransformFlagsKHR    supported_transform,
                                                 vk::SurfaceTransformFlagBitsKHR current_transform)
{
	if (request_transform & supported_transform)
	{
		return request_transform;
	}

	LOGW("(HPPSwapchain) Surface transform '{}' not supported. Selecting '{}'.", vk::to_string(request_transform), vk::to_string(current_transform));
	return current_transform;
}

vk::CompositeAlphaFlagBitsKHR choose_composite_alpha(vk::CompositeAlphaFlagBitsKHR request_composite_alpha,
                                                     vk::CompositeAlphaFlagsKHR    supported_composite_alpha)
{
	if (request_composite_alpha & supported_composite_alpha)
	{
		return request_composite_alpha;
	}

	static const std::vector<vk::CompositeAlphaFlagBitsKHR> composite_alpha_priority_list = {vk::CompositeAlphaFlagBitsKHR::eOpaque,
	                                                                                         vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
	                                                                                         vk::CompositeAlphaFlagBitsKHR::ePostMultiplied,
	                                                                                         vk::CompositeAlphaFlagBitsKHR::eInherit};

	auto const chosen_composite_alpha_it =
	    std::find_if(composite_alpha_priority_list.begin(),
	                 composite_alpha_priority_list.end(),
	                 [&supported_composite_alpha](vk::CompositeAlphaFlagBitsKHR composite_alpha) { return composite_alpha & supported_composite_alpha; });
	if (chosen_composite_alpha_it == composite_alpha_priority_list.end())
	{
		throw std::runtime_error("No compatible composite alpha found.");
	}
	else
	{
		LOGW("(HPPSwapchain) Composite alpha '{}' not supported. Selecting '{}.", vk::to_string(request_composite_alpha), vk::to_string(*chosen_composite_alpha_it));
		return *chosen_composite_alpha_it;
	}
}

bool validate_format_feature(vk::ImageUsageFlagBits image_usage, vk::FormatFeatureFlags supported_features)
{
	return (image_usage != vk::ImageUsageFlagBits::eStorage) || (supported_features & vk::FormatFeatureFlagBits::eStorageImage);
}

std::set<vk::ImageUsageFlagBits> choose_image_usage(const std::set<vk::ImageUsageFlagBits> &requested_image_usage_flags,
                                                    vk::ImageUsageFlags                     supported_image_usage,
                                                    vk::FormatFeatureFlags                  supported_features)
{
	std::set<vk::ImageUsageFlagBits> validated_image_usage_flags;
	for (auto flag : requested_image_usage_flags)
	{
		if ((flag & supported_image_usage) && validate_format_feature(flag, supported_features))
		{
			validated_image_usage_flags.insert(flag);
		}
		else
		{
			LOGW("(HPPSwapchain) Image usage ({}) requested but not supported.", vk::to_string(flag));
		}
	}

	if (validated_image_usage_flags.empty())
	{
		// Pick the first format from list of defaults, if supported
		static const std::vector<vk::ImageUsageFlagBits> image_usage_priority_list = {
		    vk::ImageUsageFlagBits::eColorAttachment, vk::ImageUsageFlagBits::eStorage, vk::ImageUsageFlagBits::eSampled, vk::ImageUsageFlagBits::eTransferDst};

		auto const priority_list_it =
		    std::find_if(image_usage_priority_list.begin(),
		                 image_usage_priority_list.end(),
		                 [&supported_image_usage, &supported_features](auto const image_usage) { return ((image_usage & supported_image_usage) && validate_format_feature(image_usage, supported_features)); });
		if (priority_list_it != image_usage_priority_list.end())
		{
			validated_image_usage_flags.insert(*priority_list_it);
		}
	}

	if (validated_image_usage_flags.empty())
	{
		throw std::runtime_error("No compatible image usage found.");
	}
	else
	{
		// Log image usage flags used
		std::string usage_list;
		for (vk::ImageUsageFlagBits image_usage : validated_image_usage_flags)
		{
			usage_list += to_string(image_usage) + " ";
		}
		LOGI("(HPPSwapchain) Image usage flags: {}", usage_list);
	}

	return validated_image_usage_flags;
}

vk::ImageUsageFlags composite_image_flags(std::set<vk::ImageUsageFlagBits> &image_usage_flags)
{
	vk::ImageUsageFlags image_usage;
	for (auto flag : image_usage_flags)
	{
		image_usage |= flag;
	}
	return image_usage;
}
}        // namespace

namespace core
{
HPPSwapchain::HPPSwapchain(HPPSwapchain &old_swapchain, const vk::Extent2D &extent) :
    HPPSwapchain{old_swapchain.device,
                 old_swapchain.surface,
                 old_swapchain.properties.present_mode,
                 old_swapchain.present_mode_priority_list,
                 old_swapchain.surface_format_priority_list,
                 extent,
                 old_swapchain.properties.image_count,
                 old_swapchain.properties.pre_transform,
                 old_swapchain.image_usage_flags,
                 old_swapchain.get_handle()}
{}

HPPSwapchain::HPPSwapchain(HPPSwapchain &old_swapchain, const uint32_t image_count) :
    HPPSwapchain{old_swapchain.device,
                 old_swapchain.surface,
                 old_swapchain.properties.present_mode,
                 old_swapchain.present_mode_priority_list,
                 old_swapchain.surface_format_priority_list,
                 old_swapchain.properties.extent,
                 image_count,
                 old_swapchain.properties.pre_transform,
                 old_swapchain.image_usage_flags,
                 old_swapchain.get_handle()}
{}

HPPSwapchain::HPPSwapchain(HPPSwapchain &old_swapchain, const std::set<vk::ImageUsageFlagBits> &image_usage_flags) :
    HPPSwapchain{old_swapchain.device,
                 old_swapchain.surface,
                 old_swapchain.properties.present_mode,
                 old_swapchain.present_mode_priority_list,
                 old_swapchain.surface_format_priority_list,
                 old_swapchain.properties.extent,
                 old_swapchain.properties.image_count,
                 old_swapchain.properties.pre_transform,
                 image_usage_flags,
                 old_swapchain.get_handle()}
{}

HPPSwapchain::HPPSwapchain(HPPSwapchain &old_swapchain, const vk::Extent2D &extent, const vk::SurfaceTransformFlagBitsKHR transform) :
    HPPSwapchain{old_swapchain.device,
                 old_swapchain.surface,
                 old_swapchain.properties.present_mode,
                 old_swapchain.present_mode_priority_list,
                 old_swapchain.surface_format_priority_list,
                 extent,
                 old_swapchain.properties.image_count,
                 transform,
                 old_swapchain.image_usage_flags,
                 old_swapchain.get_handle()}
{}

HPPSwapchain::HPPSwapchain(HPPDevice                               &device,
                           vk::SurfaceKHR                           surface,
                           const vk::PresentModeKHR                 present_mode,
                           const std::vector<vk::PresentModeKHR>   &present_mode_priority_list,
                           const std::vector<vk::SurfaceFormatKHR> &surface_format_priority_list,
                           const vk::Extent2D                      &extent,
                           const uint32_t                           image_count,
                           const vk::SurfaceTransformFlagBitsKHR    transform,
                           const std::set<vk::ImageUsageFlagBits>  &image_usage_flags,
                           vk::SwapchainKHR                         old_swapchain) :
    device{device},
    surface{surface}
{
	this->present_mode_priority_list   = present_mode_priority_list;
	this->surface_format_priority_list = surface_format_priority_list;

	surface_formats = device.get_gpu().get_handle().getSurfaceFormatsKHR(surface);
	LOGI("Surface supports the following surface formats:");
	for (auto &surface_format : surface_formats)
	{
		LOGI("  \t{}", vk::to_string(surface_format.format) + ", " + vk::to_string(surface_format.colorSpace));
	}

	present_modes = device.get_gpu().get_handle().getSurfacePresentModesKHR(surface);
	LOGI("Surface supports the following present modes:");
	for (auto &present_mode : present_modes)
	{
		LOGI("  \t{}", to_string(present_mode));
	}

	// Chose best properties based on surface capabilities
	vk::SurfaceCapabilitiesKHR const surface_capabilities = device.get_gpu().get_handle().getSurfaceCapabilitiesKHR(surface);

	vk::FormatProperties const format_properties = device.get_gpu().get_handle().getFormatProperties(properties.surface_format.format);
	this->image_usage_flags                      = choose_image_usage(image_usage_flags, surface_capabilities.supportedUsageFlags, format_properties.optimalTilingFeatures);

	properties.image_count     = clamp(image_count,
	                                   surface_capabilities.minImageCount,
                                   surface_capabilities.maxImageCount ? surface_capabilities.maxImageCount : std::numeric_limits<uint32_t>::max());
	properties.extent          = choose_extent(extent, surface_capabilities.minImageExtent, surface_capabilities.maxImageExtent, surface_capabilities.currentExtent);
	properties.array_layers    = 1;
	properties.surface_format  = choose_surface_format(properties.surface_format, surface_formats, surface_format_priority_list);
	properties.image_usage     = composite_image_flags(this->image_usage_flags);
	properties.pre_transform   = choose_transform(transform, surface_capabilities.supportedTransforms, surface_capabilities.currentTransform);
	properties.composite_alpha = choose_composite_alpha(vk::CompositeAlphaFlagBitsKHR::eInherit, surface_capabilities.supportedCompositeAlpha);

	properties.old_swapchain = old_swapchain;
	properties.present_mode  = present_mode;

	// Revalidate the present mode and surface format
	properties.present_mode   = choose_present_mode(properties.present_mode, present_modes, present_mode_priority_list);
	properties.surface_format = choose_surface_format(properties.surface_format, surface_formats, surface_format_priority_list);

	vk::SwapchainCreateInfoKHR const create_info({},
	                                             surface,
	                                             properties.image_count,
	                                             properties.surface_format.format,
	                                             properties.surface_format.colorSpace,
	                                             properties.extent,
	                                             properties.array_layers,
	                                             properties.image_usage,
	                                             {},
	                                             {},
	                                             properties.pre_transform,
	                                             properties.composite_alpha,
	                                             properties.present_mode,
	                                             {},
	                                             properties.old_swapchain);

	handle = device.get_handle().createSwapchainKHR(create_info);

	images = device.get_handle().getSwapchainImagesKHR(handle);
}

HPPSwapchain::~HPPSwapchain()
{
	if (handle)
	{
		device.get_handle().destroySwapchainKHR(handle);
	}
}

HPPSwapchain::HPPSwapchain(HPPSwapchain &&other) :
    device{other.device},
    surface{std::exchange(other.surface, nullptr)},
    handle{std::exchange(other.handle, nullptr)},
    images{std::exchange(other.images, {})},
    surface_formats{std::exchange(other.surface_formats, {})},
    present_modes{std::exchange(other.present_modes, {})},
    properties{std::exchange(other.properties, {})},
    present_mode_priority_list{std::exchange(other.present_mode_priority_list, {})},
    surface_format_priority_list{std::exchange(other.surface_format_priority_list, {})},
    image_usage_flags{std::move(other.image_usage_flags)}
{}

bool HPPSwapchain::is_valid() const
{
	return !!handle;
}

HPPDevice const &HPPSwapchain::get_device() const
{
	return device;
}

vk::SwapchainKHR HPPSwapchain::get_handle() const
{
	return handle;
}

std::pair<vk::Result, uint32_t> HPPSwapchain::acquire_next_image(vk::Semaphore image_acquired_semaphore, vk::Fence fence) const
{
	vk::ResultValue<uint32_t> rv = device.get_handle().acquireNextImageKHR(handle, std::numeric_limits<uint64_t>::max(), image_acquired_semaphore, fence);
	return std::make_pair(rv.result, rv.value);
}

const vk::Extent2D &HPPSwapchain::get_extent() const
{
	return properties.extent;
}

vk::Format HPPSwapchain::get_format() const
{
	return properties.surface_format.format;
}

const std::vector<vk::Image> &HPPSwapchain::get_images() const
{
	return images;
}

vk::SurfaceTransformFlagBitsKHR HPPSwapchain::get_transform() const
{
	return properties.pre_transform;
}

vk::SurfaceKHR HPPSwapchain::get_surface() const
{
	return surface;
}

vk::ImageUsageFlags HPPSwapchain::get_usage() const
{
	return properties.image_usage;
}

vk::PresentModeKHR HPPSwapchain::get_present_mode() const
{
	return properties.present_mode;
}
}        // namespace core
}        // namespace vkb
