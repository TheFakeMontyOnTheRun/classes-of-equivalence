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

- (IBAction)drawLine:(id)sender {
    onDrawLineClicked();
}

- (IBAction)fillRect:(id)sender {
    onDrawSquareClicked();
}

- (IBAction)saveMap:(id)sender {

    NSSavePanel *panel = [NSSavePanel savePanel];
    /*
    [panel setCanChooseFiles:YES];
    [panel setCanChooseDirectories:NO];
    [panel setAllowsMultipleSelection:NO];
    */
    NSInteger clicked = [panel runModal];
    
    if (clicked == NSFileHandlingPanelOKButton) {
        [gridView saveMap: [panel URL].path];
    }

}

- (IBAction)loadMap:(id)sender {

    NSOpenPanel *panel = [NSOpenPanel openPanel];
    [panel setCanChooseFiles:YES];
    [panel setCanChooseDirectories:NO];
    [panel setAllowsMultipleSelection:NO];
    
    NSInteger clicked = [panel runModal];
    
    if (clicked == NSFileHandlingPanelOKButton) {
        for (NSURL *url in [panel URLs]) {
            [gridView loadMap: url.path];
            
        }
    }
}
@end
