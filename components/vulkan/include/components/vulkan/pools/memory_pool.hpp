#pragma once

#include <mutex>

#include "components/vulkan/context/context.hpp"

#include <vk_mem_alloc.h>

namespace components
{
namespace vulkan
{

class MemoryPool;

/*
 * @brief An allocation is a buffer that has been allocated from a buffer pool
 *
 * @note The allocation is not thread safe, but the buffer pool is
 */
struct Allocation
{
	Allocation()  = default;
	~Allocation() = default;

	inline VkDeviceSize offset() const noexcept
	{
		return allocation_info.offset;
	}

	inline VkDeviceSize size() const noexcept
	{
		return allocation_info.size;
	}

	void upload(const void *data, size_t size, size_t offset = 0);

	void upload(const std::vector<uint8_t> &data, size_t offset = 0);

	template <typename T>
	void upload(const std::vector<T> &data, size_t offset = 0);

	template <typename T>
	void upload(const T &data, size_t offset = 0);

  private:
	friend class MemoryPool;

	VkBuffer          buffer_handle;
	VmaAllocation     allocation_handle;
	VmaAllocationInfo allocation_info;

	MemoryPool        *_pool{nullptr};
	mutable std::mutex _mutex;
};

using AllocationPtr = std::weak_ptr<Allocation>;

class MemoryPool
{
  public:
	MemoryPool(ContextPtr context);
	MemoryPool(const MemoryPool &)            = delete;
	MemoryPool(MemoryPool &&)                 = delete;
	MemoryPool &operator=(const MemoryPool &) = delete;
	MemoryPool &operator=(MemoryPool &&)      = delete;
	~MemoryPool();

	AllocationPtr allocate(const VkBufferCreateInfo &buffer_create_info, const VmaAllocationCreateInfo &allocation_create_info);

	void free(AllocationPtr allocation);

	void upload(const Allocation &allocation, const void *data, size_t size, size_t offset = 0);

	inline size_t allocation_count() const noexcept
	{
		return _allocations.size();
	}

	inline void upload(const Allocation &allocation, const std::vector<uint8_t> &data, size_t offset = 0)
	{
		upload(allocation, data.data(), sizeof(data[0]) * data.size(), offset);
	}

	template <typename T>
	inline void upload(const Allocation &allocation, const T &data, size_t offset = 0)
	{
		upload(allocation, &data, sizeof(T), offset);
	}

	void record_copy(VkCommandBuffer cmd, const AllocationPtr src, const AllocationPtr dest, uint32_t offset = 0);

  private:
	ContextPtr   _context{nullptr};
	VmaAllocator _allocator{VK_NULL_HANDLE};

	std::vector<std::shared_ptr<Allocation>> _allocations;
};

inline void Allocation::upload(const void *data, size_t size, size_t offset)
{
	if (_pool)
	{
		_pool->upload(*this, data, size, offset);
	}
}

inline void Allocation::upload(const std::vector<uint8_t> &data, size_t offset)
{
	if (_pool)
	{
		_pool->upload(*this, data, offset);
	}
}

template <typename T>
inline void Allocation::upload(const std::vector<T> &data, size_t offset)
{
	if (_pool)
	{
		_pool->upload(*this, data.data(), sizeof(T) * data.size(), offset);
	}
}

template <typename T>
inline void Allocation::upload(const T &data, size_t offset)
{
	if (_pool)
	{
		_pool->upload(*this, data, offset);
	}
}
}        // namespace vulkan
}        // namespace components