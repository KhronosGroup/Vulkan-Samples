#include "components/vulkan/pools/fence_pool.hpp"

#include <cassert>

namespace components
{
namespace vulkan
{
FencePool::FencePool(ContextPtr context) :
    _context{context}
{
}

FencePool::~FencePool()
{
	for (auto fence : _fences)
	{
		vkDestroyFence(_context->device, fence, nullptr);
	}
}

VkFence FencePool::acquire_fence(VkFenceCreateFlags flags)
{
	if (!_free_fences.empty())
	{
		VkFence fence = _free_fences.front();
		_free_fences.pop();
		return fence;
	}

	VkFence fence = create_fence(_context, flags);
	_fences.emplace(fence);
	return fence;
}

// wait on all fences in the pool
// false if the timeout was reached
bool FencePool::wait_all(uint64_t timeout)
{
	std::vector<VkFence> fences{_fences.begin(), _fences.end()};

	VkResult result = vkWaitForFences(_context->device, static_cast<uint32_t>(fences.size()), fences.data(), VK_TRUE, timeout);

	if (result == VK_TIMEOUT)
	{
		return false;
	}

	VK_CHECK(result, "failed to wait for fences");

	return true;
}

bool FencePool::wait_fence(VkFence fence, uint64_t timeout)
{
	assert(_fences.find(fence) != _fences.end() && "fence not in pool");

	VkResult result = vkWaitForFences(_context->device, 1, &fence, VK_TRUE, timeout);
	if (result == VK_TIMEOUT)
	{
		return false;
	}

	VK_CHECK(result, "failed to wait for fence");

	return true;
}

void FencePool::reset_fence(VkFence fence)
{
	assert(_fences.find(fence) != _fences.end() && "fence not in pool");

	wait_fence(fence);
	VK_CHECK(vkResetFences(_context->device, 1, &fence), "failed to reset fences");

	for (auto fence : _fences)
	{
		_free_fences.push(fence);
	}
}

void FencePool::reset_fences()
{
	std::vector<VkFence> fences{_fences.begin(), _fences.end()};

	wait_all();
	VK_CHECK(vkResetFences(_context->device, static_cast<uint32_t>(fences.size()), fences.data()), "failed to reset fences");

	for (auto fence : _fences)
	{
		_free_fences.push(fence);
	}
}

}        // namespace vulkan
}        // namespace components