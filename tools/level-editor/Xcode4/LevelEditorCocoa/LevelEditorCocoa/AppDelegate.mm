//
//  AppDelegate.m
//  LevelEditorCocoa
//
//  Created by Daniel Monteiro on 8/4/24.
//  Copyright (c) 2024 Daniel Monteiro. All rights reserved.
//

#import "AppDelegate.h"
#include "level-editor.h"
@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    // Insert code here to initialize your application
}

- (IBAction)loadMap:(id)sender {

    NSOpenPanel *panel = [NSOpenPanel openPanel];
    [panel setCanChooseFiles:YES];
    [panel setCanChooseDirectories:NO];
    [panel setAllowsMultipleSelection:NO];
    
    NSInteger clicked = [panel runModal];
    
    if (clicked == NSFileHandlingPanelOKButton) {
        for (NSURL *url in [panel URLs]) {
            NSLog(@"Load map! %@", url.path);
            struct Map map = loadMap( url.path.UTF8String );
            [gridView updateMap: &map];
            
        }
    }
}
@end
