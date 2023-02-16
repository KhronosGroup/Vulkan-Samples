#pragma once

#include "components/vulkan/context/extension_builder.hpp"
#include "components/vulkan/context/physical_device_builder.hpp"

namespace components
{
namespace vulkan
{

/**
 * @brief Allows a Sample to configure the created device including Extensions and Features
 */
class DeviceBuilder final : public DeviceExtensionBuilder
{
	CVC_CREATE_BUILDER(DeviceBuilder)
  public:
	using FeatureFunc = std::function<void(VkPhysicalDeviceFeatures &)>;
	Self &configure_features(FeatureFunc &&func);
	Self &enable_queue(VkQueueFlagBits queue_type, uint32_t required_queue_count = 1);
	Self &must_support_presentation(VkSurfaceKHR surface);

  private:
	struct Output
	{
		VkDevice     device;
		QueueManager queue_manager;
	};

	Output build(VkPhysicalDevice gpu, const PhysicalDeviceInfo &info);

	std::unordered_map<VkQueueFlagBits, uint32_t> _required_queue_counts;
	VkSurfaceKHR                                  _surface{VK_NULL_HANDLE};
	VkPhysicalDeviceFeatures                      _features{};
};

}        // namespace vulkan
}        // namespace components