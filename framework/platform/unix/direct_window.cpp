/* Copyright (c) 2019-2021, Arm Limited and Contributors
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
DirectWindow::DirectWindow(Platform &platform, uint32_t width, uint32_t height) :
    Window(platform, width, height)
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

	platform.set_focus(true);
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
	if (instance.get_handle() == VK_NULL_HANDLE)
	{
		return VK_NULL_HANDLE;
	}

	auto phys_dev = instance.get_first_gpu().get_handle();
	if (phys_dev == VK_NULL_HANDLE)
	{
		return VK_NULL_HANDLE;
	}

	// Query the display properties
	uint32_t num_displays;
	VK_CHECK(vkGetPhysicalDeviceDisplayPropertiesKHR(phys_dev, &num_displays, nullptr));

	if (num_displays == 0)
	{
		LOGE("Direct-to-display: No displays found");
		return VK_NULL_HANDLE;
	}

	VkDisplayPropertiesKHR display_properties;
	num_displays = 1;
	VK_CHECK(vkGetPhysicalDeviceDisplayPropertiesKHR(phys_dev, &num_displays, &display_properties));

	VkDisplayKHR display = display_properties.display;

	// Calculate the display DPI
	dpi = 25.4f * display_properties.physicalResolution.width / display_properties.physicalDimensions.width;

	// Query display mode properties
	uint32_t num_modes;
	VK_CHECK(vkGetDisplayModePropertiesKHR(phys_dev, display, &num_modes, nullptr));

	if (num_modes == 0)
	{
		LOGE("Direct-to-display: No display modes found");
		return VK_NULL_HANDLE;
	}

	VkDisplayModePropertiesKHR mode_props;
	num_modes = 1;
	VK_CHECK(vkGetDisplayModePropertiesKHR(phys_dev, display, &num_modes, &mode_props));

	// Get the list of planes
	uint32_t num_planes;
	VK_CHECK(vkGetPhysicalDeviceDisplayPlanePropertiesKHR(phys_dev, &num_planes, nullptr));

	if (num_planes == 0)
	{
		LOGE("Direct-to-display: No display planes found");
		return VK_NULL_HANDLE;
	}

	std::vector<VkDisplayPlanePropertiesKHR> plane_properties(num_planes);

	VK_CHECK(vkGetPhysicalDeviceDisplayPlanePropertiesKHR(phys_dev, &num_planes, plane_properties.data()));

	// Find a compatible plane index
	uint32_t plane_index = find_compatible_plane(phys_dev, display, plane_properties);
	if (plane_index == ~0U)
	{
		return VK_NULL_HANDLE;
	}

	VkExtent2D image_extent;
	image_extent.width  = mode_props.parameters.visibleRegion.width;
	image_extent.height = mode_props.parameters.visibleRegion.height;

	VkDisplaySurfaceCreateInfoKHR surface_create_info{};
	surface_create_info.sType           = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR;
	surface_create_info.displayMode     = mode_props.displayMode;
	surface_create_info.planeIndex      = plane_index;
	surface_create_info.planeStackIndex = plane_properties[plane_index].currentStackIndex;
	surface_create_info.transform       = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	surface_create_info.alphaMode       = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR;
	surface_create_info.imageExtent     = image_extent;

	VkSurfaceKHR surface;
	VK_CHECK(vkCreateDisplayPlaneSurfaceKHR(instance.get_handle(), &surface_create_info, nullptr, &surface));

	return surface;
}

vk::SurfaceKHR DirectWindow::create_surface(vk::Instance instance, vk::PhysicalDevice physical_device)
{
	if (!instance || !physical_device)
	{
		return nullptr;
	}

	// Query the display properties
	std::vector<vk::DisplayPropertiesKHR> display_properties_all = physical_device.getDisplayPropertiesKHR();

	if (display_properties_all.empty())
	{
		LOGE("Direct-to-display: No displays found");
		return nullptr;
	}

	// select the first display
	vk::DisplayPropertiesKHR &display_properties = display_properties_all.front();

	// Calculate the display DPI
	dpi = 25.4f * display_properties.physicalResolution.width / display_properties.physicalDimensions.width;

	// Query display mode properties
	std::vector<vk::DisplayModePropertiesKHR> display_mode_properties = physical_device.getDisplayModePropertiesKHR(display_properties.display);

	if (display_mode_properties.empty())
	{
		LOGE("Direct-to-display: No display modes found");
		return nullptr;
	}

	vk::DisplayModePropertiesKHR &mode_props = display_mode_properties.front();

	// Get the list of planes
	std::vector<vk::DisplayPlanePropertiesKHR> plane_properties = physical_device.getDisplayPlanePropertiesKHR();

	if (plane_properties.empty())
	{
		LOGE("Direct-to-display: No display planes found");
		return nullptr;
	}

	// Find a compatible plane index
	uint32_t plane_index = find_compatible_plane(physical_device, display_properties.display, plane_properties);
	if (plane_index == ~0U)
	{
		return nullptr;
	}

	vk::DisplaySurfaceCreateInfoKHR surface_create_info;
	surface_create_info.displayMode     = mode_props.displayMode;
	surface_create_info.planeIndex      = plane_index;
	surface_create_info.planeStackIndex = plane_properties[plane_index].currentStackIndex;
	surface_create_info.transform       = vk::SurfaceTransformFlagBitsKHR::eIdentity;
	surface_create_info.alphaMode       = vk::DisplayPlaneAlphaFlagBitsKHR::eOpaque;
	surface_create_info.imageExtent     = mode_props.parameters.visibleRegion;

	return instance.createDisplayPlaneSurfaceKHR(surface_create_info);
}

uint32_t DirectWindow::find_compatible_plane(VkPhysicalDevice phys_dev, VkDisplayKHR display,
                                             const std::vector<VkDisplayPlanePropertiesKHR> &plane_properties)
{
	// Find a plane compatible with the display
	for (uint32_t pi = 0; pi < plane_properties.size(); ++pi)
	{
		if ((plane_properties[pi].currentDisplay != VK_NULL_HANDLE) &&
		    (plane_properties[pi].currentDisplay != display))
		{
			continue;
		}

		uint32_t num_supported;
		VK_CHECK(vkGetDisplayPlaneSupportedDisplaysKHR(phys_dev, pi, &num_supported, nullptr));

		if (num_supported == 0)
		{
			continue;
		}

		std::vector<VkDisplayKHR> supported_displays(num_supported);

		VK_CHECK(vkGetDisplayPlaneSupportedDisplaysKHR(phys_dev, pi, &num_supported, supported_displays.data()));

		for (uint32_t i = 0; i < num_supported; ++i)
		{
			if (supported_displays[i] == display)
			{
				return pi;
			}
		}
	}

	LOGE("Direct-to-display: No plane found compatible with the display");
	return ~0U;
}

uint32_t DirectWindow::find_compatible_plane(vk::PhysicalDevice physical_device, vk::DisplayKHR display,
                                             const std::vector<vk::DisplayPlanePropertiesKHR> &plane_properties) const
{
	assert(display);
	// Find a plane compatible with the display
	for (uint32_t pi = 0; pi < plane_properties.size(); ++pi)
	{
		if (plane_properties[pi].currentDisplay == display)
		{
			std::vector<vk::DisplayKHR> supported_displays = physical_device.getDisplayPlaneSupportedDisplaysKHR(pi);

			auto display_it = std::find(supported_displays.begin(), supported_displays.end(), display);
			if (display_it != supported_displays.end())
			{
				return pi;
			}
		}
	}

	LOGE("Direct-to-display: No plane found compatible with the display");
	return ~0U;
}

void DirectWindow::process_events()
{
	if (tty_fd > 0)
	{
		if (key_down != KeyCode::Unknown)
		{
			// Signal release for the key we previously reported as down
			// (we don't get separate press & release from the terminal)
			platform.get_app().input_event(KeyInputEvent{platform, key_down, KeyAction::Up});
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
			platform.get_app().input_event(KeyInputEvent{platform, key_down, KeyAction::Down});
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
}        // namespace vkb
