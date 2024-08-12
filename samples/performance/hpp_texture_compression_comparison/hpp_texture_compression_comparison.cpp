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

#include "hpp_texture_compression_comparison.h"
#include "scene_graph/components/hpp_image.h"
#include "scene_graph/components/hpp_mesh.h"
#include "scene_graph/components/material.h"

namespace
{
constexpr std::array<const char *, 19> error_codes = {
    "KTX_SUCCESS",
    "KTX_FILE_DATA_ERROR",
    "KTX_FILE_ISPIPE",
    "KTX_FILE_OPEN_FAILED",
    "KTX_FILE_OVERFLOW",
    "KTX_FILE_READ_ERROR",
    "KTX_FILE_SEEK_ERROR",
    "KTX_FILE_UNEXPECTED_EOF",
    "KTX_FILE_WRITE_ERROR",
    "KTX_GL_ERROR",
    "KTX_INVALID_OPERATION",
    "KTX_INVALID_VALUE",
    "KTX_NOT_FOUND",
    "KTX_OUT_OF_MEMORY",
    "KTX_TRANSCODE_FAILED",
    "KTX_UNKNOWN_FILE_FORMAT",
    "KTX_UNSUPPORTED_TEXTURE_TYPE",
    "KTX_UNSUPPORTED_FEATURE",
    "KTX_LIBRARY_NOT_LINKED",
};

std::string get_sponza_texture_filename(const std::string &short_name)
{
	return vkb::fs::path::get(vkb::fs::path::Type::Assets) + "scenes/sponza/ktx2/" + short_name + "2";
}

class HPPCompressedImage : public vkb::scene_graph::components::HPPImage
{
  public:
	HPPCompressedImage(vkb::core::HPPDevice                                  &device,
	                   const std::string                                     &name,
	                   std::vector<vkb::scene_graph::components::HPPMipmap> &&mipmaps,
	                   vk::Format                                             format) :
	    vkb::scene_graph::components::HPPImage(name, std::vector<uint8_t>{}, std::move(mipmaps))
	{
		vkb::scene_graph::components::HPPImage::set_format(format);
		vkb::scene_graph::components::HPPImage::create_vk_image(device);
	}
};
}        // namespace

#define KTX_CHECK(x)                                                                              \
	do                                                                                            \
	{                                                                                             \
		KTX_error_code err = x;                                                                   \
		if (err != KTX_SUCCESS)                                                                   \
		{                                                                                         \
			auto index = static_cast<uint32_t>(err);                                              \
			LOGE("Detected KTX error: {}", index < error_codes.size() ? error_codes[index] : ""); \
			abort();                                                                              \
		}                                                                                         \
	} while (0)

HPPTextureCompressionComparison::HPPTextureCompressionComparison()
{
	texture_compression_data = {
	    {nullptr, "", vk::Format::eR8G8B8A8Srgb, KTX_TTF_RGBA32, "KTX_TTF_RGBA32", "RGBA 32", true},
	    {&vk::PhysicalDeviceFeatures::textureCompressionBC, "", vk::Format::eBc7SrgbBlock, KTX_TTF_BC7_RGBA, "KTX_TTF_BC7_RGBA", "BC7"},
	    {&vk::PhysicalDeviceFeatures::textureCompressionBC, "", vk::Format::eBc3SrgbBlock, KTX_TTF_BC3_RGBA, "KTX_TTF_BC3_RGBA", "BC3"},
	    {&vk::PhysicalDeviceFeatures::textureCompressionASTC_LDR, "", vk::Format::eAstc4x4SrgbBlock, KTX_TTF_ASTC_4x4_RGBA, "KTX_TTF_ASTC_4x4_RGBA", "ASTC 4x4"},
	    {&vk::PhysicalDeviceFeatures::textureCompressionETC2, "", vk::Format::eEtc2R8G8B8A8SrgbBlock, KTX_TTF_ETC2_RGBA, "KTX_TTF_ETC2_RGBA", "ETC2"},
	    {nullptr, VK_IMG_FORMAT_PVRTC_EXTENSION_NAME, vk::Format::ePvrtc14BppSrgbBlockIMG, KTX_TTF_PVRTC1_4_RGBA, "KTX_TTF_PVRTC1_4_RGBA", "PVRTC1 4"}};

	add_device_extension(VK_IMG_FORMAT_PVRTC_EXTENSION_NAME, true);
}

