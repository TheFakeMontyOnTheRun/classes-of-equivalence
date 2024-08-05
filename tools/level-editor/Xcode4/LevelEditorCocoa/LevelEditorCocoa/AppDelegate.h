//
//  AppDelegate.h
//  LevelEditorCocoa
//
//  Created by Daniel Monteiro on 8/4/24.
//  Copyright (c) 2024 Daniel Monteiro. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "EditorGridView.h"

@interface AppDelegate : NSObject <NSApplicationDelegate> {
    IBOutlet EditorGridView* gridView;
}

- (IBAction)loadMap:(id)sender;
- (IBAction)saveMap:(id)sender;
- (IBAction)drawLine:(id)sender;
- (IBAction)fillRect:(id)sender;
@property (assign) IBOutlet NSWindow *window;

@end
