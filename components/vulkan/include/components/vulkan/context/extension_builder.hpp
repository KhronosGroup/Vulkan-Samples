#pragma once

#include <functional>
#include <set>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <volk.h>

#include <components/common/error.hpp>

#include "macros.hpp"

namespace components
{
namespace vulkan
{

template <typename Type, typename ParentType>
class ExtensionBuilder
{
  public:
	using Self            = ExtensionBuilder;
	using LayerName       = std::string_view;
	using ExtensionName   = std::string_view;
	using EnabledCallback = std::function<void(Type &)>;

  private:
	// must be declared before inline functions
	struct LabelledCallback
	{
		std::string_view             name;
		std::vector<EnabledCallback> callbacks;

		inline void append(const EnabledCallback &callback)
		{
			if (callback)
			{
				callbacks.push_back(std::move(callback));
			}
		}

		inline void callback(Type &info) const
		{
			for (auto &callback : callbacks)
			{
				if (callback)
				{
					callback(info);
				}
			}
		}
	};

	using Layer     = LabelledCallback;
	using Extension = LabelledCallback;

  public:
	Self &optional_extension(std::string_view layer_name, std::string_view extension_name, const EnabledCallback &callback = nullptr);
	Self &required_extension(std::string_view layer_name, std::string_view extension_name, const EnabledCallback &callback = nullptr);
	Self &optional_layer(std::string_view layer_name, const EnabledCallback &callback = nullptr);
	Self &required_layer(std::string_view layer_name, const EnabledCallback &callback = nullptr);
	Self &disable_extension(std::string_view extension_name);

	inline ParentType &done()
	{
		auto *me = reinterpret_cast<ParentType *>(this);
		assert(me && "reinterpret_cast failed");
		return *me;
	}

  protected:
	// methods implemented for a specific builder
	template <typename... Args>
	std::vector<VkExtensionProperties> get_available_extensions_impl(std::string_view layer_name, Args... args) const;
	template <typename... Args>
	std::vector<VkLayerProperties> get_available_layers_impl(Args... args) const;

	// intermediaries used to prune the available extension and layers list
	template <typename... Args>
	std::vector<VkExtensionProperties> get_available_extensions(std::string_view layer_name, Args... args) const
	{
		auto extensions = get_available_extensions_impl(layer_name, args...);

		// remove all disabled extensions
		extensions.erase(
		    std::remove_if(
		        extensions.begin(),
		        extensions.end(),
		        [this](const VkExtensionProperties &extension) {
			        return std::find(_disabled_extensions.begin(), _disabled_extensions.end(), extension.extensionName) != _disabled_extensions.end();
		        }),
		    extensions.end());

		return extensions;
	}

	template <typename... Args>
	std::vector<VkLayerProperties> get_available_layers(Args... args) const
	{
		return get_available_layers_impl(args...);
	}

	template <typename... Args>
	std::vector<ExtensionName> collect_enabled_extensions(Type &info, Args... args) const;
	template <typename... Args>
	std::vector<LayerName> collect_enabled_layers(Type &info, Args... args) const;

