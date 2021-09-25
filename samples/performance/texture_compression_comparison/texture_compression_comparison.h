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

class TextureCompressionComparison : public ApiVulkanSample
{
  public:
    TextureCompressionComparison();
    ~TextureCompressionComparison();
    virtual void request_gpu_features(vkb::PhysicalDevice &gpu) override;
    bool         prepare(vkb::Platform &platform) override;
    virtual void render(float delta_time) override;
    virtual void view_changed() override;
    virtual void on_update_ui_overlay(vkb::Drawer &drawer) override;
    void         build_command_buffers() override;

  private:
    struct CompressedTexture_t
    {
        VkBool32 VkPhysicalDeviceFeatures::*feature_ptr{nullptr};
        const char                         *extension_name = "";
        VkFormat                            format         = VK_FORMAT_MAX_ENUM;
        ktx_transcode_fmt_e                 ktx_format     = KTX_TTF_NOSELECTION;
        const char                         *format_name    = "";
    };

  private:
    static std::vector<CompressedTexture_t> get_texture_formats();
    void                                    get_available_texture_formats();
    std::vector<CompressedTexture_t>        available_texture_formats = {};
};

std::unique_ptr<TextureCompressionComparison> create_texture_compression_comparison();
