//
//  EditorGridView.m
//  LevelEditorCocoa
//
//  Created by Daniel Monteiro on 8/4/24.
//  Copyright (c) 2024 Daniel Monteiro. All rights reserved.
//

#import "EditorGridView.h"

uint32_t mapSurface[32][32];

@implementation EditorGridView


- (void) repaintGame:(NSTimer *)timer
{
    [self setNeedsDisplay: YES ];
}

-(void) initTimer {
    [NSTimer scheduledTimerWithTimeInterval:0.05f
                                     target:self selector:@selector(repaintGame:) userInfo:nil repeats:YES];
    
}


- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code here.
        [self initTimer];
    }
    
    return self;
}

- (void)mouseDown:(NSEvent *)theEvent {
    NSPoint touchPoint = [NSEvent mouseLocation];
    NSPoint event_location = [theEvent locationInWindow];
    NSPoint local_point = [self convertPoint:event_location fromView:nil];
    
    int y = local_point.y / (self.bounds.size.height / 33);
    int x = local_point.x / (self.bounds.size.width / 33);
    mapSurface[32 - y][x] = 1;
}

- (void)drawRect:(NSRect)rect
{
    CGRect bounds;
    bounds.origin.x = rect.origin.x;
    bounds.origin.y = rect.origin.y;
    bounds.size.width = rect.size.width;
    bounds.size.height = rect.size.height;

    /*  MAC_OS_X_VERSION_10_10*/
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 101000
    CGContextRef context = (CGContextRef)[[NSGraphicsContext currentContext] CGContext];
#else
    CGContextRef context = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
#endif
    
    CGContextSaveGState(context);
    
    CGContextSetRGBFillColor(context, 1.0f, 1.0f, 1.0f, 1.0f);

    CGContextFillRect(context, bounds);
    
    
    for( int y = 0; y < 33; ++y) {
        CGContextSetLineWidth(context, 1);
        CGContextMoveToPoint(context, 0, y * (self.bounds.size.height / 33));
        CGContextAddLineToPoint(context, self.bounds.size.width, y * (self.bounds.size.height / 33));
        CGContextStrokePath(context);
    }
    
    for ( int x = 0; x < 33; ++x) {
        CGContextSetLineWidth(context, 1);
        CGContextMoveToPoint(context, x * (self.bounds.size.width / 33), 0);
        CGContextAddLineToPoint(context, x * (self.bounds.size.width / 33), self.bounds.size.height);
        CGContextStrokePath(context);

    }
    
    bounds.size.width = self.bounds.size.width / 33;
    bounds.size.height = self.bounds.size.height / 33;
    
    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 32; ++x ) {
            bounds.origin.x = x * (self.bounds.size.width / 33);
            bounds.origin.y = y * (self.bounds.size.height / 33);

            uint32_t flags = mapSurface[32 - y][x];
            if ((flags & CELL_VOID) == CELL_VOID) {
                CGContextSetRGBFillColor(context, 0.5f, 0.5f, 0.5f, 1.0f);
            } else {
                CGContextSetRGBFillColor(context, 1.0f, 1.0f, 1.0f, 1.0f);
            }

            CGContextFillRect(context, bounds);
            
            if ((flags & VERTICAL_LINE) == VERTICAL_LINE) {
                CGContextSetLineWidth(context, 2);
                CGContextMoveToPoint(context, x * (self.bounds.size.width / 33), y * (self.bounds.size.height / 33));
                CGContextAddLineToPoint(context, x * (self.bounds.size.width / 33), (y + 1) * (self.bounds.size.height / 33));
                CGContextStrokePath(context);
            }
            
            if ((flags & HORIZONTAL_LINE) == HORIZONTAL_LINE) {
                CGContextSetLineWidth(context, 2);
                CGContextMoveToPoint(context, x * (self.bounds.size.width / 33), (y + 1) * (self.bounds.size.height / 33));
                CGContextAddLineToPoint(context, (x + 1) * (self.bounds.size.width / 33), (y + 1) * (self.bounds.size.height / 33));
                CGContextStrokePath(context);
            }
            
            if ((flags & LEFT_FAR_LINE) == LEFT_FAR_LINE) {
                CGContextSetLineWidth(context, 2);
                CGContextMoveToPoint(context, x * (self.bounds.size.width / 33), (y + 1) * (self.bounds.size.height / 33));
                CGContextAddLineToPoint(context, (x + 1) * (self.bounds.size.width / 33), (y + 0) * (self.bounds.size.height / 33));
                CGContextStrokePath(context);
            }
            
            if ((flags & LEFT_NEAR_LINE) == LEFT_NEAR_LINE) {
                CGContextSetLineWidth(context, 2);
                CGContextMoveToPoint(context, (x + 1) * (self.bounds.size.width / 33), (y + 1) * (self.bounds.size.height / 33));
                CGContextAddLineToPoint(context, x * (self.bounds.size.width / 33), (y + 0) * (self.bounds.size.height / 33));
                CGContextStrokePath(context);
            }
            
        }
    }
    

    
    CGContextRestoreGState(context);
}

- (void)updateMap: (Map*) map {
    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 32; ++x) {
            mapSurface[y][x] = getFlags(map, x, y);
        }
    }
}

- (void)saveMap: (Map*)map {
    
}

@end