  private:
	std::vector<std::string_view>            _disabled_extensions;
	std::unordered_map<LayerName, Extension> _optional_extensions;
	std::unordered_map<LayerName, Extension> _required_extensions;
	std::vector<Layer>                       _optional_layers;
	std::vector<Layer>                       _required_layers;
};

template <typename Type, typename ParentType>
ExtensionBuilder<Type, ParentType> &ExtensionBuilder<Type, ParentType>::optional_extension(std::string_view layer_name, std::string_view extension_name, const EnabledCallback &callback)
{
	// find and append - create new if not found
	auto it = _optional_extensions.find(layer_name);
	if (it != _optional_extensions.end())
	{
		it->second.append(callback);
		return *this;
	}
	_optional_extensions.emplace(layer_name, Extension{extension_name, std::vector<EnabledCallback>{callback}});
	return *this;
}

template <typename Type, typename ParentType>
ExtensionBuilder<Type, ParentType> &ExtensionBuilder<Type, ParentType>::required_extension(std::string_view layer_name, std::string_view extension_name, const EnabledCallback &callback)
{
	// find and append - create new if not found
	auto it = _required_extensions.find(layer_name);
	if (it != _required_extensions.end())
	{
		it->second.append(callback);
		return *this;
	}
	_required_extensions.emplace(layer_name, Extension{extension_name, std::vector<EnabledCallback>{callback}});
	return *this;
}

template <typename Type, typename ParentType>
ExtensionBuilder<Type, ParentType> &ExtensionBuilder<Type, ParentType>::optional_layer(std::string_view layer_name, const EnabledCallback &callback)
{
	// find and append - create new if not found

	for (auto &layer : _optional_layers)
	{
		if (layer.name == layer_name)
		{
			layer.append(callback);
			return *this;
		}
	}

	_optional_layers.push_back({layer_name, std::vector<EnabledCallback>{callback}});
	return *this;
}

template <typename Type, typename ParentType>
ExtensionBuilder<Type, ParentType> &ExtensionBuilder<Type, ParentType>::required_layer(std::string_view layer_name, const EnabledCallback &callback)
{
	// find and append - create new if not found
	for (auto &layer : _required_layers)
	{
		if (layer.name == layer_name)
		{
			layer.append(callback);
			return *this;
		}
	}
	_required_layers.push_back({layer_name, std::vector<EnabledCallback>{callback}});
	return *this;
}

template <typename Type, typename ParentType>
ExtensionBuilder<Type, ParentType> &ExtensionBuilder<Type, ParentType>::disable_extension(std::string_view extension_name)
{
	_disabled_extensions.push_back(extension_name);
	return *this;
}

template <typename Type, typename ParentType>
template <typename... Args>
std::vector<std::string_view> ExtensionBuilder<Type, ParentType>::collect_enabled_extensions(Type &info, Args... args) const
{
	std::set<std::string_view> enabled_extensions;

	for (const auto &[layer_name, optional_extension] : _optional_extensions)
	{
		std::vector<VkExtensionProperties> available_layer_extensions = get_available_extensions(layer_name, args...);

		// try enable all extensions - we don't care if they fail as they are optional

		for (VkExtensionProperties &available_extension : available_layer_extensions)
		{
			if (std::string_view{available_extension.extensionName} == optional_extension.name)
			{
				// we can enable the extension
				enabled_extensions.emplace(optional_extension.name);
				// allow the enabled extension to configure the builder more
				optional_extension.callback(info);
			}
		}
	}

	std::set<std::string_view> missing_required_extensions;

	for (const auto &[layer_name, required_extension] : _required_extensions)
	{
		std::vector<VkExtensionProperties> available_layer_extensions = get_available_extensions(layer_name, args...);

		// try enable all extensions - we care if they fail as they are required
		bool found = false;
		for (VkExtensionProperties &available_extension : available_layer_extensions)
		{
			if (std::string_view{available_extension.extensionName} == required_extension.name)
			{
				// we can enable the extension
				enabled_extensions.emplace(required_extension.name);
				// allow the enabled extension to configure the builder more
				required_extension.callback(info);
				found = true;
			}
		}

		if (!found)
		{
			missing_required_extensions.emplace(required_extension.name);
		}
	}

	if (missing_required_extensions.size() > 0)
	{
		std::stringstream ss;
		for (auto &missing_extension : missing_required_extensions)
		{
			ss << "Missing required extension: " << missing_extension << std::endl;
		}
		ss << "Missing required extensions" << std::endl;
		throw std::runtime_error{ss.str()};
	}

	return {enabled_extensions.begin(), enabled_extensions.end()};
}

template <typename Type, typename ParentType>
template <typename... Args>
inline std::vector<std::string_view> ExtensionBuilder<Type, ParentType>::collect_enabled_layers(Type &info, Args... args) const
{
	std::set<std::string_view> enabled_layers;

	std::vector<VkLayerProperties> supported_instance_layers = get_available_layers(args...);

	// try enable the layer - we don't care if it fails as it is optional
	for (auto &optional_layer : _optional_layers)
	{
		for (auto &supported_layer : supported_instance_layers)
		{
			if (std::string_view{supported_layer.layerName} == optional_layer.name)
			{
				enabled_layers.emplace(optional_layer.name);
				optional_layer.callback(info);
			}
		}
	}

	std::set<std::string_view> missing_required_layers;

	// try enable the layer - we care if it fails as it is required
	for (auto &required_layer : _required_layers)
	{
		bool found = false;
		for (auto &supported_layer : supported_instance_layers)
		{
			if (std::string_view{supported_layer.layerName} == required_layer.name)
			{
				enabled_layers.emplace(required_layer.name);
				required_layer.callback(info);
				found = true;
			}
		}

		if (!found)
		{
			missing_required_layers.emplace(required_layer.name);
		}
	}

	if (missing_required_layers.size() > 0)
	{
		std::stringstream ss;
		for (auto &missing_layer : missing_required_layers)
		{
			ss << "Missing required layer: " << missing_layer << std::endl;
		}
		ss << "Missing required layers" << std::endl;
		throw std::runtime_error{ss.str()};
	}

	return {enabled_layers.begin(), enabled_layers.end()};
}

template <typename Type, typename ParentType>
template <typename... Args>
inline std::vector<VkExtensionProperties> ExtensionBuilder<Type, ParentType>::get_available_extensions_impl(std::string_view layer_name, Args... args) const
{
	// we only have specific scenarios for this builder - throw for any misuse
	NOT_IMPLEMENTED();
}

template <typename Type, typename ParentType>
template <typename... Args>
inline std::vector<VkLayerProperties> ExtensionBuilder<Type, ParentType>::get_available_layers_impl(Args... args) const
{
	// we only have specific scenarios for this builder - throw for any misuse
	NOT_IMPLEMENTED();
}

class InstanceBuilder;
using InstanceExtensionBuilder = ExtensionBuilder<VkInstanceCreateInfo, InstanceBuilder>;

template <>
template <>
inline std::vector<VkExtensionProperties> InstanceExtensionBuilder::get_available_extensions_impl(std::string_view layer_name) const
{
	uint32_t count;
	VK_CHECK(vkEnumerateInstanceExtensionProperties(layer_name.data(), &count, nullptr), "failed to get extensions");
	std::vector<VkExtensionProperties> extensions{count};
	VK_CHECK(vkEnumerateInstanceExtensionProperties(layer_name.data(), &count, extensions.data()), "failed to get extensions");
	return extensions;
}

template <>
template <>
inline std::vector<VkLayerProperties> InstanceExtensionBuilder::get_available_layers_impl() const
{
	uint32_t count;
	VK_CHECK(vkEnumerateInstanceLayerProperties(&count, nullptr), "failed to enumerate layers");
	std::vector<VkLayerProperties> layers(count);
	VK_CHECK(vkEnumerateInstanceLayerProperties(&count, layers.data()), "failed to enumerate layers");
	return layers;
}

class DeviceBuilder;
using DeviceExtensionBuilder = ExtensionBuilder<VkDeviceCreateInfo, DeviceBuilder>;

template <>
template <>
inline std::vector<VkExtensionProperties> DeviceExtensionBuilder::get_available_extensions_impl(std::string_view layer_name, VkPhysicalDevice gpu) const
{
	uint32_t count;
	VK_CHECK(vkEnumerateDeviceExtensionProperties(gpu, layer_name.data(), &count, nullptr), "failed to get extensions");
	std::vector<VkExtensionProperties> extensions{count};
	VK_CHECK(vkEnumerateDeviceExtensionProperties(gpu, layer_name.data(), &count, extensions.data()), "failed to get extensions");
	return extensions;
}

template <>
template <>
inline std::vector<VkLayerProperties> DeviceExtensionBuilder::get_available_layers_impl(VkPhysicalDevice gpu) const
{
	uint32_t count;
	VK_CHECK(vkEnumerateDeviceLayerProperties(gpu, &count, nullptr), "failed to enumerate layers");
	std::vector<VkLayerProperties> layers(count);
	VK_CHECK(vkEnumerateDeviceLayerProperties(gpu, &count, layers.data()), "failed to enumerate layers");
	return layers;
}

}        // namespace vulkan
}        // namespace components