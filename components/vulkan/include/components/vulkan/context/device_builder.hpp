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

  private:
	struct Output
	{
		VkDevice              device;
		std::vector<QueuePtr> queues;
	};

	Output build(VkPhysicalDevice gpu, const PhysicalDeviceInfo &info, std::vector<QueuePtr> requested_queues);

	VkPhysicalDeviceFeatures                      _features{};
	std::vector<Queue>                            _enabled_queues;
};

}        // namespace vulkan
}        // namespace components