/* Copyright (c) 2019-2024, Arm Limited and Contributors
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

#include "core/util/logging.hpp"
#include "platform/platform.h"
#include "../plugins/plugins.h"

#include <core/platform/entrypoint.hpp>
#include <filesystem/filesystem.hpp>

#include <TargetConditionals.h>
#import <Foundation/Foundation.h>
#import "AppDelegate.h"

#include "platform/ios/ios_platform.h"
#include "ios/context.hpp"

#undef CUSTOM_MAIN
#    define CUSTOM_MAIN(context_name)                           \
        int platform_main(const vkb::PlatformContext &);        \
        int main(int argc, char *argv[])                        \
        {                                                       \
            NSString * appDelegateClassName;                    \
            @autoreleasepool {                                  \
                appDelegateClassName = NSStringFromClass([AppDelegate class]); \
                return UIApplicationMain(argc, argv, nil, appDelegateClassName); \
            }                                                   \
        }                                                       \
        int platform_main(const vkb::PlatformContext &context_name)

std::unique_ptr<vkb::IosPlatform> platform;

CUSTOM_MAIN(context)
{
    vkb::filesystem::init_with_context(context);

    platform = std::make_unique<vkb::IosPlatform>(context);

    auto code = platform->initialize(plugins::get_all());
    ((vkb::IosPlatformContext*)&context)->userPlatform = platform.get();

    return (int)code;
}

