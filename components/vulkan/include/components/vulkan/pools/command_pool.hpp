#pragma once

#include <queue>
#include <set>

#include "components/vulkan/context/context.hpp"

namespace components
{
namespace vulkan
{
class CommandPool
{
  public:
	CommandPool(ContextPtr context, QueuePtr queue, VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	~CommandPool();

	VkCommandBuffer allocate_command_buffer(VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	void free_command_buffer(VkCommandBuffer command_buffer);

	bool reset_command_buffer(VkCommandBuffer command_buffer, bool release = false);

	void reset_pool(bool release = false);

  private:
	ContextPtr    _context{nullptr};
	VkCommandPool _command_pool{VK_NULL_HANDLE};
	bool          _is_individually_resettable{false};

	std::set<VkCommandBuffer>   _command_buffers;
	std::queue<VkCommandBuffer> _free_command_buffers;
};
}        // namespace vulkan
}        // namespace components