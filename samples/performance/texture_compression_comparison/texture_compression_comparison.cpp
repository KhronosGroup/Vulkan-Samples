#include "texture_compression_comparison.h"

TextureCompressionComparison::TextureCompressionComparison()
{
	title = "Texture Compression Comparison";
}
TextureCompressionComparison::~TextureCompressionComparison()
{
}

void TextureCompressionComparison::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = VK_TRUE;
	}
}

bool TextureCompressionComparison::prepare(vkb::Platform &platform)
{
}

void TextureCompressionComparison::render(float delta_time)
{
}

void TextureCompressionComparison::view_changed()
{
}

void TextureCompressionComparison::on_update_ui_overlay(vkb::Drawer &drawer)
{
}

std::unique_ptr<TextureCompressionComparison> create_texture_compression_comparison()
{
}

void TextureCompressionComparison::build_command_buffers()
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
