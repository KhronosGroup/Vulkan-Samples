#include "components/vulkan/pools/memory_pool.hpp"

#include <cassert>
#include <cstring>

#include "macros.hpp"

namespace components
{
namespace vulkan
{
MemoryPool::MemoryPool(ContextPtr context) :
    _context{context}
{
	assert(_context != nullptr);

	VmaAllocatorCreateInfo allocator_create_info{};
	allocator_create_info.physicalDevice = _context->gpu;
	allocator_create_info.device         = _context->device;
	allocator_create_info.instance       = _context->instance;

	VK_CHECK(vmaCreateAllocator(&allocator_create_info, &_allocator), "failed to initialize VMA");
}

AllocationPtr MemoryPool::allocate(const VkBufferCreateInfo &buffer_create_info, const VmaAllocationCreateInfo &allocation_create_info)
{
	assert(_allocator != VK_NULL_HANDLE);

	VkBuffer          buffer_handle;
	VmaAllocation     allocation_handle;
	VmaAllocationInfo allocation_info;

	VK_CHECK(vmaCreateBuffer(_allocator, &buffer_create_info, &allocation_create_info, &buffer_handle, &allocation_handle, &allocation_info), "failed to allocate buffer");

	auto allocation               = std::make_shared<Allocation>();
	allocation->buffer_handle     = buffer_handle;
	allocation->allocation_handle = allocation_handle;
	allocation->allocation_info   = allocation_info;
	allocation->_pool             = this;

	_allocations.push_back(allocation);

	return std::weak_ptr<Allocation>(std::move(allocation));
}

void MemoryPool::free(AllocationPtr allocation)
{
	if (auto ptr = allocation.lock())
	{
		for (auto it = _allocations.begin(); it != _allocations.end(); ++it)
		{
			if (*it == ptr)
			{
				vmaDestroyBuffer(_allocator, ptr->buffer_handle, ptr->allocation_handle);
				_allocations.erase(it);
				return;
			}
		}
	}
}

void MemoryPool::upload(const Allocation &allocation, const void *data, size_t size, size_t offset)
{
	std::lock_guard<std::mutex> lock(allocation._mutex);

	if (allocation.size() < size + offset)
	{
		throw std::runtime_error("upload size is too big for the allocation");
	}

	if (void *persistentMapped = allocation.allocation_info.pMappedData)
	{
		// allocation was created with VMA_ALLOCATION_CREATE_MAPPED_BIT flag
		std::memcpy(static_cast<uint8_t *>(persistentMapped) + offset, data, size);
	}
	else
	{
		void *mapped;
		VK_CHECK(vmaMapMemory(_allocator, allocation.allocation_handle, &mapped), "unable to map memory");
		std::memcpy(static_cast<uint8_t *>(mapped) + offset, data, size);
		vmaUnmapMemory(_allocator, allocation.allocation_handle);
	}
}

void MemoryPool::record_copy(VkCommandBuffer cmd, const AllocationPtr src, const AllocationPtr dest, uint32_t offset)
{
	auto src_buffer  = src.lock();
	auto dest_buffer = dest.lock();

	VkBufferCopy copy_region = {};
	copy_region.size         = src_buffer->size();
	copy_region.dstOffset    = offset;
	copy_region.srcOffset    = 0;

	vkCmdCopyBuffer(cmd, src_buffer->buffer_handle, dest_buffer->buffer_handle, 1, &copy_region);
}

MemoryPool::~MemoryPool()
{
	assert(_allocator != VK_NULL_HANDLE);

	for (auto &allocation : _allocations)
	{
		vmaDestroyBuffer(_allocator, allocation->buffer_handle, allocation->allocation_handle);
	}

	vmaDestroyAllocator(_allocator);
}
}        // namespace vulkan
}        // namespace components