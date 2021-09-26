/* Copyright (c) 2021, Holochip
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

#include "api_vulkan_sample.h"
#include "scene_graph/components/camera.h"

class TextureCompressionComparison : public vkb::VulkanSample
{
  public:
    TextureCompressionComparison();
    ~TextureCompressionComparison();
    bool         prepare(vkb::Platform &platform) override;
    virtual void update(float delta_time) override;
    virtual void draw_gui() override;

  private:
    struct CompressedTexture_t
    {
        VkBool32 VkPhysicalDeviceFeatures::*feature_ptr{nullptr};
        const char                         *extension_name  = "";
        VkFormat                            format          = VK_FORMAT_MAX_ENUM;
        ktx_transcode_fmt_e                 ktx_format      = KTX_TTF_NOSELECTION;
        const char                         *format_name     = "";
        const char                         *short_name      = "";
        bool                                always_suported = false;
    };

	struct TextureBenchmark
	{
		TextureBenchmark &operator+=(const TextureBenchmark &other)
		{
			total_bytes += other.total_bytes;
			compress_time_ms += other.compress_time_ms;
			frametime_ms += other.frametime_ms;
			return *this;
		}
		VkDeviceSize total_bytes      = 0;
		float        compress_time_ms = 0.f;
		float        frametime_ms     = 0.f;
	};

	struct SampleTexture
	{
		static void                     destroy_texture(ktxTexture2 *ktx_texture);
		std::vector<uint8_t>            raw_bytes;        //{nullptr, &destroy_texture};
		std::unique_ptr<vkb::sg::Image> image;
		TextureBenchmark                benchmark;
	};

  private:
    static const std::vector<CompressedTexture_t>               &get_texture_formats();
    bool                                                         is_texture_format_supported(const CompressedTexture_t &format);
    void                                                         get_available_texture_formats();
    void                                                         load_assets();
    TextureBenchmark                                             update_textures(const CompressedTexture_t &new_format);
    std::unique_ptr<vkb::sg::Image>                              create_image(ktxTexture2 *ktx_texture, const std::string &name);
    std::vector<uint8_t>                                         get_raw_image(const std::string &filename);
    std::pair<std::unique_ptr<vkb::sg::Image>, TextureBenchmark> compress(const std::string &filename, CompressedTexture_t texture_format, const std::string &name);
    std::vector<CompressedTexture_t>                             available_texture_formats = {};
    std::unordered_map<std::string, SampleTexture>               texture_raw_data;
    std::vector<std::pair<vkb::sg::Texture *, std::string>>      textures;
    vkb::sg::Camera                                             *camera{VK_NULL_HANDLE};
    TextureBenchmark                                             current_benchmark{};
    int                                                          current_format = 1, current_gui_format = 1;
    bool                                                         require_redraw = true;
};

std::unique_ptr<TextureCompressionComparison> create_texture_compression_comparison();
