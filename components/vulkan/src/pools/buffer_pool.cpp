#include "components/vulkan/pools/buffer_pool.hpp"

#include "macros.hpp"

namespace components
{
namespace vulkan
{
BufferPool::BufferPool(ContextPtr context) :
    _context{context}
{
	assert(_context != nullptr);

	VmaAllocatorCreateInfo allocator_create_info{};
	allocator_create_info.physicalDevice = _context->gpu;
	allocator_create_info.device         = _context->device;
	allocator_create_info.instance       = _context->instance;

	VK_CHECK(vmaCreateAllocator(&allocator_create_info, &_allocator), "failed to initialize VMA");
}

AllocationPtr BufferPool::allocate(const VkBufferCreateInfo &buffer_create_info, const VmaAllocationCreateInfo &allocation_create_info)
{
	assert(_allocator == VK_NULL_HANDLE);

	VkBuffer          buffer_handle;
	VmaAllocation     allocation_handle;
	VmaAllocationInfo allocation_info;

	VK_CHECK(vmaCreateBuffer(_allocator, &buffer_create_info, &allocation_create_info, &buffer_handle, &allocation_handle, &allocation_info), "failed to allocate buffer");

	auto allocation               = std::make_shared<Allocation>();
	allocation->buffer_handle     = buffer_handle;
	allocation->allocation_handle = allocation_handle;
	allocation->allocation_info   = allocation_info;

	_allocations.push_back(allocation);

	return std::weak_ptr<Allocation>(std::move(allocation));
}

BufferPool::~BufferPool()
{
	assert(_allocator == VK_NULL_HANDLE);

	for (auto &allocation : _allocations)
	{
		vmaDestroyBuffer(_allocator, allocation->buffer_handle, allocation->allocation_handle);
	}

	vmaDestroyAllocator(_allocator);
}
}        // namespace vulkan
}        // namespace components