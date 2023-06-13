/* Copyright (c) 2019-2022, Arm Limited and Contributors
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

#include "direct_window.h"

#include "core/instance.h"

#include "platform/headless_window.h"
#include "platform/unix/direct_window.h"

namespace vkb
{
namespace
{
static KeyCode key_map[] = {
    KeyCode::Unknown,
    KeyCode::Unknown,
    KeyCode::Unknown,
    KeyCode::Unknown,
    KeyCode::Unknown,
    KeyCode::Unknown,
    KeyCode::Unknown,
    KeyCode::Unknown,
    KeyCode::Backspace,
    KeyCode::Tab,
    KeyCode::Unknown,
    KeyCode::Unknown,
    KeyCode::Unknown,
    KeyCode::Enter,
    KeyCode::Unknown,
    KeyCode::Unknown,
    KeyCode::Unknown,
    KeyCode::Unknown,
    KeyCode::Unknown,
    KeyCode::Unknown,
    KeyCode::Unknown,
    KeyCode::Unknown,
    KeyCode::Unknown,
    KeyCode::Unknown,
    KeyCode::Unknown,
    KeyCode::Unknown,
    KeyCode::Unknown,
    KeyCode::Escape,
    KeyCode::Unknown,
    KeyCode::Unknown,
    KeyCode::Unknown,
    KeyCode::Unknown,
    KeyCode::Space,
    KeyCode::_1,
    KeyCode::Apostrophe,
    KeyCode::Backslash,
    KeyCode::_4,
    KeyCode::_5,
    KeyCode::_7,
    KeyCode::Apostrophe,
    KeyCode::_9,
    KeyCode::_0,
    KeyCode::_8,
    KeyCode::Equal,
    KeyCode::Comma,
    KeyCode::Minus,
    KeyCode::Period,
    KeyCode::Slash,
    KeyCode::_0,
    KeyCode::_1,
    KeyCode::_2,
    KeyCode::_3,
    KeyCode::_4,
    KeyCode::_5,
    KeyCode::_6,
    KeyCode::_7,
    KeyCode::_8,
    KeyCode::_9,
    KeyCode::Semicolon,
    KeyCode::Semicolon,
    KeyCode::Comma,
    KeyCode::Equal,
    KeyCode::Period,
    KeyCode::Slash,
    KeyCode::_2,
    KeyCode::A,
    KeyCode::B,
    KeyCode::C,
    KeyCode::D,
    KeyCode::E,
    KeyCode::F,
    KeyCode::G,
    KeyCode::H,
    KeyCode::I,
    KeyCode::J,
    KeyCode::K,
    KeyCode::L,
    KeyCode::M,
    KeyCode::N,
    KeyCode::O,
    KeyCode::P,
    KeyCode::Q,
    KeyCode::R,
    KeyCode::S,
    KeyCode::T,
    KeyCode::U,
    KeyCode::V,
    KeyCode::W,
    KeyCode::X,
    KeyCode::Y,
    KeyCode::Z,
    KeyCode::LeftBracket,
    KeyCode::Backslash,
    KeyCode::RightBracket,
    KeyCode::_6,
    KeyCode::Minus,
    KeyCode::GraveAccent,
    KeyCode::A,
    KeyCode::B,
    KeyCode::C,
    KeyCode::D,
    KeyCode::E,
    KeyCode::F,
    KeyCode::G,
    KeyCode::H,
    KeyCode::I,
    KeyCode::J,
    KeyCode::K,
    KeyCode::L,
    KeyCode::M,
    KeyCode::N,
    KeyCode::O,
    KeyCode::P,
    KeyCode::Q,
    KeyCode::R,
    KeyCode::S,
    KeyCode::T,
    KeyCode::U,
    KeyCode::V,
    KeyCode::W,
    KeyCode::X,
    KeyCode::Y,
    KeyCode::Z,
    KeyCode::LeftBracket,
    KeyCode::Backslash,
    KeyCode::RightBracket,
    KeyCode::GraveAccent,
    KeyCode::Backspace};

static KeyCode map_multichar_key(int tty_fd, KeyCode initial)
{
	static std::map<std::string, KeyCode> mc_map;        // Static for one-time-init
	if (mc_map.size() == 0)
	{
		mc_map["[A"]   = KeyCode::Up;
		mc_map["[B"]   = KeyCode::Down;
		mc_map["[C"]   = KeyCode::Right;
		mc_map["[D"]   = KeyCode::Left;
		mc_map["[2~"]  = KeyCode::Insert;
		mc_map["[3~"]  = KeyCode::DelKey;
		mc_map["[5~"]  = KeyCode::PageUp;
		mc_map["[6~"]  = KeyCode::PageDown;
		mc_map["[H"]   = KeyCode::Home;
		mc_map["[F"]   = KeyCode::End;
		mc_map["OP"]   = KeyCode::F1;
		mc_map["OQ"]   = KeyCode::F2;
		mc_map["OR"]   = KeyCode::F3;
		mc_map["OS"]   = KeyCode::F4;
		mc_map["[15~"] = KeyCode::F5;
		mc_map["[17~"] = KeyCode::F6;
		mc_map["[18~"] = KeyCode::F7;
		mc_map["[19~"] = KeyCode::F8;
		mc_map["[20~"] = KeyCode::F9;
		mc_map["[21~"] = KeyCode::F10;
		mc_map["[23~"] = KeyCode::F11;
		mc_map["[24~"] = KeyCode::F12;
	}

	char        key;
	std::string buf;

	while (::read(tty_fd, &key, 1) == 1)
	{
		buf += key;
	}

	if (buf.size() == 0)
	{
		return initial;        // Nothing new read, just return the initial char
	}

	// Is it a code we recognise?
	auto iter = mc_map.find(buf);
	if (iter != mc_map.end())
	{
		return iter->second;
	}

	return KeyCode::Unknown;
}

}        // namespace
DirectWindow::DirectWindow(Platform *platform, const Window::Properties &properties) :
    Window(properties),
    platform{platform}
{
	// Setup tty for reading keyboard from console
	if ((tty_fd = open("/dev/tty", O_RDWR | O_NDELAY)) > 0)
	{
		tcgetattr(tty_fd, &termio_prev);
		tcgetattr(tty_fd, &termio);
		cfmakeraw(&termio);
		termio.c_lflag |= ISIG;
		termio.c_oflag |= OPOST | ONLCR;
		termio.c_cc[VMIN]  = 1;
		termio.c_cc[VTIME] = 0;

		if (tcsetattr(tty_fd, TCSANOW, &termio) == -1)
			LOGW("Failed to set attribs for '/dev/tty'");
	}

	platform->set_focus(true);
}

DirectWindow::~DirectWindow()
{
	if (tty_fd > 0)
	{
		tcsetattr(tty_fd, TCSANOW, &termio_prev);
	}
}

VkSurfaceKHR DirectWindow::create_surface(Instance &instance)
{
	return create_surface(instance.get_handle(), instance.get_first_gpu().get_handle());
}

// Local structure for holding display candidate information
struct Candidate
{
	VkDisplayKHR                  display;
	VkDisplayPropertiesKHR        display_props;
	VkDisplayModePropertiesKHR    mode;
	VkDisplayPlaneCapabilitiesKHR caps;
	uint32_t                      plane_index;
	uint32_t                      stack_index;
};

// Helper template to get a vector of properties using the Vulkan idiom of
// querying size, then querying data
template <typename T, typename F, typename... Targs>
static std::vector<T> get_props(F func, Targs... args)
{
	std::vector<T> result;
	uint32_t       count;

	VK_CHECK(func(args..., &count, nullptr));
	if (count > 0)
	{
		result.resize(count);
		VK_CHECK(func(args..., &count, result.data()));
	}
	return result;
}

// Find all valid display candidates in the system
static std::vector<Candidate> find_display_candidates(VkPhysicalDevice phys_dev, Window::Mode window_mode)
{
	// Possible display config candidates
	std::vector<Candidate> candidates;

	// Find all the displays connected to this platform
	auto display_properties = get_props<VkDisplayPropertiesKHR>(vkGetPhysicalDeviceDisplayPropertiesKHR, phys_dev);

	// Find all the display planes
	auto plane_properties = get_props<VkDisplayPlanePropertiesKHR>(vkGetPhysicalDeviceDisplayPlanePropertiesKHR, phys_dev);

	// Build a list of candidates
	for (uint32_t plane_index = 0; plane_index < plane_properties.size(); plane_index++)
	{
		const VkDisplayPlanePropertiesKHR &plane_props = plane_properties[plane_index];

		// Find the list of displays compatible with the plane
		auto supported_displays = get_props<VkDisplayKHR>(vkGetDisplayPlaneSupportedDisplaysKHR, phys_dev, plane_index);

		for (const auto &display : supported_displays)
		{
			auto props = std::find_if(display_properties.cbegin(), display_properties.cend(),
			                          [display](const VkDisplayPropertiesKHR &p) { return p.display == display; });
			if (props == display_properties.end())
			{
				continue;
			}

			// Cannot use if already on another display
			if (plane_props.currentDisplay != VK_NULL_HANDLE && plane_props.currentDisplay != display)
			{
				continue;
			}

			// Cannot use if identity transform is unsupported
			if ((props->supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) == 0)
			{
				continue;
			}

			// Find all the display modes for the display
			auto modes = get_props<VkDisplayModePropertiesKHR>(vkGetDisplayModePropertiesKHR, phys_dev, display);

			for (auto mode : modes)
			{
				// Query capabilities of this mode/plane combination
				VkDisplayPlaneCapabilitiesKHR caps;
				VK_CHECK(vkGetDisplayPlaneCapabilitiesKHR(phys_dev, mode.displayMode, plane_index, &caps));

				// Check for opaque alpha support
				if ((caps.supportedAlpha & VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR) == 0)
				{
					continue;
				}

				// Eliminate modes that don't fit the plane capabilities
				if (mode.parameters.visibleRegion.width > caps.maxDstExtent.width ||
				    mode.parameters.visibleRegion.height > caps.maxDstExtent.height ||
				    mode.parameters.visibleRegion.width < caps.minDstExtent.width ||
				    mode.parameters.visibleRegion.height < caps.minDstExtent.height)
				{
					continue;
				}

				if (window_mode == Window::Mode::Fullscreen ||
				    window_mode == Window::Mode::FullscreenBorderless)
				{
					// For full-screen modes (where the src image is the same size as the
					// display) we must also check the src extents are valid.
					if (mode.parameters.visibleRegion.width > caps.maxSrcExtent.width ||
					    mode.parameters.visibleRegion.height > caps.maxSrcExtent.height ||
					    mode.parameters.visibleRegion.width < caps.minSrcExtent.width ||
					    mode.parameters.visibleRegion.height < caps.minSrcExtent.height)
					{
						continue;
					}
				}

				// Add to list of candidates
				candidates.push_back(Candidate{display, *props, mode, caps, plane_index,
				                               plane_properties[plane_index].currentStackIndex});
			}
		}
	}

	return candidates;
}

VkSurfaceKHR DirectWindow::create_surface(VkInstance instance, VkPhysicalDevice phys_dev)
{
	if (instance == VK_NULL_HANDLE || phys_dev == VK_NULL_HANDLE)
	{
		return VK_NULL_HANDLE;
	}

	// Find possible display config candidates
	std::vector<Candidate> candidates = find_display_candidates(phys_dev, properties.mode);
	if (candidates.empty())
	{
		LOGE("Direct-to-display: No compatible display candidates found");
		return VK_NULL_HANDLE;
	}

	// 'candidates' contains valid potential display configs. Find the best one.
	// Look for the candidate with the closest resolution to our requested width & height.
	uint32_t best_candidate = 0;
	uint32_t wanted_area    = properties.extent.width * properties.extent.height;
	uint32_t best_diff      = ~0;

	for (size_t c = 0; c < candidates.size(); c++)
	{
		const Candidate &cand = candidates[c];

		uint32_t area = cand.display_props.physicalResolution.width *
		                cand.display_props.physicalResolution.height;

		uint32_t diff = abs(static_cast<int32_t>(area) - static_cast<int32_t>(wanted_area));

		if (diff < best_diff)
		{
			best_diff      = diff;
			best_candidate = c;
		}
	}

	const Candidate &best = candidates[best_candidate];

	// Get the full display mode extent
	full_extent.width  = best.mode.parameters.visibleRegion.width;
	full_extent.height = best.mode.parameters.visibleRegion.height;

	VkExtent2D image_extent;
	if (properties.mode == Window::Mode::Fullscreen ||
	    properties.mode == Window::Mode::FullscreenBorderless)
	{
		// Fullscreen & Borderless options create a surface that matches the display size
		image_extent.width  = full_extent.width;
		image_extent.height = full_extent.height;
	}
	else
	{
		// Normal and stretch options create a surface at the requested width & height
		// clamped to the plane capabilities in play
		image_extent.width  = std::min(best.caps.maxSrcExtent.width,
		                               std::max(properties.extent.width, best.caps.minSrcExtent.width));
		image_extent.height = std::min(best.caps.maxSrcExtent.height,
		                               std::max(properties.extent.height, best.caps.minSrcExtent.height));
	}

	// Calculate the display DPI
	constexpr float mm_per_inch = 25.4f;

	dpi = mm_per_inch * best.display_props.physicalResolution.width / best.display_props.physicalDimensions.width;

	// Create the surface
	VkDisplaySurfaceCreateInfoKHR surface_create_info{};
	surface_create_info.sType           = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR;
	surface_create_info.displayMode     = best.mode.displayMode;
	surface_create_info.planeIndex      = best.plane_index;
	surface_create_info.planeStackIndex = best.stack_index;
	surface_create_info.transform       = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	surface_create_info.alphaMode       = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR;
	surface_create_info.imageExtent     = image_extent;

	VkSurfaceKHR surface;
	VK_CHECK(vkCreateDisplayPlaneSurfaceKHR(instance, &surface_create_info, nullptr, &surface));

	return surface;
}

void DirectWindow::process_events()
{
	if (tty_fd > 0)
	{
		if (key_down != KeyCode::Unknown)
		{
			// Signal release for the key we previously reported as down
			// (we don't get separate press & release from the terminal)
			platform->input_event(KeyInputEvent{key_down, KeyAction::Up});
			key_down = KeyCode::Unknown;
		}

		// See if there is a new keypress
		uint8_t key   = 0;
		int     bytes = ::read(tty_fd, &key, 1);

		if (bytes > 0 && key > 0 && (key < sizeof(key_map) / sizeof(KeyCode)))
		{
			// Map the key
			key_down = key_map[key];

			// Is this potentially a multi-character code?
			if (key_down == KeyCode::Escape)
			{
				key_down = map_multichar_key(tty_fd, key_down);
			}

			// Signal the press
			platform->input_event(KeyInputEvent{key_down, KeyAction::Down});
		}
	}
}

bool DirectWindow::should_close()
{
	return !keep_running;
}

void DirectWindow::close()
{
	keep_running = false;
}

float DirectWindow::get_dpi_factor() const
{
	const float win_base_density = 96.0f;
	return dpi / win_base_density;
}

bool DirectWindow::get_display_present_info(VkDisplayPresentInfoKHR *info,
                                            uint32_t src_width, uint32_t src_height) const
{
	// Only stretch mode needs to supply a VkDisplayPresentInfoKHR
	if (properties.mode != Window::Mode::FullscreenStretch || !info)
		return false;

	info->sType      = VK_STRUCTURE_TYPE_DISPLAY_PRESENT_INFO_KHR;
	info->pNext      = nullptr;
	info->srcRect    = {0, 0, src_width, src_height};
	info->dstRect    = {0, 0, full_extent.width, full_extent.height};
	info->persistent = false;

	return true;
}

std::vector<const char *> DirectWindow::get_required_surface_extensions() const
{
	return {VK_KHR_DISPLAY_EXTENSION_NAME};
}

}        // namespace vkb
