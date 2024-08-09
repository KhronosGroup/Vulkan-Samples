/* Copyright (c) 2021-2024, Holochip
 * Copyright (c) 2024, NVIDIA CORPORATION. All rights reserved.
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

#include "ktx.h"
#include "vulkan_sample.h"

class HPPTextureCompressionComparison : public vkb::VulkanSample<vkb::BindingType::Cpp>
{
  public:
	HPPTextureCompressionComparison();

  private:
	// from vkb::VulkanSample
	void draw_gui() override;
	bool prepare(const vkb::ApplicationOptions &options) override;
	void update(float delta_time) override;

  private:
	struct HPPTextureCompressionData
	{
		vk::Bool32 vk::PhysicalDeviceFeatures::*feature_ptr      = nullptr;
		std::string                             extension_name   = {};
		vk::Format                              format           = {};
		ktx_transcode_fmt_e                     ktx_format       = KTX_TTF_NOSELECTION;
		std::string                             format_name      = {};
		std::string                             short_name       = {};
		bool                                    always_supported = false;
		std::string                             gui_name         = {};
		bool                                    is_supported     = {};
	};

	struct HPPTextureBenchmark
	{
		HPPTextureBenchmark &operator+=(const HPPTextureBenchmark &other)
		{
			total_bytes += other.total_bytes;
			compress_time_ms += other.compress_time_ms;
			frame_time_ms += other.frame_time_ms;
			return *this;
		}
		vk::DeviceSize total_bytes      = 0;
		float          compress_time_ms = 0.f;
		float          frame_time_ms    = 0.f;
	};

	struct HPPSampleTexture
	{
		std::vector<uint8_t>                                    raw_bytes;
		std::unique_ptr<vkb::scene_graph::components::HPPImage> image;
		HPPTextureBenchmark                                     benchmark;
	};

  private:
	std::pair<std::unique_ptr<vkb::scene_graph::components::HPPImage>, HPPTextureBenchmark> compress(
	    const std::string &filename, HPPTextureCompressionData texture_format, const std::string &name);
	std::unique_ptr<vkb::scene_graph::components::HPPImage> create_image(ktxTexture2 *ktx_texture, const std::string &name);
	void                                                    create_subpass();
	bool                                                    is_texture_format_supported(const HPPTextureCompressionData &tcd, vk::PhysicalDeviceFeatures const &device_features);
	void                                                    load_assets();
	void                                                    prepare_gui();
	HPPTextureBenchmark                                     update_textures(const HPPTextureCompressionData &new_format);

  private:
	vkb::sg::Camera                                                                *camera             = nullptr;
	HPPTextureBenchmark                                                             current_benchmark  = {};
	int                                                                             current_gui_format = 0;
	int                                                                             current_format     = 0;
	bool                                                                            require_redraw     = true;
	std::vector<HPPTextureCompressionData>                                          texture_compression_data;
	std::unordered_map<std::string, HPPSampleTexture>                               texture_raw_data;
	std::vector<std::pair<vkb::scene_graph::components::HPPTexture *, std::string>> textures;
};

std::unique_ptr<HPPTextureCompressionComparison> create_hpp_texture_compression_comparison();