void HPPTextureCompressionComparison::draw_gui()
{
	get_gui().show_options_window(
	    [this]() {
		    if (ImGui::Combo(
		            "Compressed Format",
		            &current_gui_format,
		            [](void *user_data, int idx) -> char const * { return reinterpret_cast<HPPTextureCompressionData *>(user_data)[idx].gui_name.c_str(); },
		            texture_compression_data.data(),
		            static_cast<int>(texture_compression_data.size())))
		    {
			    require_redraw = true;
			    if (texture_compression_data[current_gui_format].is_supported)
			    {
				    current_format = current_gui_format;
			    }
		    }
		    const auto &current_gui_tc = texture_compression_data[current_gui_format];
		    if (current_gui_tc.is_supported)
		    {
			    ImGui::Text("Format name: %s", current_gui_tc.format_name.c_str());
			    ImGui::Text("Bytes: %f MB", static_cast<float>(current_benchmark.total_bytes) / 1024.f / 1024.f);
			    ImGui::Text("Compression Time: %f (ms)", current_benchmark.compress_time_ms);
		    }
		    else
		    {
			    ImGui::Text("%s not supported on this GPU.", current_gui_tc.short_name.c_str());
		    }
	    });
}

bool HPPTextureCompressionComparison::prepare(const vkb::ApplicationOptions &options)
{
	if (vkb::VulkanSample<vkb::BindingType::Cpp>::prepare(options))
	{
		load_assets();

		auto &camera_node = vkb::common::add_free_camera(get_scene(), "main_camera", get_render_context().get_surface_extent());
		camera            = &camera_node.get_component<vkb::sg::Camera>();

		create_subpass();

		get_stats().request_stats({vkb::StatIndex::frame_times, vkb::StatIndex::gpu_ext_read_bytes});
		create_gui(*window, &get_stats());
		prepare_gui();

		return true;
	}
	return false;
}

void HPPTextureCompressionComparison::update(float delta_time)
{
	if (require_redraw)
	{
		require_redraw = false;
		assert(current_format >= 0 && static_cast<size_t>(current_format) < texture_compression_data.size());
		current_benchmark = update_textures(texture_compression_data[current_format]);
	}
	VulkanSample<vkb::BindingType::Cpp>::update(delta_time);
}

std::pair<std::unique_ptr<vkb::scene_graph::components::HPPImage>, HPPTextureCompressionComparison::HPPTextureBenchmark>
    HPPTextureCompressionComparison::compress(const std::string                                         &filename,
                                              HPPTextureCompressionComparison::HPPTextureCompressionData texture_format,
                                              const std::string                                         &name)
{
	ktxTexture2 *ktx_texture{nullptr};
	KTX_CHECK(ktxTexture2_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktx_texture));

	HPPTextureBenchmark benchmark;
	{
		const auto start = std::chrono::high_resolution_clock::now();
		KTX_CHECK(ktxTexture2_TranscodeBasis(ktx_texture, texture_format.ktx_format, 0));
		const auto end             = std::chrono::high_resolution_clock::now();
		benchmark.compress_time_ms = static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()) / 1000.f;
	}
	benchmark.total_bytes = ktx_texture->dataSize;
	auto image            = create_image(ktx_texture, name);
	ktxTexture_Destroy((ktxTexture *) ktx_texture);

	return {std::move(image), benchmark};
}

std::unique_ptr<vkb::scene_graph::components::HPPImage> HPPTextureCompressionComparison::create_image(ktxTexture2 *ktx_texture, const std::string &name)
{
	std::unique_ptr<vkb::core::HPPBuffer> staging_buffer =
	    std::make_unique<vkb::core::HPPBuffer>(get_device(), ktx_texture->dataSize, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
	memcpy(staging_buffer->map(), ktx_texture->pData, ktx_texture->dataSize);

	const auto vk_format = static_cast<vk::Format>(ktx_texture->vkFormat);

	vk::Extent3D extent(ktx_texture->baseWidth, ktx_texture->baseHeight, 1);

	std::vector<vk::BufferImageCopy>                        buffer_copies;
	std::unique_ptr<vkb::scene_graph::components::HPPImage> image_out;
	{
		std::vector<vkb::scene_graph::components::HPPMipmap> mip_maps;
		for (uint32_t mip_level = 0; mip_level < ktx_texture->numLevels; ++mip_level)
		{
			vk::Extent3D mip_extent(extent.width >> mip_level, extent.height >> mip_level, 1);
			if (!mip_extent.width || !mip_extent.height)
			{
				break;
			}

			ktx_size_t offset{0};
			KTX_CHECK(ktxTexture_GetImageOffset((ktxTexture *) ktx_texture, mip_level, 0, 0, &offset));
			vk::BufferImageCopy buffer_image_copy = {};
			buffer_image_copy.imageSubresource    = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, mip_level, 0, 1);
			buffer_image_copy.imageExtent         = mip_extent;
			buffer_image_copy.bufferOffset        = static_cast<uint32_t>(offset);
			buffer_copies.push_back(buffer_image_copy);

			vkb::scene_graph::components::HPPMipmap mip_map;
			mip_map.extent = buffer_image_copy.imageExtent;
			mip_map.level  = mip_level;
			mip_map.offset = static_cast<uint32_t>(offset);
			mip_maps.push_back(mip_map);
		}

		image_out = std::make_unique<HPPCompressedImage>(get_device(), name, std::move(mip_maps), vk_format);
	}
	auto &vkb_image = image_out->get_vk_image();
	auto  image     = vkb_image.get_handle();

	vk::ImageSubresourceRange subresource_range(vk::ImageAspectFlagBits::eColor, 0, static_cast<uint32_t>(buffer_copies.size()), 0, 1);

	vk::CommandBuffer command_buffer = get_device().create_command_buffer(vk::CommandBufferLevel::ePrimary, true);

	vkb::common::image_layout_transition(command_buffer, image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, subresource_range);

	command_buffer.copyBufferToImage(staging_buffer->get_handle(), image, vk::ImageLayout::eTransferDstOptimal, buffer_copies);

	vkb::common::image_layout_transition(
	    command_buffer, image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, subresource_range);

	get_device().flush_command_buffer(command_buffer, get_device().get_queue_by_flags(vk::QueueFlagBits::eGraphics, 0).get_handle(), true);

	return image_out;
}

