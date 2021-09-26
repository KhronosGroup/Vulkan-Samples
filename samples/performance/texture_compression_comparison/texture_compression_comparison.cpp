#include "texture_compression_comparison.h"
#include "rendering/subpasses/forward_subpass.h"
#include "scene_graph/components/camera.h"
#include "scene_graph/components/image.h"
#include "scene_graph/components/material.h"
#include "scene_graph/components/mesh.h"

#define KTX_CHECK(x)                                                    \
	do                                                                  \
	{                                                                   \
		KTX_error_code err = x;                                         \
		if (err != KTX_SUCCESS)                                         \
		{                                                               \
			LOGE("Detected KTX error: {}", static_cast<uint32_t>(err)); \
			abort();                                                    \
		}                                                               \
	} while (0)

TextureCompressionComparison::TextureCompressionComparison()
{
}
TextureCompressionComparison::~TextureCompressionComparison()
{
}

bool TextureCompressionComparison::prepare(vkb::Platform &platform)
{
	if (!VulkanSample::prepare(platform))
	{
		return false;
	}

	load_assets();

	auto &camera_node = vkb::add_free_camera(*scene, "main_camera", get_render_context().get_surface_extent());
	camera            = &camera_node.get_component<vkb::sg::Camera>();

	vkb::ShaderSource vert_shader("base.vert");
	vkb::ShaderSource frag_shader("base.frag");
	auto              scene_subpass = std::make_unique<vkb::ForwardSubpass>(get_render_context(), std::move(vert_shader), std::move(frag_shader), *scene, *camera);

	auto render_pipeline = vkb::RenderPipeline();
	render_pipeline.add_subpass(std::move(scene_subpass));

	set_render_pipeline(std::move(render_pipeline));

	stats->request_stats({vkb::StatIndex::frame_times});
	gui = std::make_unique<vkb::Gui>(*this, platform.get_window(), stats.get());

	return true;
}

void TextureCompressionComparison::update(float delta_time)
{
	VulkanSample::update(delta_time);
}

void TextureCompressionComparison::draw_gui()
{
	gui->show_options_window([this]() {

	});
}

const std::vector<TextureCompressionComparison::CompressedTexture_t> &TextureCompressionComparison::get_texture_formats()
{
	static std::vector<TextureCompressionComparison::CompressedTexture_t> formats = {
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
							"PVRTC1 4"},
		CompressedTexture_t{nullptr,
							"",
							VK_FORMAT_R8G8B8A8_SRGB,
							KTX_TTF_RGBA32,
							"KTX_TTF_RGBA32",
							"RGBA 32",
							true}};
	return formats;
}

bool TextureCompressionComparison::is_texture_format_supported(const TextureCompressionComparison::CompressedTexture_t &format)
{
	const auto device_features = get_device().get_gpu().get_features();

	const bool supported_by_feature   = format.feature_ptr && device_features.*format.feature_ptr;
	const bool supported_by_extension = strlen(format.extension_name) && get_device().is_extension_supported(format.extension_name);
	const bool supported_by_default   = format.always_suported;

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
	load_scene("scenes/sponza/Sponza01.gltf");
	if (!scene)
	{
		throw std::runtime_error("Unable to load Sponza scene");
	}

	// Decompress all of the textures and store in memory
	// We will use these textures later when performing benchmarks / comparisons
	for (auto &&mesh : scene->get_components<vkb::sg::Mesh>())
	{
		for (auto &&sub_mesh : mesh->get_submeshes())
		{
			auto material = sub_mesh->get_material();
			for (auto &name_texture : material->textures)
			{
				auto       &texture  = name_texture.second;
				auto        image    = texture->get_image();
				std::string filename = image->get_name();
				if (image && texture_raw_data.find(filename) == texture_raw_data.cend())
				{
					texture_raw_data[filename] = get_raw_image(vkb::fs::path::get(vkb::fs::path::Type::Assets) + "scenes/sponza/ktx2/" + filename + "2");
				}
			}
		}
	}
}

