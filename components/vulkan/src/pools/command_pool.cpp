#include "components/vulkan/pools/command_pool.hpp"

#include <cassert>

#include "macros.hpp"

namespace components
{
namespace vulkan
{

CommandPool::CommandPool(ContextPtr context, QueuePtr queue, VkCommandPoolCreateFlags flags) :
    _context{context},
    _is_individually_resettable{(flags & VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT) == flags}
{
	assert(context && "Context is null");
	assert(queue && "Queue is null");
	assert(queue->is_valid() && "Queue is not valid");

	VkCommandPoolCreateInfo command_pool_create_info{};
	command_pool_create_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_create_info.queueFamilyIndex = queue->family_index();
	command_pool_create_info.flags            = flags;

	VK_CHECK(vkCreateCommandPool(_context->device, &command_pool_create_info, nullptr, &_command_pool), "Failed to create command pool");
}

CommandPool::~CommandPool()
{
	assert(_context && "Context is null");

	reset_pool(true);

	if (_command_pool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(_context->device, _command_pool, nullptr);
	}
}

VkCommandBuffer CommandPool::allocate_command_buffer(VkCommandBufferLevel level)
{
	assert(_context && "Context is null");

	if (!_free_command_buffers.empty())
	{
		VkCommandBuffer command_buffer = _free_command_buffers.front();
		_free_command_buffers.pop();
		return command_buffer;
	}

	VkCommandBufferAllocateInfo command_buffer_allocate_info{};
	command_buffer_allocate_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_allocate_info.commandPool        = _command_pool;
	command_buffer_allocate_info.level              = level;
	command_buffer_allocate_info.commandBufferCount = 1;

	VkCommandBuffer command_buffer{VK_NULL_HANDLE};
	VK_CHECK(vkAllocateCommandBuffers(_context->device, &command_buffer_allocate_info, &command_buffer), "Failed to allocate command buffer");

	_command_buffers.emplace(command_buffer);

	return command_buffer;
}

void CommandPool::free_command_buffer(VkCommandBuffer command_buffer)
{
	vkFreeCommandBuffers(_context->device, _command_pool, 1, &command_buffer);
	_command_buffers.erase(command_buffer);
}

bool CommandPool::reset_command_buffer(VkCommandBuffer command_buffer, bool release)
{
	assert(_context && "Context is null");

	VkCommandBufferResetFlags flags = release ? VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT : 0;
	if (_is_individually_resettable)
	{
		VK_CHECK(vkResetCommandBuffer(command_buffer, flags), "Failed to reset command buffer");

		if (!release)
		{
			_free_command_buffers.push(command_buffer);
		}
		return true;
	}

	return false;
}

void CommandPool::reset_pool(bool release)
{
	assert(_context && "Context is null");
	VkCommandPoolResetFlags flags = release ? VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT : 0;
	VK_CHECK(vkResetCommandPool(_context->device, _command_pool, flags), "Failed to reset command pool");
	if (!release)
	{
		while (!_free_command_buffers.empty())
		{
			_free_command_buffers.pop();
		}

		for (auto command_buffer : _command_buffers)
		{
			_free_command_buffers.push(command_buffer);
		}
	}
}
}        // namespace vulkan
}        // namespace components