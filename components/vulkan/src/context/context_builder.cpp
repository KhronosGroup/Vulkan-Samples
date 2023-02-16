#include "components/vulkan/context/context_builder.hpp"

#include <stdexcept>

#include <volk.h>

namespace components
{
namespace vulkan
{
InstanceBuilder::ApplicationInfoFunc default_application_info(uint32_t api_version)
{
	return [=]() -> VkApplicationInfo {
		VkApplicationInfo info;
		info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		info.pApplicationName   = "Vulkan Samples";
		info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		info.pEngineName        = "Vulkan Samples Engine";
		info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
		info.apiVersion         = api_version;
		return info;
	};
}

ContextPtr ContextBuilder::build()
{
	InstanceBuilder::Output instance_output = _instance_builder.build();
	if (instance_output.instance == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Failed to create Vulkan Instance");
	}

	PhysicalDeviceBuilder::Output gpu_output = _physical_device_selector.select_physical_device(instance_output.instance);
	if (gpu_output.gpu == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Failed to find a suitable GPU");
	}

	DeviceBuilder::Output device_output = _device_builder.build(gpu_output.gpu, gpu_output.info);
	if (device_output.device == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Failed to create Vulkan Device");
	}

	return std::make_shared<Context>(instance_output.instance, instance_output.debugger_info, gpu_output.gpu, gpu_output.info, device_output.device, device_output.queue_manager);
}

}        // namespace vulkan
}        // namespace components