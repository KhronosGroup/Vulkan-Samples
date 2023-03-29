#include "components/vulkan/context/device_builder.hpp"

#include <cassert>

#include <components/common/map.hpp>
#include <components/common/strings.hpp>

#ifdef min
#	undef min
#endif

namespace components
{
namespace vulkan
{

DeviceBuilder &DeviceBuilder::configure_features(FeatureFunc &&func)
{
	func(_features);
	return *this;
}

struct QueueFamily
{
	uint32_t family_index;

	inline void add_queue(QueuePtr queue)
	{
		queues.push_back(queue);
	}

	std::vector<QueuePtr> queues;
};

DeviceBuilder::Output DeviceBuilder::build(VkPhysicalDevice gpu, const PhysicalDeviceInfo &info, std::vector<QueuePtr> requested_queues)
{
	VkDeviceCreateInfo device_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};

	Map<uint32_t, QueueFamily> allocated_families;

	for (auto queue : requested_queues)
	{
		for (auto family : info.queue_families)
		{
			bool use_family = true;

			// make sure requested surfaces are supported
			for (VkSurfaceKHR surface : queue->requested_surfaces())
			{
				// prioritize the first queue which can present to the requested surface
				for (const auto &family : info.queue_families)
				{
					VkBool32 present_supported{false};
					VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(gpu, family.index, surface, &present_supported), "failed to query for presentation support");

					if (present_supported)
					{
						use_family = false;
						break;
					}
				}

				if (!use_family)
				{
					break;
				}
			}

			if (!use_family)
			{
				continue;
			}

			// make sure that supported queue types are supported
			if ((queue->requested_queue_types() & family.properties.queueFlags) != queue->requested_queue_types())
			{
				continue;
			}

			auto it = allocated_families.find_or_create(family.index, [&]() { return QueueFamily{family.index}; });
			if (family.properties.queueCount > it->second.queues.size())
			{
				it->second.add_queue(queue);
				break;
			}
		}
	}

	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
	queue_create_infos.reserve(allocated_families.size());

	for (const auto &[index, family] : allocated_families)
	{
		if (family.queues.size() == 0)
		{
			continue;
		}

		float queue_priority = 1.0f;

		VkDeviceQueueCreateInfo queue_create_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
		queue_create_info.queueFamilyIndex = family.family_index;
		queue_create_info.queueCount       = static_cast<uint32_t>(family.queues.size());
		queue_create_info.pQueuePriorities = &queue_priority;

		queue_create_infos.push_back(queue_create_info);
	}

	std::vector<const char *> enabled_extensions_cstr = strings::to_cstr(collect_enabled_extensions(device_info, gpu));
	device_info.enabledExtensionCount                 = static_cast<uint32_t>(enabled_extensions_cstr.size());
	device_info.ppEnabledExtensionNames               = enabled_extensions_cstr.data();

	std::vector<const char *> enabled_layers_cstr = strings::to_cstr(collect_enabled_layers(device_info, gpu));
	device_info.enabledLayerCount                 = static_cast<uint32_t>(enabled_layers_cstr.size());
	device_info.ppEnabledLayerNames               = enabled_layers_cstr.data();

