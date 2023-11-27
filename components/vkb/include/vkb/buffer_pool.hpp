#pragma once

#include <memory>

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace vkb
{
// A buffer interface
// This is used to abstract the underlying buffer implementation
class Buffer
{
  public:
	virtual ~Buffer() = default;

	virtual vk::Buffer handle() const = 0;

	// Instead of a getter for each property, we can just return a struct
	struct BufferProperties : vk::BufferCreateInfo
	{};

	// Get the properties of the buffer
	virtual const BufferProperties &properties() const = 0;

	// Copy data from one buffer to another
	virtual void copy_to(Buffer &buffer) const = 0;

	// Upload data to the buffer
	virtual void upload(const void *data, size_t size, size_t offset) = 0;

	// Uses vkGetBufferDeviceAddressKHR to get the device address of the buffer
	virtual uint64_t device_address() const = 0;

	// Upload vector data to the buffer
	template <typename T>
	void upload(const std::vector<T> &data, size_t offset = 0)
	{
		upload(data.data(), data.size() * sizeof(T), offset);
	}

	// Upload typed data to the buffer
	template <typename T>
	void upload(const T &data, size_t offset = 0)
	{
		upload(&data, sizeof(T), offset);
	}
};

// A buffer handle
// If the handle goes out of scope, the buffer is returned to the pool
// The handle therefore represents the buffers lifetime
using BufferHandle = std::shared_ptr<Buffer>;

// A buffer pool manages a set of buffers
// A buffer can be requested and freed
// Buffers are not guaranteed to be freed immediately or at all
class BufferPool
{
  public:
	virtual ~BufferPool() = default;

	// Request a new buffer from the pool
	virtual BufferHandle request_buffer(vk::DeviceSize size, vk::BufferUsageFlags usage, VmaMemoryUsage memory_usage) = 0;

	// Create a GPU only buffer used for scratch memory
	BufferHandle request_scratch_buffer(vk::DeviceSize size)
	{
		return request_buffer(size, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress, VMA_MEMORY_USAGE_GPU_ONLY);
	}

  protected:
	// Release a buffer back to the pool
	// Protected because only the BufferHandle should call this method
	virtual void release_buffer(Buffer *buffer) = 0;
};
}        // namespace vkb