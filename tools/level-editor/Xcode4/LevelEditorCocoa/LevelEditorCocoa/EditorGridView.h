//
//  EditorGridView.h
//  LevelEditorCocoa
//
//  Created by Daniel Monteiro on 8/4/24.
//  Copyright (c) 2024 Daniel Monteiro. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include "level-editor.h"

@interface EditorGridView : NSView
- (void)drawRect:(NSRect)rect;
- (void)mouseDown:(NSEvent *)theEvent;
- (void)loadMap: (NSString*) path;
- (void)saveMap: (NSString*)path;
@end
