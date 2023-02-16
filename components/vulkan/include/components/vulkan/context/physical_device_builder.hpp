#pragma once

#include "components/vulkan/context/instance_builder.hpp"

namespace components
{
namespace vulkan
{
/**
 * @brief Allows a Sample to filter physical devices for specific traits and compatible devices
 *
 * 		  Score functions return a score for a given device, allowing devices to be ranked for compatability with a Samples requirements
 *
 * 		  Returns a VkPhysicalDevice handle for the device which matches the most
 */
class PhysicalDeviceBuilder final
{
	CVC_CREATE_BUILDER(PhysicalDeviceBuilder)

  public:
	// required features score
	static const uint32_t RequiredScore = 1000U;
	// preferred features score
	static const uint32_t PreferredScore = 1000U;
	// nice to haves score
	static const uint32_t GeneralScore = 10U;

	// score a specific device on its worthiness
	using ScoringFunc = std::function<uint32_t(VkPhysicalDevice gpu, const PhysicalDeviceInfo &)>;
	inline Self &score_device(ScoringFunc &&func)
	{
		_scoring_func = std::move(func);
		return *this;
	}

  private:
	struct Output
	{
		VkPhysicalDevice   gpu;
		PhysicalDeviceInfo info;
	};

	Output select_physical_device(VkInstance instance);

	PhysicalDeviceInfo get_device_info(VkPhysicalDevice gpu) const;

	ScoringFunc _scoring_func;
};

namespace scores
{
// score a device using multiple function
PhysicalDeviceBuilder::ScoringFunc combined_scoring(std::vector<PhysicalDeviceBuilder::ScoringFunc> &&funcs);

// require a device of a specific type
PhysicalDeviceBuilder::ScoringFunc require_device_type(VkPhysicalDeviceType type);

// score based on a preference order of device types
PhysicalDeviceBuilder::ScoringFunc device_preference(const std::vector<VkPhysicalDeviceType> &preference_order);

// requires that a device has the correct amount of queues of a specific type
PhysicalDeviceBuilder::ScoringFunc has_queue(VkQueueFlagBits queue_type, uint32_t required_queue_count = 1);

// require a device supports presenting to a specific surface
PhysicalDeviceBuilder::ScoringFunc can_present(VkSurfaceKHR surface);
}        // namespace funcs
}        // namespace vulkan
}        // namespace components
