#include "components/vulkan/context/device_builder.hpp"

#include <cassert>

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

DeviceBuilder &DeviceBuilder::enable_queue(VkQueueFlagBits queue_type, uint32_t required_queue_count)
{
	_required_queue_counts[queue_type] = required_queue_count;
	return *this;
}

DeviceBuilder &DeviceBuilder::must_support_presentation(VkSurfaceKHR surface)
{
	_surface = surface;
	return *this;
}

DeviceBuilder::Output DeviceBuilder::build(VkPhysicalDevice gpu, const PhysicalDeviceInfo &info)
{
	VkDeviceCreateInfo device_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};

	std::unordered_map<VkQueueFlagBits, uint32_t> queue_tally = _required_queue_counts;

	if (queue_tally.empty())
	{
		throw std::runtime_error{"No queues requested. Device must request one or more queues."};
	}

	std::unordered_map<uint32_t, QueueFamily> allocated_families;

	const auto get_family = [&](const QueueFamilyInfo &family) {
		auto it = allocated_families.find(family.index);
		if (it == allocated_families.end())
		{
			QueueFamily queue{family.index};
			queue.total_supported_queues_count = family.properties.queueCount;
			it                                 = allocated_families.emplace(family.index, queue).first;
		}
		return it;
	};

	const auto allocate = [&](const QueueFamilyInfo &family) {
		auto it = get_family(family);

		std::vector<VkQueueFlagBits> queues_to_remove;

		for (auto &[queue_type, remaining_tally] : queue_tally)
		{
			if (remaining_tally == 0)
			{
				queues_to_remove.push_back(queue_type);
				continue;
			}

			// try and allocate as many queues as possible up to the required count for this queue type
			uint32_t allocated_count = std::min(remaining_tally, family.properties.queueCount);

			if (queue_type == VK_QUEUE_GRAPHICS_BIT)
			{
				it->second.graphics_queue_count += allocated_count;
			}
			if (queue_type == VK_QUEUE_COMPUTE_BIT)
			{
				it->second.compute_queue_count += allocated_count;
			}
			if (queue_type == VK_QUEUE_TRANSFER_BIT)
			{
				it->second.transfer_queue_count += allocated_count;
			}

			remaining_tally -= allocated_count;
		}

		// clean up any queues which have been fully allocated
		for (auto queue_type : queues_to_remove)
		{
			queue_tally.erase(queue_type);
		}
	};

	if (_surface != VK_NULL_HANDLE)
	{
		// prioritize the first queue which can present to the requested surface
		for (const auto &family : info.queue_families)
		{
			VkBool32 present_supported{false};
			VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(gpu, family.index, _surface, &present_supported), "failed to query for presentation support");

			if (present_supported)
			{
				auto it = get_family(family);

				it->second.supports_presentation = true;

				allocate(family);
				break;
			}
		}
	}

	for (const auto &family : info.queue_families)
	{
		if (queue_tally.empty())
		{
			// no longer need queues
			break;
		}

		allocate(family);
	}

	if (!queue_tally.empty())
	{
		// we didn't find enough queues
		throw std::runtime_error("Could not find enough queues to satisfy the requested requirements");
	}

	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
	queue_create_infos.reserve(allocated_families.size());

	for (const auto &[index, family] : allocated_families)
	{
		auto total_requested_queues = family.graphics_queue_count + family.compute_queue_count + family.transfer_queue_count;
		if (total_requested_queues == 0)
		{
			continue;
		}

		float queue_priority = 1.0f;

		VkDeviceQueueCreateInfo queue_create_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
		queue_create_info.queueFamilyIndex = family.family_index;
		queue_create_info.queueCount       = total_requested_queues;
		queue_create_info.pQueuePriorities = &queue_priority;

		assert(queue_create_info.queueCount <= family.total_supported_queues_count && "Requested more queues than the family supports");

		queue_create_infos.push_back(queue_create_info);
	}

	std::vector<const char *> enabled_extensions_cstr = strings::to_cstr(collect_enabled_extensions(device_info, gpu));
	device_info.enabledExtensionCount                 = static_cast<uint32_t>(enabled_extensions_cstr.size());
	device_info.ppEnabledExtensionNames               = enabled_extensions_cstr.data();

	std::vector<const char *> enabled_layers_cstr = strings::to_cstr(collect_enabled_layers(device_info, gpu));
	device_info.enabledLayerCount                 = static_cast<uint32_t>(enabled_layers_cstr.size());
	device_info.ppEnabledLayerNames               = enabled_layers_cstr.data();

	device_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
	device_info.pQueueCreateInfos    = queue_create_infos.data();

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
	return {device, QueueManager{}};
}
}        // namespace vulkan
}        // namespace components