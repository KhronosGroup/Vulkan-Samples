/* Copyright (c) 2018-2026, Arm Limited and Contributors
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

#include "common/vk_common.h"
#include <vulkan/vulkan.hpp>

namespace vkb
{
namespace common
{
/**
 * @brief Vulkan exception class
 */
template <vkb::BindingType bindingType>
class VulkanException : public std::runtime_error
{
  public:
	using ResultType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::Result, VkResult>::type;

  public:
	/**
	 * @brief Vulkan exception constructor
	 */
	VulkanException(ResultType result, std::string const &msg = "Vulkan error");

	/**
	 * @brief Returns the Vulkan error code as string
	 * @return String message of exception
	 */
	char const *what() const noexcept override;

  private:
	std::string error_message;
	vk::Result  result;
};

using VulkanExceptionC   = VulkanException<vkb::BindingType::C>;
using VulkanExceptionCpp = VulkanException<vkb::BindingType::Cpp>;

template <vkb::BindingType bindingType>
inline VulkanException<bindingType>::VulkanException(VulkanException<bindingType>::ResultType result_,
                                                     std::string const                       &msg) :
    result{static_cast<vk::Result>(result_)}, std::runtime_error{msg}
{
	error_message = std::string(std::runtime_error::what()) + std::string{" : "} + vk::to_string(result);
}

template <vkb::BindingType bindingType>
inline char const *VulkanException<bindingType>::what() const noexcept
{
	return error_message.c_str();
}

}        // namespace common
}        // namespace vkb
