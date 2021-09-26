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
}

std::vector<TextureCompressionComparison::CompressedTexture_t> TextureCompressionComparison::get_texture_formats()
{
	return {CompressedTexture_t{&VkPhysicalDeviceFeatures::textureCompressionBC,
								"",
								VK_FORMAT_BC7_SRGB_BLOCK,
								KTX_TTF_BC7_RGBA,
								"KTX_TTF_BC7_RGBA"},
			CompressedTexture_t{&VkPhysicalDeviceFeatures::textureCompressionBC,
								"",
								VK_FORMAT_BC3_SRGB_BLOCK,
								KTX_TTF_BC3_RGBA,
								"KTX_TTF_BC3_RGBA"},
			CompressedTexture_t{&VkPhysicalDeviceFeatures::textureCompressionASTC_LDR,
								"",
								VK_FORMAT_ASTC_4x4_SRGB_BLOCK,
								KTX_TTF_ASTC_4x4_RGBA,
								"KTX_TTF_ASTC_4x4_RGBA"},
			CompressedTexture_t{&VkPhysicalDeviceFeatures::textureCompressionETC2,
								"",
								VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK,
								KTX_TTF_ETC2_RGBA,
								"KTX_TTF_ETC2_RGBA"},
			CompressedTexture_t{nullptr,
								VK_IMG_FORMAT_PVRTC_EXTENSION_NAME,
								VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG,
								KTX_TTF_PVRTC1_4_RGBA,
								"KTX_TTF_PVRTC1_4_RGBA"}};
}

void TextureCompressionComparison::get_available_texture_formats()
{
	available_texture_formats.clear();
	const auto device_features = get_device().get_gpu().get_features();

	const auto all_formats = get_texture_formats();
	std::copy_if(all_formats.cbegin(), all_formats.cend(), std::back_inserter(available_texture_formats), [&](const auto &texture_format) {
		return (texture_format.feature_ptr && device_features.*texture_format.feature_ptr) ||
			   (strlen(texture_format.extension_name) && get_device().is_extension_supported(texture_format.extension_name));
	});
}

void TextureCompressionComparison::load_assets()
{
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
				auto       &name     = name_texture.first;
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

std::unique_ptr<TextureCompressionComparison> create_texture_compression_comparison()
{
	return std::make_unique<TextureCompressionComparison>();
}
