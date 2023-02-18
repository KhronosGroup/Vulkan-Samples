#pragma once

#include "components/vulkan/context/context.hpp"

#include <vk_mem_alloc.h>

namespace components
{
namespace vulkan
{
struct Allocation
{
	VkBuffer          buffer_handle;
	VmaAllocation     allocation_handle;
	VmaAllocationInfo allocation_info;

	inline VkDeviceSize offset() const noexcept
	{
		return allocation_info.offset;
	}

	inline VkDeviceSize size() const noexcept
	{
		return allocation_info.size;
	}
};

using AllocationPtr = std::weak_ptr<Allocation>;

class BufferPool
{
  public:
	BufferPool(ContextPtr context);
	BufferPool(const BufferPool &)            = delete;
	BufferPool(BufferPool &&)                 = delete;
	BufferPool &operator=(const BufferPool &) = delete;
	BufferPool &operator=(BufferPool &&)      = delete;
	~BufferPool();

	AllocationPtr allocate(const VkBufferCreateInfo &buffer_create_info, const VmaAllocationCreateInfo &allocation_create_info);

  private:
	ContextPtr   _context{nullptr};
	VmaAllocator _allocator{VK_NULL_HANDLE};

	std::vector<std::shared_ptr<Allocation>> _allocations;
};
}        // namespace vulkan
}        // namespace components