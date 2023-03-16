#pragma once

#include <memory>
#include <optional>
#include <unordered_map>

#include <volk.h>

#include <components/vulkan/context/context.hpp>

namespace components
{
namespace vulkan
{
class DeviceBuilder;

class Queue
{
  public:
	inline bool is_valid() const noexcept
	{
		return get_handle() != VK_NULL_HANDLE;
	}

	inline uint32_t family_index() const noexcept
	{
		return _queue_info.has_value() ? _queue_info->_family_index : 0;
	}

	inline VkQueue get_handle() const noexcept
	{
		return _queue_info.has_value() ? _queue_info->_queue : VK_NULL_HANDLE;
	}

	inline VkQueueFlags requested_queue_types() const noexcept
	{
		return _requested_queue_types;
	}

	inline std::vector<VkSurfaceKHR> requested_surfaces() const noexcept
	{
		return _requested_surfaces;
	}

  private:
  protected:
	Queue(VkQueueFlags queue_types, const std::vector<VkSurfaceKHR> &presentable_surfaces = {}) :
	    _requested_queue_types(queue_types), _requested_surfaces{presentable_surfaces}
	{
	}

  private:        // Physical Device Builder usage
	friend class ContextBuilder;
	friend class PhysicalDeviceBuilder;
	friend class DeviceBuilder;

	VkQueueFlags              _requested_queue_types{VK_QUEUE_FLAG_BITS_MAX_ENUM};
	std::vector<VkSurfaceKHR> _requested_surfaces;

	struct QueueInfo
	{
		VkQueue  _queue{VK_NULL_HANDLE};
		uint32_t _family_index{0};
		uint32_t _index{0};
	};

	std::optional<QueueInfo> _queue_info;
};

using QueuePtr = std::shared_ptr<Queue>;

}        // namespace vulkan
}        // namespace components