	device_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
	device_info.pQueueCreateInfos    = queue_create_infos.size() > 0 ? queue_create_infos.data() : nullptr;

// check that all features are supported before calling vkCreateDevice
#define FEATURE_IS_SUPPORTED(name)                                                                  \
	if ((_features.name == VK_TRUE) && info.features.name != VK_TRUE)                               \
	{                                                                                               \
		throw std::runtime_error("Requested device feature " #name " is not supported by the GPU"); \
	}

	FEATURE_IS_SUPPORTED(robustBufferAccess)
	FEATURE_IS_SUPPORTED(fullDrawIndexUint32)
	FEATURE_IS_SUPPORTED(imageCubeArray)
	FEATURE_IS_SUPPORTED(independentBlend)
	FEATURE_IS_SUPPORTED(geometryShader)
	FEATURE_IS_SUPPORTED(tessellationShader)
	FEATURE_IS_SUPPORTED(sampleRateShading)
	FEATURE_IS_SUPPORTED(dualSrcBlend)
	FEATURE_IS_SUPPORTED(logicOp)
	FEATURE_IS_SUPPORTED(multiDrawIndirect)
	FEATURE_IS_SUPPORTED(drawIndirectFirstInstance)
	FEATURE_IS_SUPPORTED(depthClamp)
	FEATURE_IS_SUPPORTED(depthBiasClamp)
	FEATURE_IS_SUPPORTED(fillModeNonSolid)
	FEATURE_IS_SUPPORTED(depthBounds)
	FEATURE_IS_SUPPORTED(wideLines)
	FEATURE_IS_SUPPORTED(largePoints)
	FEATURE_IS_SUPPORTED(alphaToOne)
	FEATURE_IS_SUPPORTED(multiViewport)
	FEATURE_IS_SUPPORTED(samplerAnisotropy)
	FEATURE_IS_SUPPORTED(textureCompressionETC2)
	FEATURE_IS_SUPPORTED(textureCompressionASTC_LDR)
	FEATURE_IS_SUPPORTED(textureCompressionBC)
	FEATURE_IS_SUPPORTED(occlusionQueryPrecise)
	FEATURE_IS_SUPPORTED(pipelineStatisticsQuery)
	FEATURE_IS_SUPPORTED(vertexPipelineStoresAndAtomics)
	FEATURE_IS_SUPPORTED(fragmentStoresAndAtomics)
	FEATURE_IS_SUPPORTED(shaderTessellationAndGeometryPointSize)
	FEATURE_IS_SUPPORTED(shaderImageGatherExtended)
	FEATURE_IS_SUPPORTED(shaderStorageImageExtendedFormats)
	FEATURE_IS_SUPPORTED(shaderStorageImageMultisample)
	FEATURE_IS_SUPPORTED(shaderStorageImageReadWithoutFormat)
	FEATURE_IS_SUPPORTED(shaderStorageImageWriteWithoutFormat)
	FEATURE_IS_SUPPORTED(shaderUniformBufferArrayDynamicIndexing)
	FEATURE_IS_SUPPORTED(shaderSampledImageArrayDynamicIndexing)
	FEATURE_IS_SUPPORTED(shaderStorageBufferArrayDynamicIndexing)
	FEATURE_IS_SUPPORTED(shaderStorageImageArrayDynamicIndexing)
	FEATURE_IS_SUPPORTED(shaderClipDistance)
	FEATURE_IS_SUPPORTED(shaderCullDistance)
	FEATURE_IS_SUPPORTED(shaderFloat64)
	FEATURE_IS_SUPPORTED(shaderInt64)
	FEATURE_IS_SUPPORTED(shaderInt16)
	FEATURE_IS_SUPPORTED(shaderResourceResidency)
	FEATURE_IS_SUPPORTED(shaderResourceMinLod)
	FEATURE_IS_SUPPORTED(sparseBinding)
	FEATURE_IS_SUPPORTED(sparseResidencyBuffer)
	FEATURE_IS_SUPPORTED(sparseResidencyImage2D)
	FEATURE_IS_SUPPORTED(sparseResidencyImage3D)
	FEATURE_IS_SUPPORTED(sparseResidency2Samples)
	FEATURE_IS_SUPPORTED(sparseResidency4Samples)
	FEATURE_IS_SUPPORTED(sparseResidency8Samples)
	FEATURE_IS_SUPPORTED(sparseResidency16Samples)
	FEATURE_IS_SUPPORTED(sparseResidencyAliased)
	FEATURE_IS_SUPPORTED(variableMultisampleRate)
	FEATURE_IS_SUPPORTED(inheritedQueries)

	device_info.pEnabledFeatures = &_features;

	VkDevice device{VK_NULL_HANDLE};
	VK_CHECK(vkCreateDevice(gpu, &device_info, nullptr, &device), "Could not create Vulkan device");

	for (const auto &[index, family] : allocated_families)
	{
		uint32_t queue_index{0};
		for (auto queue : family.queues)
		{
			Queue::QueueInfo queue_info;
			queue_info._family_index = family.family_index;
			queue_info._index        = queue_index++;
			vkGetDeviceQueue(device, queue_info._family_index, queue_info._index, &queue_info._queue);
			queue->_queue_info = queue_info;
		}
	}

	return {device, requested_queues};
}
}        // namespace vulkan
}        // namespace components