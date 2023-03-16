#pragma once

#include "components/vulkan/context/instance_builder.hpp"
#include "components/vulkan/context/queue.hpp"

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
		_scoring_funcs.push_back(std::move(func));
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

	std::vector<ScoringFunc> _scoring_funcs;
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
PhysicalDeviceBuilder::ScoringFunc supports_queue(const QueuePtr &queue);
}        // namespace scores
}        // namespace vulkan
}        // namespace components