void HPPTextureCompressionComparison::create_subpass()
{
	vkb::ShaderSource vert_shader("base.vert");
	vkb::ShaderSource frag_shader("base.frag");
	auto              scene_sub_pass = std::make_unique<vkb::rendering::subpasses::HPPForwardSubpass>(
        get_render_context(), std::move(vert_shader), std::move(frag_shader), get_scene(), *camera);

	auto render_pipeline = std::make_unique<vkb::rendering::HPPRenderPipeline>();
	render_pipeline->add_subpass(std::move(scene_sub_pass));

	set_render_pipeline(std::move(render_pipeline));
}

bool HPPTextureCompressionComparison::is_texture_format_supported(const HPPTextureCompressionData &tcd, vk::PhysicalDeviceFeatures const &device_features)
{
	const bool supported_by_feature   = tcd.feature_ptr && device_features.*tcd.feature_ptr;
	const bool supported_by_extension = tcd.extension_name.length() && get_device().is_extension_supported(tcd.extension_name);
	const bool supported_by_default   = tcd.always_supported;

	return supported_by_default || supported_by_feature || supported_by_extension;
}

void HPPTextureCompressionComparison::load_assets()
{
	load_scene("scenes/sponza/Sponza01.gltf");
	if (!has_scene())
	{
		throw std::runtime_error("Unable to load Sponza scene");
	}

	for (auto &&mesh : get_scene().get_components<vkb::scene_graph::components::HPPMesh>())
	{
		for (auto &&sub_mesh : mesh->get_submeshes())
		{
			auto material = sub_mesh->get_material();
			for (auto &name_texture : material->get_textures())
			{
				vkb::scene_graph::components::HPPTexture *texture = name_texture.second;
				auto                                      image   = texture->get_image();
				textures.emplace_back(texture, image->get_name());
			}
		}
	}
}

void HPPTextureCompressionComparison::prepare_gui()
{
	const auto device_features = get_device().get_gpu().get_features();

	for (auto &tc : texture_compression_data)
	{
		tc.is_supported = is_texture_format_supported(tc, device_features);
		tc.gui_name     = fmt::format(FMT_STRING("{:s} {:s}"), tc.short_name, tc.is_supported ? "" : "(not supported)");
	}
}

HPPTextureCompressionComparison::HPPTextureBenchmark HPPTextureCompressionComparison::update_textures(const HPPTextureCompressionData &new_format)
{
	HPPTextureBenchmark             benchmark;
	std::unordered_set<std::string> visited;
	for (auto &&texture_filename : textures)
	{
		vkb::scene_graph::components::HPPTexture *texture = texture_filename.first;
		assert(!!texture);
		auto &internal_name = texture_filename.second;
		if (!visited.count(internal_name))
		{
			auto filename                             = get_sponza_texture_filename(internal_name);
			auto new_image                            = compress(filename, new_format, "");
			texture_raw_data[internal_name].image     = std::move(new_image.first);
			texture_raw_data[internal_name].benchmark = new_image.second;
			benchmark += new_image.second;
		}

		vkb::scene_graph::components::HPPImage *image = texture_raw_data[internal_name].image.get();
		assert(image);
		texture->set_image(*image);
		visited.insert(internal_name);
	}

	// update the forward subpass to use the new textures
	create_subpass();

	return benchmark;
}

std::unique_ptr<HPPTextureCompressionComparison> create_hpp_texture_compression_comparison()
{
	return std::make_unique<HPPTextureCompressionComparison>();
}
