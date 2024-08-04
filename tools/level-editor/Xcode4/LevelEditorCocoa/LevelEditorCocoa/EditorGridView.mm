//
//  EditorGridView.m
//  LevelEditorCocoa
//
//  Created by Daniel Monteiro on 8/4/24.
//  Copyright (c) 2024 Daniel Monteiro. All rights reserved.
//

#import "EditorGridView.h"

CGContextRef currentContext;
extern struct Map map;
CGSize viewSize;

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
        initMapEditor();
        [self initTimer];
    }
    
    return self;
}

- (void)mouseDown:(NSEvent *)theEvent {
    NSPoint event_location = [theEvent locationInWindow];
    NSPoint local_point = [self convertPoint:event_location fromView:nil];
    
    onClick(1,
            local_point.x,
            self.bounds.size.height - local_point.y,
            self.bounds.size.width,
            self.bounds.size.height
            );
}

void setLineWidth(uint8_t width) {
    CGContextSetLineWidth(currentContext, width);
}

void setColour(uint32_t colour) {
    float r = ((colour & 0xFF0000) >> 16) / 256.0f;
    float g = ((colour & 0x00FF00) >>  8) / 256.0f;
    float b = ((colour & 0x0000FF) >>  0) / 256.0f;
    
    CGContextSetRGBFillColor(currentContext, r, g, b, 1.0f);
}

void drawLine(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1) {
    CGContextMoveToPoint(currentContext, x0, viewSize.height - y0);
    CGContextAddLineToPoint(currentContext, x1, viewSize.height - y1);
    CGContextStrokePath(currentContext);

}

void fillRect(uint32_t x0, uint32_t y0, uint32_t width, uint32_t height) {
    CGRect bounds;
    bounds.origin.x = x0;
    bounds.origin.y = viewSize.height - y0 - height;
    bounds.size.width = width;
    bounds.size.height = height;
    CGContextFillRect(currentContext, bounds);
}


- (void)drawRect:(NSRect)rect
{
    CGRect bounds;
    bounds.origin.x = rect.origin.x;
    bounds.origin.y = rect.origin.y;
    bounds.size.width = rect.size.width;
    bounds.size.height = rect.size.height;
    viewSize = self.bounds.size;
    /*  MAC_OS_X_VERSION_10_10*/
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 101000
    CGContextRef context = (CGContextRef)[[NSGraphicsContext currentContext] CGContext];
#else
    CGContextRef context = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
#endif
    
    CGContextSaveGState(context);
    
    CGContextSetRGBFillColor(context, 1.0f, 1.0f, 1.0f, 1.0f);

    CGContextFillRect(context, bounds);
    
    currentContext = context;
    
    redrawGrid(self.bounds.size.width, self.bounds.size.height);
    
    CGContextRestoreGState(context);
}

- (void)loadMap: (NSString*) path {
    map = loadMap( path.UTF8String );
}

- (void)saveMap: (NSString*)path {
    saveMap(path.UTF8String, &map);
}


@end
