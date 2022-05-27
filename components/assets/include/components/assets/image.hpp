#pragma once

#include <memory>
#include <string>
#include <stdint.h>
#include <vector>

#include <volk.h>

namespace components
{
namespace assets
{
struct Mipmap
{
	/// Mipmap level
	uint32_t level = 0;

	/// Byte offset used for uploading
	uint32_t offset = 0;

	// Byte length of image
	uint32_t byte_length = 0;

	/// Width depth and height of the mipmap
	VkExtent3D extent = {0, 0, 0};
};

struct ImageAsset;
using ImageAssetPtr = std::shared_ptr<ImageAsset>;

struct ImageAsset
{
	std::string name{""};

	VkFormat format{VK_FORMAT_UNDEFINED};

	uint32_t layers{0U};

	std::vector<Mipmap> mips{};

	std::vector<uint8_t> data{};

	inline bool valid() const
	{
		return format != VK_FORMAT_UNDEFINED && layers > 0 && !mips.empty() && !data.empty();
	}

	inline uint32_t width() const
	{
		if (mips.size() == 0)
		{
			return 0;
		}

		return mips[0].extent.width;
	}

	inline uint32_t height() const
	{
		if (mips.size() == 0)
		{
			return 0;
		}

		return mips[0].extent.height;
	}
};
}        // namespace images
}        // namespace components