/* Copyright (c) 2024-2025, Holochip Inc.
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

#include <memory>
#include <string>
#include <typeinfo>
#include <vector>

#include "common/glm_common.h"
#include "common/vk_common.h"
#include "core/buffer.h"
#include "scene_graph/component.h"

namespace vkb
{
namespace sg
{

/**
 * @brief Gaussian Splat rendering primitive data
 * 
 * Stores data for rendering 3D Gaussian Splats as defined by the
 * KHR_gaussian_splatting GLTF extension.
 * 
 * Each splat is an oriented 3D Gaussian defined by:
 * - Position (center point)
 * - Rotation (quaternion orientation)
 * - Scale (3D scale factors)
 * - Opacity (alpha value)
 * - Color (RGB or spherical harmonics coefficients)
 */
class GaussianSplat : public Component
{
  public:
	/**
	 * @brief Kernel type for splat rendering
	 */
	enum class KernelType
	{
		Ellipse,    // Default elliptical kernel
		Sphere      // Spherical kernel (isotropic)
	};

	/**
	 * @brief Color space for splat colors
	 */
	enum class ColorSpace
	{
		SRGB,       // BT.709-sRGB
		Linear      // Linear RGB
	};

	GaussianSplat(const std::string &name = {});

	virtual ~GaussianSplat() = default;

	virtual std::type_index get_type() override;

	// Number of splats
	uint32_t splat_count = 0;

	// Spherical harmonics degree (0-3)
	uint32_t sh_degree = 0;

	// Whether antialiasing is enabled
	bool antialiased = false;

	// Kernel type for rendering
	KernelType kernel = KernelType::Ellipse;

	// Color space
	ColorSpace color_space = ColorSpace::SRGB;

	// GPU buffers for splat data
	std::unique_ptr<vkb::core::BufferC> position_buffer;    // VEC3 positions
	std::unique_ptr<vkb::core::BufferC> rotation_buffer;    // VEC4 quaternions
	std::unique_ptr<vkb::core::BufferC> scale_buffer;       // VEC3 scales
	std::unique_ptr<vkb::core::BufferC> opacity_buffer;     // SCALAR opacities
	std::unique_ptr<vkb::core::BufferC> color_buffer;       // VEC3 colors (or SH coefficients)
	std::unique_ptr<vkb::core::BufferC> sh_buffer;          // MAT3 spherical harmonics (if sh_degree > 0)

	/**
	 * @brief Get the total GPU memory used by this splat primitive
	 */
	size_t get_gpu_memory_size() const;

	/**
	 * @brief Check if spherical harmonics data is available
	 */
	bool has_spherical_harmonics() const { return sh_degree > 0 && sh_buffer != nullptr; }
};

}        // namespace sg
}        // namespace vkb
