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

ContextBuilder &ContextBuilder::request_queue(VkQueueFlags queue_types, const std::vector<VkSurfaceKHR> &presentable_surfaces, QueuePtr *out_queue)
{
	auto queue = std::shared_ptr<Queue>(new Queue(queue_types, presentable_surfaces));

	select_gpu()
	    .score_device(scores::supports_queue(queue))
	    .done();

	_requested_queues.push_back(queue);

	if (queue != nullptr)
	{
		*out_queue = queue;
	}

	return *this;
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

	DeviceBuilder::Output device_output = _device_builder.build(gpu_output.gpu, gpu_output.info, _requested_queues);
	if (device_output.device == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Failed to create Vulkan Device");
	}

	return std::make_shared<Context>(
	    instance_output.instance,
	    instance_output.debugger_info,
	    gpu_output.gpu,
	    gpu_output.info,
	    device_output.device,
	    device_output.queues);
}

}        // namespace vulkan
}        // namespace components