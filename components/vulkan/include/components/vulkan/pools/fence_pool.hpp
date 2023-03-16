#pragma once

#include <queue>
#include <set>

#include "components/vulkan/context/context.hpp"
#include "macros.hpp"

namespace components
{
namespace vulkan
{
inline VkFence create_fence(ContextPtr context, VkFenceCreateFlags flags = 0)
{
	VkFenceCreateInfo create_info;
	create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = flags;

	VkFence fence{VK_NULL_HANDLE};
	VK_CHECK(vkCreateFence(context->device, &create_info, nullptr, &fence), "failed to create fence");
	return fence;
}

inline bool wait_fence(ContextPtr context, VkFence fence, uint64_t timeout = UINT64_MAX)
{
	VkResult result = vkWaitForFences(context->device, 1, &fence, VK_TRUE, timeout);
	if (result == VK_TIMEOUT)
	{
		return false;
	}
	VK_CHECK(result, "failed to wait for fence");
	return true;
}

inline void reset_fence(ContextPtr context, VkFence fence)
{
	VK_CHECK(vkResetFences(context->device, 1, &fence), "failed to reset fence");
}

class FencePool
{
  public:
	FencePool(ContextPtr context);

	~FencePool();

	VkFence acquire_fence(VkFenceCreateFlags flags = 0);

	// wait on all fences in the pool
	// false if the timeout was reached
	bool wait_all(uint64_t timeout = UINT64_MAX);

	bool wait_fence(VkFence fence, uint64_t timeout = UINT64_MAX);

	void reset_fence(VkFence fence);

	void reset_fences();

  private:
	ContextPtr          _context{nullptr};
	std::set<VkFence>   _fences;
	std::queue<VkFence> _free_fences;
};
}        // namespace vulkan
}        // namespace components