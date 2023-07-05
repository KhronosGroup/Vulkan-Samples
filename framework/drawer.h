/* Copyright (c) 2023, Mobica Limited
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

#include <array>
#include <imgui.h>
#include <stdint.h>
#include <string>
#include <vector>

namespace vkb
{
class Drawer;

/**
 * @brief Responsible for drawing new elements into the gui
 */
class Drawer
{
  public:
	enum class ColorOp
	{
		Edit,
		Pick
	};

	Drawer() = default;

	/**
	 * @brief Clears the dirty bit set
	 */
	void clear();

	/**
	 * @brief Returns true if the drawer has been updated
	 */
	bool is_dirty();

	/**
	 * @brief May be used to force drawer update
	 */
	void set_dirty(bool dirty);

	/**
	 * @brief Adds a collapsable header item to the gui
	 * @param caption The text to display
	 * @returns True if adding item was successful
	 */
	bool header(const char *caption);

	/**
	 * @brief Adds a checkbox to the gui
	 * @param caption The text to display
	 * @param value The boolean value to map the checkbox to
	 * @returns True if adding item was successful
	 */
	bool checkbox(const char *caption, bool *value);

	/**
	 * @brief Adds a checkbox to the gui
	 * @param caption The text to display
	 * @param value The integer value to map the checkbox to
	 * @returns True if adding item was successful
	 */
	bool checkbox(const char *caption, int32_t *value);

	/**
	 * @brief Adds a number input field to the gui
	 * @param caption The text to display
	 * @param value The value to map to
	 * @param step The step increment
	 * @param precision The precision
	 * @returns True if adding item was successful
	 */
	bool input_float(const char *caption, float *value, float step, uint32_t precision);

	/**
	 * @brief Adds a slide bar to the gui for floating points to the gui
	 * @param caption The text to display
	 * @param value The value to map to
	 * @param min The minimum value
	 * @param max The maximum value
	 * @returns True if adding item was successful
	 */
	bool slider_float(const char *caption, float *value, float min, float max);

	/**
	 * @brief Adds a slide bar to the gui for integers to the gui
	 * @param caption The text to display
	 * @param value The value to map to
	 * @param min The minimum value
	 * @param max The maximum value
	 * @returns True if adding item was successful
	 */
	bool slider_int(const char *caption, int32_t *value, int32_t min, int32_t max);

	/**
	 * @brief Adds a multiple choice drop box to the gui
	 * @param caption The text to display
	 * @param itemindex The item index to display
	 * @param items The items to display in the box
	 * @returns True if adding item was successful
	 */
	bool combo_box(const char *caption, int32_t *itemindex, std::vector<std::string> items);

	/**
	 * @brief Adds a clickable button to the gui
	 * @param caption The text to display
	 * @returns True if adding item was successful
	 */
	bool button(const char *caption);

	/**
	 * @brief Adds a label to the gui
	 * @param formatstr The format string
	 */
	void text(const char *formatstr, ...);

	/**
	 * @brief Adds a color picker to the gui
	 * @param caption The text to display
	 * @param color Color channel array on which the picker works. It contains values ranging from 0 to 1.
	 * @param width Element width. Zero is a special value for the default element width.
	 * @param flags Flags to modify the appearance and behavior of the element.
	 */
	bool color_picker(const char *caption, std::array<float, 3> &color, float width = 0.0f, ImGuiColorEditFlags flags = 0);

	/**
	 * @brief Adds a color picker to the gui
	 * @param caption The text to display
	 * @param color Color channel array on which the picker works. It contains values ranging from 0 to 1.
	 * @param width Element width. Zero is a special value for the default element width.
	 * @param flags Flags to modify the appearance and behavior of the element.
	 */
	bool color_picker(const char *caption, std::array<float, 4> &color, float width = 0.0f, ImGuiColorEditFlags flags = 0);

	/**
	 * @brief Adds a color edit to the gui
	 * @param caption The text to display
	 * @param color Color channel array on which the picker works. It contains values ranging from 0 to 1.
	 * @param width Element width. Zero is a special value for the default element width.
	 * @param flags Flags to modify the appearance and behavior of the element.
	 */
	bool color_edit(const char *caption, std::array<float, 3> &color, float width = 0.0f, ImGuiColorEditFlags flags = 0);

	/**
	 * @brief Adds a color edit to the gui
	 * @tparam OP Mode of the color element.
	 * @tparam N Color channel count. Must be 3 or 4.
	 * @param caption The text to display
	 * @param color Color channel array on which the picker works. It contains values ranging from 0 to 1.
	 * @param width Element width. Zero is a special value for the default element width.
	 * @param flags Flags to modify the appearance and behavior of the element.
	 */
	template <ColorOp OP, size_t N>
	bool color_op(const std::string &caption, std::array<float, N> &color, float width = 0.0f, ImGuiColorEditFlags flags = 0)
	{
		static_assert((N == 3) || (N == 4), "The channel count must be 3 or 4.");

		ImGui::PushItemWidth(width);
		bool res = color_op_impl<OP, N>(caption.c_str(), color.data(), flags);
		ImGui::PopItemWidth();
		if (res)
			dirty = true;
		return res;
	}

  private:
	template <ColorOp OP, size_t N>
	inline bool color_op_impl(const char *caption, float *colors, ImGuiColorEditFlags flags)
	{
		assert(false);
		return false;
	}

	bool dirty{false};
};

template <>
bool Drawer::color_op_impl<Drawer::ColorOp::Edit, 3>(const char *caption, float *colors, ImGuiColorEditFlags flags);

template <>
bool Drawer::color_op_impl<Drawer::ColorOp::Edit, 4>(const char *caption, float *colors, ImGuiColorEditFlags flags);

template <>
bool Drawer::color_op_impl<Drawer::ColorOp::Pick, 3>(const char *caption, float *colors, ImGuiColorEditFlags flags);

template <>
bool Drawer::color_op_impl<Drawer::ColorOp::Pick, 4>(const char *caption, float *colors, ImGuiColorEditFlags flags);

}        // namespace vkb
