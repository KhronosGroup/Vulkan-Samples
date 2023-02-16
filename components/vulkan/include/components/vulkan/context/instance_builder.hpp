#pragma once

#include <functional>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "components/vulkan/context/context.hpp"
#include "components/vulkan/context/extension_builder.hpp"

#include "components/vulkan/pnext_chain.hpp"

namespace components
{
namespace vulkan
{
class ContextBuilder;

#define CVC_APPLY_BUILDER_FUNC()           \
	inline Self &apply(BuilderFunc &&func) \
	{                                      \
		func(*this);                       \
		return *this;                      \
	}

#define CVC_DONE_FUNC()           \
	inline ContextBuilder &done() \
	{                             \
		return _parent;           \
	}

#define CVC_CREATE_BUILDER(name)                     \
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
	CVC_APPLY_BUILDER_FUNC()                         \
	CVC_DONE_FUNC()

/**
 * @brief Allows a Sample to configure a VkInstance
 *
 */
class InstanceBuilder final : public InstanceExtensionBuilder
{
	CVC_CREATE_BUILDER(InstanceBuilder)

  public:
	Self &enable_portability();

	Self &enable_debug_logger();

	Self &enable_validation_layers(const std::vector<std::string_view> &required_layers = {});

	using ApplicationInfoFunc = std::function<VkApplicationInfo()>;
	inline Self &application_info(ApplicationInfoFunc &&func)
	{
		_application_info = func();
		// application pNext must always be nullptr
		_application_info.pNext = nullptr;
		return *this;
	}

	template <typename Type>
	inline Self &configure_extension_struct(pNextChain::AppendFunc<Type> &&func)
	{
		_chain.append(std::move(func));
		return *this;
	}

	template <typename Type>
	inline bool has_extension_in_chain()
	{
		return _chain.has<Type>();
	}

  private:
	struct Output
	{
		VkInstance   instance;
		DebuggerInfo debugger_info;
	};

	Output build();

	VkApplicationInfo _application_info{VK_STRUCTURE_TYPE_APPLICATION_INFO};

	pNextChain _chain;
};
}        // namespace vulkan
}        // namespace components
