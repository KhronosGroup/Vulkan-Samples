/* Copyright (c) 2018-2019, Arm Limited and Contributors
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

#include <memory>
#include <string>
#include <typeinfo>
#include <vector>

#include "scene_graph/component.h"
#include "scene_graph/components/sampler.h"

namespace vkb
{
namespace sg
{
class Image;
class Sampler;

class Texture : public Component
{
  public:
	Texture(const std::string &name);

	Texture(Texture &&other) = default;

	virtual ~Texture() = default;

	virtual std::type_index get_type() override;

	void set_image(Image &image);

	Image *get_image();

	void set_sampler(Sampler &sampler);

	Sampler *get_sampler();

  private:
	Image *image{nullptr};

	Sampler *sampler{nullptr};
};
}        // namespace sg
}        // namespace vkb
