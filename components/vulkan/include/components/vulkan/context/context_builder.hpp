/* Copyright (c) 2022, Arm Limited and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <functional>
#include <string_view>
#include <vector>

#include "context.hpp"
#include "queue_manager.hpp"

#include "components/vulkan/common/pnext_chain.hpp"

namespace components
{
namespace vulkan
{
class ContextBuilder;

#define APPLY_BUILDER_FUNC()               \
	inline Self &Apply(BuilderFunc &&func) \
	{                                      \
		func(*this);                       \
		return *this;                      \
	}

#define DONE_FUNC()               \
	inline ContextBuilder &Done() \
	{                             \
		return _parent;           \
	}

#define BUILDER(name)                                \
	friend class ContextBuilder;                     \
                                                     \
	using Self        = name;                        \
	using BuilderFunc = std::function<void(Self &)>; \
                                                     \
  private:                                           \
	ContextBuilder &_parent;                         \
                                                     \
  public:                                            \
	name(ContextBuilder &parent) :                   \
	    _parent{parent}                              \
	{}                                               \
                                                     \
	~name() = default;                               \
                                                     \
	APPLY_BUILDER_FUNC()                             \
	DONE_FUNC()

/**
 * @brief Allows a Sample to configure a VkInstance
 *
 */
class InstanceBuilder final
{
	BUILDER(InstanceBuilder)

  public:
	using ApplicationInfoFunc = std::function<VkApplicationInfo()>;
	inline Self &application_info(ApplicationInfoFunc &&func)
	{
		_application_info = func();
		return *this;
	}

	/**
	 * @brief A function used to indicate if a feature was enabled or disabled
	 * 
	 */
	using Callback = std::function<void(bool)>;

	Self &optional_extension(std::string_view extension_name, Callback callback = nullptr);
	Self &optional_extension(std::string_view layer_name, std::string_view extension_name, Callback callback = nullptr);

	Self &required_extension(std::string_view extension_name, Callback callback = nullptr);
	Self &required_extension(std::string_view layer_name, std::string_view extension_name, Callback callback = nullptr);

	Self &optional_layer(std::string_view layer, Callback callback = nullptr);

	Self &required_layer(std::string_view layer, Callback callback = nullptr);

	template <typename Type>
	using ExtensionFunc = std::function<void(Type &extension)>;

	template <typename Type>
	inline Self &configure_extension_struct(ExtensionFunc<Type> &&func)
	{
		_chain.append(func);
		return *this;
	}

  private:
	VkInstance Build();

	std::vector<std::string_view> _available_layers;

	bool layer_available(std::string_view name);

	std::unordered_map<std::string_view, std::vector<std::string_view>> _available_extension_layers;

	bool extension_available(std::string_view layer, std::string_view name);

	VkApplicationInfo             _application_info;
	std::vector<std::string_view> _optional_extensions;
	std::vector<std::string_view> _required_extensions;
	std::vector<std::string_view> _optional_layers;
	std::vector<std::string_view> _required_layers;
};

/**
 * @brief Allows a Sample to filter physical devices for specific traits and compatible devices
 *
 * 		  Filter functions return a score for a given device, allowing devices to be ranked for compatability with a Samples requirements
 *
 * 		  Returns a VkPhysicalDevice handle for the device which matches the most
 */
class PhysicalDeviceBuilder final
{
	BUILDER(PhysicalDeviceBuilder)

  public:
	Self &AcceptType(VkPhysicalDeviceType type);

	using FeatureFilter = std::function<uint32_t(const VkPhysicalDeviceFeatures &)>;
	Self &ApplyFeatureFilter(FeatureFilter &&filter);

	using Feature2Filter = std::function<uint32_t(const VkPhysicalDeviceFeatures2 &)>;
	Self &ApplyFeature2Filter(FeatureFilter &&filter);

	using LimitFilter = std::function<uint32_t(const VkPhysicalDeviceLimits &)>;
	Self &ApplyLimitFilter(LimitFilter &&filter);

	using CustomFilter = std::function<uint32_t(VkPhysicalDevice)>;
	Self &ApplyCustomFilter(CustomFilter &&filter);

  private:
	VkPhysicalDevice SelectPhysicalDevice(VkInstance instance);

	ContextBuilder &_parent;

	std::vector<VkPhysicalDeviceType> _acceptable_device_types;
	std::vector<FeatureFilter>        _feature_filters;
	std::vector<Feature2Filter>       _feature2_filters;
	std::vector<LimitFilter>          _limit_filters;
	std::vector<CustomFilter>         _custom_filters;
};

/**
 * @brief Allows a Sample to configure the created device including Extensions and Features
 */
class DeviceBuilder final
{
	BUILDER(DeviceBuilder)

  public:
  private:
	inline VkDevice build(VkPhysicalDevice /* gpu */)
	{
		return VK_NULL_HANDLE;
	}
};

/**
 * @brief Allows a Sample to define its queue usage upfront
 *
 * 		  Creates a QueueManger which allows these queues to be used in practice
 */
class QueueBuilder final
{
	BUILDER(QueueBuilder)

  public:
  private:
	inline QueueManager build(VkDevice /* device */)
	{
		return {};
	}
};

/**
 * @brief A builder to construct a Context Object
 *
 * 		  Allows a user to configure each part of the context using nested builders
 * 		  A nested builder can be completed by calling `ContextBuilder& Done()`
 *		  Calling `Context Build()` orchestrates the build process of the context using the configured builders
 */
class ContextBuilder final
{
	using Self        = ContextBuilder;
	using BuilderFunc = std::function<void(Self &)>;

  public:
	ContextBuilder()  = default;
	~ContextBuilder() = default;

	APPLY_BUILDER_FUNC()

	inline InstanceBuilder &ConfigureInstance()
	{
		return _instance_builder;
	}

	inline PhysicalDeviceBuilder &SelectGPU()
	{
		return _physical_device_selector;
	}

	inline DeviceBuilder &ConfigureDevice()
	{
		return _device_builder;
	}

	inline QueueBuilder &ConfigureQueues()
	{
		return _queue_builder;
	}

	/**
	 * @brief Orchestrates the building of a context by calling the nested build functions for each component
	 *
	 * @return Context The built context
	 */
	Context Build();

  private:
	InstanceBuilder       _instance_builder;
	PhysicalDeviceBuilder _physical_device_selector;
	DeviceBuilder         _device_builder;
	QueueBuilder          _queue_builder;
};

namespace funcs
{
void apply_debug_configuration(ContextBuilder &builder);
void apply_default_configuration(ContextBuilder &builder);
}        // namespace funcs

}        // namespace vulkan
}        // namespace components

#undef APPLY_BUILDER_FUNC
#undef DONE_FUNC
#undef BUILDER