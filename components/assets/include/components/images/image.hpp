#pragma once

#include <memory>
#include <stdint.h>
#include <vector>

#include <volk.h>

#include <components/common/stack_error.hpp>
#include <components/vfs/filesystem.hpp>

namespace components
{
namespace images
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

struct Image;
using ImagePtr = std::shared_ptr<Image>;

struct Image
{
	std::string name;

	VkFormat format{VK_FORMAT_UNDEFINED};

	uint32_t layers;

	std::vector<Mipmap> mips;

	std::vector<uint8_t> data;

	inline bool valid() const
	{
		return format != VK_FORMAT_UNDEFINED && layers > 0 && !mips.empty() && !data.empty();
	}
};

class ImageLoader
{
  public:
	ImageLoader()          = default;
	virtual ~ImageLoader() = default;

	virtual StackErrorPtr load_from_file(const std::string &name, vfs::FileSystem &fs, const std::string &path, ImagePtr *o_image) const
	{
		return StackError::unique("not implemented");
	}

	virtual StackErrorPtr load_from_memory(const std::string &name, const std::vector<uint8_t> &data, ImagePtr *o_image) const
	{
		return StackError::unique("not implemented");
	}
};

class ImageWriter
{
  public:
	ImageWriter()          = default;
	virtual ~ImageWriter() = default;

	virtual StackErrorPtr write_to_file(vfs::FileSystem &fs, const std::string &path, const Image &image) const
	{
		return StackError::unique("not implemented");
	}
};

class ImageEncoder
{
  public:
	ImageEncoder()          = default;
	virtual ~ImageEncoder() = default;

	virtual StackErrorPtr encode(const Image &image, const std::vector<VkFormat> &format_preference, ImagePtr *o_image) const
	{
		return StackError::unique("not implemented");
	}
};

class ImageDecoder
{
  public:
	ImageDecoder()          = default;
	virtual ~ImageDecoder() = default;

	virtual StackErrorPtr decode(const Image &image, const std::vector<VkFormat> &format_preference, ImagePtr *o_image) const
	{
		return StackError::unique("not implemented");
	}
};

class ImageCodec : public ImageEncoder, ImageDecoder
{
  public:
	ImageCodec()          = default;
	virtual ~ImageCodec() = default;
};
}        // namespace images
}        // namespace components