std::unique_ptr<vkb::sg::Image> TextureCompressionComparison::create_image(ktxTexture2 *ktx_texture)
{
	std::unique_ptr<vkb::core::Buffer> staging_buffer = std::make_unique<vkb::core::Buffer>(get_device(), ktx_texture->dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	memcpy(staging_buffer->map(), ktx_texture->pData, ktx_texture->dataSize);

	const VkFormat vk_format = static_cast<VkFormat>(ktx_texture->vkFormat);

	VkExtent3D              extent{ktx_texture->baseWidth, ktx_texture->baseHeight, 1};
	VkImageSubresourceRange subresource_range{VK_IMAGE_ASPECT_COLOR_BIT, 0, ktx_texture->numLevels, 0, 1};

	/*
		VkImageCreateInfo image_create = vkb::initializers::image_create_info();
		image_create.imageType         = VK_IMAGE_TYPE_2D;
		image_create.format            = vk_format;
		image_create.extent            = extent;
		image_create.mipLevels         = 1;
		image_create.arrayLayers       = 1;
		image_create.samples           = VK_SAMPLE_COUNT_1_BIT;
		image_create.tiling            = VK_IMAGE_TILING_OPTIMAL;
		image_create.usage             = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		image_create.initialLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

		VkImage image{VK_NULL_HANDLE};
		VK_CHECK(vkCreateImage(get_device().get_handle(), &image_create, VK_NULL_HANDLE, &image));

		VkImageViewCreateInfo image_view_create = vkb::initializers::image_view_create_info();
		image_view_create.viewType              = VK_IMAGE_VIEW_TYPE_2D;
		image_view_create.format                = vk_format;
		image_view_create.subresourceRange      = subresource_range;
		image_view_create.image                 = image;

		VkImageView image_view{VK_NULL_HANDLE};
		VK_CHECK(vkCreateImageView(get_device().get_handle(), &image_view_create, VK_NULL_HANDLE, &image_view));*/

	std::vector<VkBufferImageCopy>  buffer_copies;
	std::unique_ptr<vkb::sg::Image> image_out;
	{
		std::vector<vkb::sg::Mipmap> mip_maps;
		for (uint32_t mip_level = 0; mip_level < ktx_texture->numLevels; ++mip_level)
		{
			ktx_size_t offset{0};
			KTX_CHECK(ktxTexture_GetImageOffset((ktxTexture *) ktx_texture, mip_level, 0, 0, &offset));
			VkBufferImageCopy buffer_image_copy = {};
			buffer_image_copy.imageSubresource  = VkImageSubresourceLayers{VK_IMAGE_ASPECT_COLOR_BIT, mip_level, 0, 1};
			buffer_image_copy.imageExtent       = VkExtent3D{extent.width >> mip_level, extent.height >> mip_level, 1};
			buffer_image_copy.bufferOffset      = offset;
			buffer_copies.push_back(buffer_image_copy);

			vkb::sg::Mipmap mip_map;
			mip_map.extent = buffer_image_copy.imageExtent;
			mip_map.level  = mip_level;
			mip_map.offset = offset;
			mip_maps.push_back(mip_map);
		}

		image_out = std::make_unique<vkb::sg::Image>("", std::vector<uint8_t>{}, std::move(mip_maps));
	}
	image_out->create_vk_image(get_device(), VK_IMAGE_VIEW_TYPE_2D);
	auto &vkb_image = image_out->get_vk_image();
	auto  image     = vkb_image.get_handle();

	VkCommandBuffer command_buffer = get_device().create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	vkCmdCopyBufferToImage(command_buffer, staging_buffer->get_handle(), image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, buffer_copies.size(), buffer_copies.data());

	VkImageMemoryBarrier image_memory_barrier;
	image_memory_barrier.image            = image;
	image_memory_barrier.subresourceRange = subresource_range;
	image_memory_barrier.srcAccessMask    = VK_ACCESS_TRANSFER_WRITE_BIT;
	image_memory_barrier.dstAccessMask    = VK_ACCESS_TRANSFER_READ_BIT;
	image_memory_barrier.oldLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	image_memory_barrier.newLayout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &image_memory_barrier);
	device->flush_command_buffer(command_buffer, get_device().get_queue_by_flags(VK_QUEUE_TRANSFER_BIT, 0).get_handle(), true);

	return image_out;
}

std::vector<uint8_t> TextureCompressionComparison::get_raw_image(const std::string &filename)
{
	if (filename.empty())
	{
		return {};
	}
	ktxTexture2 *ktx_texture = VK_NULL_HANDLE;
	KTX_CHECK(ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, (ktxTexture **) &ktx_texture));

	if (!ktx_texture)
	{
		throw std::runtime_error("Unable to create texture from memory: null result provided.");
	}

	// Transcode to an uncompressed format so that encoding can be benchmarked
	ktxTexture2_TranscodeBasis(ktx_texture, KTX_TTF_RGBA32, 0);

	std::vector<uint8_t> out;
	if (ktx_texture->dataSize && ktx_texture->pData)
	{
		out.resize(ktx_texture->dataSize);
		memcpy(out.data(), ktx_texture->pData, ktx_texture->dataSize);
	}
	ktxTexture_Destroy((ktxTexture *) ktx_texture);
	return out;
}

std::pair<std::unique_ptr<vkb::sg::Image>, TextureCompressionComparison::TextureBenchmark> TextureCompressionComparison::compress(const std::string &filename, TextureCompressionComparison::CompressedTexture_t texture_format)
{
	auto iter = texture_raw_data.find(filename);
	if (iter == texture_raw_data.cend())
	{
		throw std::runtime_error(std::string("Unable to find raw data for ").append(filename));
	}
	const std::vector<uint8_t> &bytes       = iter->second;
	ktxTexture2	            *ktx_texture = VK_NULL_HANDLE;
	KTX_CHECK(ktxTexture_CreateFromMemory(bytes.data(), bytes.size(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, (ktxTexture **) &ktx_texture));
	assert(!!ktx_texture);

	TextureBenchmark benchmark;
	{
		const auto start = std::chrono::high_resolution_clock::now();
		ktxTexture2_TranscodeBasis(ktx_texture, texture_format.ktx_format, 0);
		const auto end             = std::chrono::high_resolution_clock::now();
		benchmark.compress_time_ms = static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()) / 1000.f;
	}
	benchmark.total_bytes = ktx_texture->dataSize;
	auto image            = create_image(ktx_texture);
	ktxTexture_Destroy((ktxTexture *) ktx_texture);

	return {std::move(image), benchmark};
}

std::unique_ptr<TextureCompressionComparison> create_texture_compression_comparison()
{
	return std::make_unique<TextureCompressionComparison>();
}
