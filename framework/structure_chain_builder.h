/* Copyright (c) 2026, NVIDIA CORPORATION. All rights reserved.
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

#include <any>
#include <vulkan/vulkan.hpp>

namespace vkb
{
template <vkb::BindingType bindingType, typename AnchorStructType>
class StructureChainBuilder
{
  public:
	StructureChainBuilder();

	template <typename T>
	T &add_chain_data(T const &data_to_add = {});        // Adds data to the structure chain builder that is not part of the structure chain itself, but is used by the
	                                                     // structures in the chain (e.g. pointed to by a member of a struct in the structure chain)

	template <typename StructType>
	StructType &add_struct(StructType const &struct_to_add = {});

	template <typename StructType>
	StructType const *get_struct(size_t skip = 0) const;

	void set_anchor_struct(AnchorStructType const &anchor_struct);

  private:
	template <typename StructType>
	StructType &add_struct_impl(StructType const &struct_to_add);
	template <typename StructType>
	StructType const *get_struct_impl(size_t skip) const;
	void              set_anchor_struct_impl(AnchorStructType const &anchor_struct);

  private:
	std::vector<std::unique_ptr<std::any>> structure_chain;
	std::vector<std::unique_ptr<std::any>> chain_data;        // data used with the structure chain, stored separately to ensure correct destruction order (the structs
	                                                          // in the structure chain may contain pointers to data owned by the chain_data)
};

template <typename AnchorStructType>
using StructureChainBuilderC = StructureChainBuilder<vkb::BindingType::C, AnchorStructType>;
template <typename AnchorStructType>
using StructureChainBuilderCpp = StructureChainBuilder<vkb::BindingType::Cpp, AnchorStructType>;

template <vkb::BindingType bindingType, typename AnchorStructType>
template <typename T>
T &StructureChainBuilder<bindingType, AnchorStructType>::add_chain_data(T const &data_to_add)
{
	chain_data.push_back(std::make_unique<std::any>(std::make_any<T>(data_to_add)));
	return *std::any_cast<T>(chain_data.back().get());
}

template <vkb::BindingType bindingType, typename AnchorStructType>
inline StructureChainBuilder<bindingType, AnchorStructType>::StructureChainBuilder()
{
	static_assert((offsetof(AnchorStructType, sType) == 0) && (offsetof(AnchorStructType, pNext) == sizeof(void *)));
	structure_chain.push_back(std::make_unique<std::any>(std::make_any<AnchorStructType>()));
}

template <vkb::BindingType bindingType, typename AnchorStructType>
template <typename StructType>
inline StructType &StructureChainBuilder<bindingType, AnchorStructType>::add_struct(StructType const &struct_to_add)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return add_struct_impl(struct_to_add);
	}
	else
	{
		return reinterpret_cast<StructureChainBuilder<vkb::BindingType::Cpp, typename vk::CppType<AnchorStructType>::Type> *>(this)->add_struct(
		    reinterpret_cast<typename vk::CppType<StructType>::Type const &>(struct_to_add));
	}
}

template <vkb::BindingType bindingType, typename AnchorStructType>
template <typename StructType>
inline StructType &StructureChainBuilder<bindingType, AnchorStructType>::add_struct_impl(StructType const &struct_to_add)
{
#if !defined(NDEBUG)
	auto it = std::ranges::find_if(structure_chain, [](auto const &chain_element) {
		return std::any_cast<StructType>(chain_element.get()) != nullptr;
	});
	assert(it == structure_chain.end() || StructType::allowDuplicate);        // If the struct type already exists in the structure chain, it must allow duplicates
#endif

	structure_chain.push_back(std::make_unique<std::any>(std::make_any<StructType>(struct_to_add)));
	std::any_cast<StructType>(structure_chain.back().get())->pNext        = std::any_cast<AnchorStructType>(structure_chain.front().get())->pNext;
	std::any_cast<AnchorStructType>(structure_chain.front().get())->pNext = std::any_cast<StructType>(structure_chain.back().get());
	return *std::any_cast<StructType>(structure_chain.back().get());
}

template <vkb::BindingType bindingType, typename AnchorStructType>
template <typename StructType>
inline StructType const *StructureChainBuilder<bindingType, AnchorStructType>::get_struct(size_t skip) const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return get_struct_impl<StructType>(skip);
	}
	else
	{
		return reinterpret_cast<StructType const *>(get_struct_impl<vk::CppType<StructType>::Type>(skip));
	}
}

template <vkb::BindingType bindingType, typename AnchorStructType>
template <typename StructType>
inline StructType const *StructureChainBuilder<bindingType, AnchorStructType>::get_struct_impl(size_t skip) const
{
	assert(!skip || StructType::allowDuplicate);        // If skip is non-zero, the struct type must allow duplicates in the structure chain

	auto it = std::ranges::find_if(structure_chain, [](auto const &chain_element) {
		return std::any_cast<StructType>(chain_element.get()) != nullptr;
	});
	for (size_t i = 0; (i < skip) && (it != structure_chain.end()); ++i)
	{
		it = std::find_if(std::next(it), structure_chain.end(), [](auto const &chain_element) {
			return std::any_cast<StructType>(chain_element.get()) != nullptr;
		});
	}
	return (it != structure_chain.end()) ? std::any_cast<StructType>(it->get()) : nullptr;
}

template <vkb::BindingType bindingType, typename AnchorStructType>
inline void StructureChainBuilder<bindingType, AnchorStructType>::set_anchor_struct(AnchorStructType const &anchor_struct)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		set_anchor_struct_impl(anchor_struct);
	}
	else
	{
		return reinterpret_cast<StructureChainBuilder<vkb::BindingType::Cpp, typename vk::CppType<AnchorStructType>::Type> *>(this)->add_anchor_struct(
		    reinterpret_cast<typename vk::CppType<AnchorStructType>::Type const &>(anchor_struct));
	}
}

template <vkb::BindingType bindingType, typename AnchorStructType>
inline void StructureChainBuilder<bindingType, AnchorStructType>::set_anchor_struct_impl(AnchorStructType const &anchor_struct)
{
	void const *pNext                                                     = std::any_cast<AnchorStructType>(structure_chain.front().get())->pNext;
	*std::any_cast<AnchorStructType>(structure_chain.front().get())       = anchor_struct;
	std::any_cast<AnchorStructType>(structure_chain.front().get())->pNext = pNext;
}

}        // namespace vkb
