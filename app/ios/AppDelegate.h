//
//  AppDelegate.h
//  epa-phase-1-demo
//
//  Created by Steven Winston on 12/21/23.
//

#include <TargetConditionals.h>
#if TARGET_OS_IOS
#import <UIKit/UIKit.h>
#else
#include <AppKit/AppKit.h>
#endif

#if TARGET_OS_IOS
@interface AppDelegate : UIResponder <UIApplicationDelegate>

@property (strong, nonatomic) UIWindow *window;
#else
@interface AppDelegate : NSResponder <NSApplicationDelegate>

@property (strong, nonatomic) NSWindow *window;
#endif

@end

