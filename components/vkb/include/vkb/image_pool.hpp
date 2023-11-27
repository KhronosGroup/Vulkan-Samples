#pragma once

#include <memory>

#include <vulkan/vulkan.hpp>

namespace vkb
{
class Image
{
  public:
	virtual ~Image() = default;

	virtual vk::Extent3D extent() const = 0;

	// Uses vkGetBufferDeviceAddressKHR to get the device address of the buffer
	virtual uint64_t device_address() const = 0;

	struct ImageProperties : vk::ImageCreateInfo
	{};

	virtual const ImageProperties &properties() const = 0;
};

using ImageHandle = std::shared_ptr<Image>;

class ImagePool
{
  public:
	virtual ~ImagePool() = default;

	virtual ImageHandle request_image(const Image::ImageProperties &properties) = 0;

  protected:
	virtual void release_image(Image *image) = 0;
};

};        // namespace vkb