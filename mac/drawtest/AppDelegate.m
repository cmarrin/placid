//
//  AppDelegate.m
//  drawtest
//
//  Created by Chris Marrin on 12/11/18.
//  Copyright © 2018 MarrinTech. All rights reserved.
//

#import "AppDelegate.h"
#import <sys/time.h>

@interface AppDelegate ()

@end

@implementation AppDelegate

void init(void);
void inputChar(uint8_t);

- (void)handleKeyEvent:(NSNotification *) notification
{
    NSData* data = notification.userInfo[NSFileHandleNotificationDataItem];
    NSString* s = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    
    for (const char* str = [s UTF8String]; *str; ++str) {
        inputChar((uint8_t) *str);
    }
    [[NSFileHandle fileHandleWithStandardInput] readInBackgroundAndNotify];
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    init();
    
    [[NSFileHandle fileHandleWithStandardInput] readInBackgroundAndNotify];
    [[NSNotificationCenter defaultCenter] addObserver:self selector: @selector(handleKeyEvent:)
           name: NSFileHandleReadCompletionNotification
         object: nil];
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
    // Insert code here to tear down your application
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

@end
