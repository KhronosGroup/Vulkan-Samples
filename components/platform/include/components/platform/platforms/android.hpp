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

#include <components/common/error.hpp>

VKBP_DISABLE_WARNINGS()
#include <android_native_app_glue.h>
VKBP_ENABLE_WARNINGS()

#include "components/platform/platform.hpp"

namespace components
{
class AndroidContext final : public PlatformContext
{
  public:
	AndroidContext(android_app *app, const std::vector<std::string> &arguments) :
	    android_app{app},
	    m_arguments{arguments}
	{}

	virtual ~AndroidContext() = default;

	virtual std::vector<std::string_view> arguments() const override
	{
		return {m_arguments.begin(), m_arguments.end()};
	}

	android_app *android_app;

  private:
	std::vector<std::string> m_arguments;
};
}        // namespace components

// TODO: get arguments from bundle
#define CUSTOM_MAIN(context_name)                            \
	void android_main(android_app *android_app)              \
	{                                                        \
		components::AndroidContext context{android_app, {}}; \
                                                             \
		platform_main(&context);                             \
	}                                                        \
                                                             \
	int platform_main(const components::PlatformContext *context_name)