/* Copyright (c) 2021-2023, Holochip
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

#include "texture_compression_comparison.h"
#include "rendering/subpasses/forward_subpass.h"
#include "scene_graph/components/camera.h"
#include "scene_graph/components/image.h"
#include "scene_graph/components/material.h"
#include "scene_graph/components/mesh.h"

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

bool TextureCompressionComparison::prepare(const vkb::ApplicationOptions &options)
{
	if (!VulkanSample::prepare(options))
	{
		return false;
	}

	load_assets();

	auto &camera_node = vkb::add_free_camera(*scene, "main_camera", get_render_context().get_surface_extent());
	camera            = &camera_node.get_component<vkb::sg::Camera>();

	create_subpass();

	stats->request_stats({vkb::StatIndex::frame_times, vkb::StatIndex::gpu_ext_read_bytes});
	gui = std::make_unique<vkb::Gui>(*this, *window, stats.get());

	return true;
}

void TextureCompressionComparison::update(float delta_time)
{
	if (require_redraw)
	{
		const auto &formats = get_texture_formats();
		require_redraw      = false;
		assert(current_format >= 0 && static_cast<size_t>(current_format) < formats.size());
		current_benchmark = update_textures(formats[current_format]);
	}
	VulkanSample::update(delta_time);
}

void TextureCompressionComparison::draw_gui()
{
	if (gui_texture_names.empty())
	{
		const auto &formats = get_texture_formats();
		gui_texture_names.resize(formats.size());
		std::transform(formats.cbegin(), formats.cend(), gui_texture_names.begin(), [this](const CompressedTexture_t &format) -> std::string {
			return fmt::format(FMT_STRING("{:s} {:s}"), format.short_name, is_texture_format_supported(format) ? "" : "(not supported)");
		});
	}

	std::vector<const char *> name_pointers(gui_texture_names.size());
	std::transform(gui_texture_names.cbegin(), gui_texture_names.cend(), name_pointers.begin(), [](const std::string &in) {
		return in.c_str();
	});

	gui->show_options_window([this, &name_pointers]() {
		if (ImGui::Combo("Compressed Format", &current_gui_format, name_pointers.data(), static_cast<int>(name_pointers.size())))
		{
			require_redraw     = true;
			const auto &format = get_texture_formats()[current_gui_format];
			if (is_texture_format_supported(format))
			{
				current_format = current_gui_format;
			}
		}
		const auto &format = get_texture_formats()[current_gui_format];
		if (is_texture_format_supported(format))
		{
			ImGui::Text("Format name: %s", format.format_name);
			ImGui::Text("Bytes: %f MB", static_cast<float>(current_benchmark.total_bytes) / 1024.f / 1024.f);
			ImGui::Text("Compression Time: %f (ms)", current_benchmark.compress_time_ms);
		}
		else
		{
			ImGui::Text("%s not supported on this GPU.", format.short_name);
		}
	});
}

const std::vector<TextureCompressionComparison::CompressedTexture_t> &TextureCompressionComparison::get_texture_formats()
{
	static std::vector<TextureCompressionComparison::CompressedTexture_t> formats = {
	    CompressedTexture_t{nullptr,
	                        "",
	                        VK_FORMAT_R8G8B8A8_SRGB,
	                        KTX_TTF_RGBA32,
	                        "KTX_TTF_RGBA32",
	                        "RGBA 32",
	                        true},
	    CompressedTexture_t{&VkPhysicalDeviceFeatures::textureCompressionBC,
	                        "",
	                        VK_FORMAT_BC7_SRGB_BLOCK,
	                        KTX_TTF_BC7_RGBA,
	                        "KTX_TTF_BC7_RGBA",
	                        "BC7"},
	    CompressedTexture_t{&VkPhysicalDeviceFeatures::textureCompressionBC,
	                        "",
	                        VK_FORMAT_BC3_SRGB_BLOCK,
	                        KTX_TTF_BC3_RGBA,
	                        "KTX_TTF_BC3_RGBA",
	                        "BC3"},
	    CompressedTexture_t{&VkPhysicalDeviceFeatures::textureCompressionASTC_LDR,
	                        "",
	                        VK_FORMAT_ASTC_4x4_SRGB_BLOCK,
	                        KTX_TTF_ASTC_4x4_RGBA,
	                        "KTX_TTF_ASTC_4x4_RGBA",
	                        "ASTC 4x4"},
	    CompressedTexture_t{&VkPhysicalDeviceFeatures::textureCompressionETC2,
	                        "",
	                        VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK,
	                        KTX_TTF_ETC2_RGBA,
	                        "KTX_TTF_ETC2_RGBA",
	                        "ETC2"},
	    CompressedTexture_t{nullptr,
	                        VK_IMG_FORMAT_PVRTC_EXTENSION_NAME,
	                        VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG,
	                        KTX_TTF_PVRTC1_4_RGBA,
	                        "KTX_TTF_PVRTC1_4_RGBA",
	                        "PVRTC1 4"}};
	return formats;
}

bool TextureCompressionComparison::is_texture_format_supported(const TextureCompressionComparison::CompressedTexture_t &format)
{
	const auto device_features = get_device().get_gpu().get_features();

	const bool supported_by_feature   = format.feature_ptr && device_features.*format.feature_ptr;
	const bool supported_by_extension = strlen(format.extension_name) && get_device().is_extension_supported(format.extension_name);
	const bool supported_by_default   = format.always_supported;

	return supported_by_default || supported_by_feature || supported_by_extension;
}

void TextureCompressionComparison::get_available_texture_formats()
{
	available_texture_formats.clear();

	const auto all_formats = get_texture_formats();

	// Determine which formats are supported by this device
	std::copy_if(all_formats.cbegin(), all_formats.cend(), std::back_inserter(available_texture_formats), [this](const auto &texture_format) {
		return is_texture_format_supported(texture_format);
	});
}

void TextureCompressionComparison::load_assets()
{
	get_available_texture_formats();
	load_scene("scenes/sponza/Sponza01.gltf");
	if (!scene)
	{
		throw std::runtime_error("Unable to load Sponza scene");
	}

	for (auto &&mesh : scene->get_components<vkb::sg::Mesh>())
	{
		for (auto &&sub_mesh : mesh->get_submeshes())
		{
			auto material = sub_mesh->get_material();
			for (auto &name_texture : material->textures)
			{
				vkb::sg::Texture *texture = name_texture.second;
				auto              image   = texture->get_image();
				textures.emplace_back(texture, image->get_name());
			}
		}
	}
}

void TextureCompressionComparison::create_subpass()
{
	vkb::ShaderSource vert_shader("base.vert");
	vkb::ShaderSource frag_shader("base.frag");
	auto              scene_sub_pass = std::make_unique<vkb::ForwardSubpass>(get_render_context(), std::move(vert_shader), std::move(frag_shader), *scene, *camera);

	auto render_pipeline = vkb::RenderPipeline();
	render_pipeline.add_subpass(std::move(scene_sub_pass));

	set_render_pipeline(std::move(render_pipeline));
}

TextureCompressionComparison::TextureBenchmark TextureCompressionComparison::update_textures(const TextureCompressionComparison::CompressedTexture_t &new_format)
{
	TextureBenchmark                benchmark;
	std::unordered_set<std::string> visited;
	for (auto &&texture_filename : textures)
	{
		vkb::sg::Texture *texture = texture_filename.first;
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

		vkb::sg::Image *image = texture_raw_data[internal_name].image.get();
		assert(image);
		texture->set_image(*image);
		visited.insert(internal_name);
	}

	// update the forward subpass to use the new textures
	create_subpass();

	return benchmark;
}

namespace
{
class CompressedImage : public vkb::sg::Image
{
  public:
	CompressedImage(vkb::Device &device, const std::string &name, std::vector<vkb::sg::Mipmap> &&mipmaps, VkFormat format) :
	    vkb::sg::Image(name, std::vector<uint8_t>{}, std::move(mipmaps))
	{
		vkb::sg::Image::set_format(format);
		vkb::sg::Image::create_vk_image(device);
	}
};
}        // namespace

std::unique_ptr<vkb::sg::Image> TextureCompressionComparison::create_image(ktxTexture2 *ktx_texture, const std::string &name)
{
	std::unique_ptr<vkb::core::Buffer> staging_buffer = std::make_unique<vkb::core::Buffer>(get_device(), ktx_texture->dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	memcpy(staging_buffer->map(), ktx_texture->pData, ktx_texture->dataSize);

	const auto vk_format = static_cast<VkFormat>(ktx_texture->vkFormat);

	VkExtent3D extent{ktx_texture->baseWidth, ktx_texture->baseHeight, 1};

	std::vector<VkBufferImageCopy>  buffer_copies;
	std::unique_ptr<vkb::sg::Image> image_out;
	{
		std::vector<vkb::sg::Mipmap> mip_maps;
		for (uint32_t mip_level = 0; mip_level < ktx_texture->numLevels; ++mip_level)
		{
			VkExtent3D mip_extent = VkExtent3D{extent.width >> mip_level, extent.height >> mip_level, 1};
			if (!mip_extent.width || !mip_extent.height)
			{
				break;
			}

			ktx_size_t offset{0};
			KTX_CHECK(ktxTexture_GetImageOffset((ktxTexture *) ktx_texture, mip_level, 0, 0, &offset));
			VkBufferImageCopy buffer_image_copy = {};
			buffer_image_copy.imageSubresource  = VkImageSubresourceLayers{VK_IMAGE_ASPECT_COLOR_BIT, mip_level, 0, 1};
			buffer_image_copy.imageExtent       = mip_extent;
			buffer_image_copy.bufferOffset      = offset;
			buffer_copies.push_back(buffer_image_copy);

			vkb::sg::Mipmap mip_map;
			mip_map.extent = buffer_image_copy.imageExtent;
			mip_map.level  = mip_level;
			mip_map.offset = offset;
			mip_maps.push_back(mip_map);
		}

		image_out = std::make_unique<CompressedImage>(get_device(), name, std::move(mip_maps), vk_format);
	}
	auto &vkb_image = image_out->get_vk_image();
	auto  image     = vkb_image.get_handle();

	VkImageSubresourceRange subresource_range{VK_IMAGE_ASPECT_COLOR_BIT, 0, static_cast<uint32_t>(buffer_copies.size()), 0, 1};

	VkCommandBuffer command_buffer = get_device().create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	vkb::image_layout_transition(command_buffer, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresource_range);

	vkCmdCopyBufferToImage(command_buffer, staging_buffer->get_handle(), image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, buffer_copies.size(), buffer_copies.data());

	vkb::image_layout_transition(command_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresource_range);

	device->flush_command_buffer(command_buffer, get_device().get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0).get_handle(), true);

	return image_out;
}

std::vector<uint8_t> TextureCompressionComparison::get_raw_image(const std::string &filename)
{
	if (filename.empty())
	{
		return {};
	}

	std::ifstream                  is(filename, std::ios::binary);
	std::istream_iterator<uint8_t> start(is), end;
	return {start, end};
}

std::pair<std::unique_ptr<vkb::sg::Image>, TextureCompressionComparison::TextureBenchmark> TextureCompressionComparison::compress(const std::string &filename, TextureCompressionComparison::CompressedTexture_t texture_format, const std::string &name)
{
	ktxTexture2 *ktx_texture{nullptr};
	KTX_CHECK(ktxTexture2_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktx_texture));

	TextureBenchmark benchmark;
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

std::unique_ptr<TextureCompressionComparison> create_texture_compression_comparison()
{
	return std::make_unique<TextureCompressionComparison>();
}
