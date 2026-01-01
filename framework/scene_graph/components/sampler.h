/* Copyright (c) 2018-2025, Arm Limited and Contributors
 * Copyright (c) 2025, NVIDIA CORPORATION. All rights reserved.
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

#include "core/hpp_sampler.h"
#include "core/sampler.h"
#include "scene_graph/component.h"

namespace vkb
{
namespace scene_graph
{
namespace components
{
template <vkb::BindingType bindingType>
class Sampler : public vkb::sg::Component
{
  public:
	using CoreSamplerType = typename std::conditional<bindingType == BindingType::Cpp, vkb::core::HPPSampler, vkb::core::Sampler>::type;

  public:
	Sampler(std::string const &name, CoreSamplerType &&core_sampler);

	Sampler(Sampler &&other) = default;
	virtual ~Sampler()       = default;

	CoreSamplerType const &get_core_sampler() const;

	virtual std::type_index get_type() override;

  private:
	vkb::core::HPPSampler core_sampler;
};

using SamplerC   = Sampler<vkb::BindingType::C>;
using SamplerCpp = Sampler<vkb::BindingType::Cpp>;

// Member function definitions

template <>
inline Sampler<vkb::BindingType::Cpp>::Sampler(std::string const &name, vkb::core::HPPSampler &&core_sampler_) :
    Component{name},
    core_sampler{std::move(core_sampler_)}
{
}

template <>
inline Sampler<vkb::BindingType::C>::Sampler(std::string const &name, vkb::core::Sampler &&core_sampler_) :
    Component{name},
    core_sampler{std::move(reinterpret_cast<vkb::core::HPPSampler &&>(core_sampler_))}
{
}

template <vkb::BindingType bindingType>
inline typename Sampler<bindingType>::CoreSamplerType const &Sampler<bindingType>::get_core_sampler() const
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return core_sampler;
	}
	else
	{
		return reinterpret_cast<vkb::core::Sampler const &>(core_sampler);
	}
}

template <vkb::BindingType bindingType>
inline std::type_index Sampler<bindingType>::get_type()
{
	return typeid(Sampler<bindingType>);
}
}        // namespace components
}        // namespace scene_graph
}        // namespace vkb
