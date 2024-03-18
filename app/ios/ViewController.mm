/* Copyright (c) 2024, Holochip Inc.
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

#include "ViewController.h"
#include "ios/context.hpp"
#include "core/platform/entrypoint.hpp"
#include "platform/ios/ios_platform.h"
#include "platform/ios/ios_window.h"

int platform_main(const vkb::PlatformContext &);

@interface ViewController(){
    CADisplayLink* _displayLink;
    std::unique_ptr<vkb::PlatformContext> context;
    vkb::IosPlatformContext* _context;
    vkb::ExitCode _code;
}
@property (retain, nonatomic) IBOutlet VulkanView *vulkan_view;

@end

@implementation ViewController
- (void)viewDidLoad {
    [super viewDidLoad];


    // Convert incoming args to "C" argc/argv strings
    NSArray *args = [[NSProcessInfo processInfo] arguments];
    const char** argv = (const char**) alloca(sizeof(char*) * args.count);
    for(unsigned int i = 0; i < args.count; i++) {
        NSString *s = args[i];
        argv[i] = s.UTF8String;
    }

    context = create_platform_context((int)args.count, const_cast<char**>(argv));
    _context = (vkb::IosPlatformContext*)context.get();
    _context->view = self.vulkan_view;
    _code = (vkb::ExitCode)platform_main(*context);
    uint32_t fps = 60;
   _displayLink = [CADisplayLink displayLinkWithTarget: self selector: @selector(renderLoop)];
   [_displayLink setPreferredFramesPerSecond:fps];
   [_displayLink addToRunLoop: NSRunLoop.currentRunLoop forMode: NSDefaultRunLoopMode];
}

-(void) renderLoop {
    if(!_displayLink.isPaused && [UIApplication sharedApplication].applicationState == UIApplicationStateActive) {
        _code = ((vkb::IosPlatform*)_context->userPlatform)->main_loop_frame();
    }
}

- (void)dealloc {
    [_displayLink invalidate];
    [_displayLink release];
    ((vkb::IosPlatform*)_context->userPlatform)->terminate(_code);

    [self.vulkan_view release];
    [super dealloc];
}
@end
