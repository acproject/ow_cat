#include "platform/macos/macos_ime_adapter.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <iostream>

#ifdef __APPLE__
#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>

// Forward declarations
@class OwCatCandidateWindowController;
@class OwCatCandidateView;

// Objective-C Candidate View
@interface OwCatCandidateView : NSView {
    MacOSCandidateWindow* _cppWindow;
    NSArray* _candidates;
    NSInteger _selectedIndex;
    NSFont* _font;
    NSColor* _backgroundColor;
    NSColor* _textColor;
    NSColor* _selectedBackgroundColor;
    NSColor* _selectedTextColor;
    NSColor* _borderColor;
    CGFloat _itemHeight;
    CGFloat _padding;
}

@property (nonatomic, assign) MacOSCandidateWindow* cppWindow;
@property (nonatomic, strong) NSArray* candidates;
@property (nonatomic, assign) NSInteger selectedIndex;
@property (nonatomic, strong) NSFont* font;
@property (nonatomic, strong) NSColor* backgroundColor;
@property (nonatomic, strong) NSColor* textColor;
@property (nonatomic, strong) NSColor* selectedBackgroundColor;
@property (nonatomic, strong) NSColor* selectedTextColor;
@property (nonatomic, strong) NSColor* borderColor;
@property (nonatomic, assign) CGFloat itemHeight;
@property (nonatomic, assign) CGFloat padding;

- (instancetype)initWithCppWindow:(MacOSCandidateWindow*)window;
- (void)updateCandidates:(NSArray*)candidates selectedIndex:(NSInteger)index;
- (NSSize)calculateOptimalSize;
- (void)mouseDown:(NSEvent*)event;
- (void)mouseMoved:(NSEvent*)event;
- (void)drawRect:(NSRect)dirtyRect;

@end

@implementation OwCatCandidateView

@synthesize cppWindow = _cppWindow;
@synthesize candidates = _candidates;
@synthesize selectedIndex = _selectedIndex;
@synthesize font = _font;
@synthesize backgroundColor = _backgroundColor;
@synthesize textColor = _textColor;
@synthesize selectedBackgroundColor = _selectedBackgroundColor;
@synthesize selectedTextColor = _selectedTextColor;
@synthesize borderColor = _borderColor;
@synthesize itemHeight = _itemHeight;
@synthesize padding = _padding;

- (instancetype)initWithCppWindow:(MacOSCandidateWindow*)window {
    self = [super init];
    if (self) {
        _cppWindow = window;
        _candidates = @[];
        _selectedIndex = 0;
        _itemHeight = 24.0;
        _padding = 8.0;
        
        // Initialize colors and font
        _font = [NSFont systemFontOfSize:14.0];
        _backgroundColor = [NSColor colorWithRed:0.95 green:0.95 blue:0.95 alpha:0.95];
        _textColor = [NSColor blackColor];
        _selectedBackgroundColor = [NSColor colorWithRed:0.2 green:0.4 blue:0.8 alpha:1.0];
        _selectedTextColor = [NSColor whiteColor];
        _borderColor = [NSColor colorWithRed:0.7 green:0.7 blue:0.7 alpha:1.0];
        
        // Enable mouse tracking
        NSTrackingArea* trackingArea = [[NSTrackingArea alloc] 
                                       initWithRect:NSZeroRect
                                            options:(NSTrackingMouseMoved | NSTrackingActiveInKeyWindow | NSTrackingInVisibleRect)
                                              owner:self
                                           userInfo:nil];
        [self addTrackingArea:trackingArea];
    }
    return self;
}

- (void)updateCandidates:(NSArray*)candidates selectedIndex:(NSInteger)index {
    _candidates = candidates;
    _selectedIndex = index;
    
    // Update window size
    NSSize newSize = [self calculateOptimalSize];
    NSWindow* window = [self window];
    if (window) {
        NSRect frame = [window frame];
        frame.size = newSize;
        [window setFrame:frame display:YES animate:NO];
    }
    
    [self setNeedsDisplay:YES];
}

- (NSSize)calculateOptimalSize {
    if ([_candidates count] == 0) {
        return NSMakeSize(100, 50);
    }
    
    CGFloat maxWidth = 0;
    for (NSString* candidate in _candidates) {
        NSDictionary* attributes = @{NSFontAttributeName: _font};
        NSSize textSize = [candidate sizeWithAttributes:attributes];
        maxWidth = MAX(maxWidth, textSize.width);
    }
    
    CGFloat width = maxWidth + _padding * 2 + 30; // Extra space for index numbers
    CGFloat height = [_candidates count] * _itemHeight + _padding * 2;
    
    // Limit maximum size
    width = MIN(width, 400);
    height = MIN(height, 300);
    
    return NSMakeSize(width, height);
}

- (void)mouseDown:(NSEvent*)event {
    NSPoint location = [self convertPoint:[event locationInWindow] fromView:nil];
    NSInteger clickedIndex = (NSInteger)((location.y - _padding) / _itemHeight);
    
    if (clickedIndex >= 0 && clickedIndex < [_candidates count]) {
        _selectedIndex = clickedIndex;
        [self setNeedsDisplay:YES];
        
        // Notify C++ window
        if (_cppWindow) {
            NSString* selectedCandidate = [_candidates objectAtIndex:clickedIndex];
            std::string candidateStr = [selectedCandidate UTF8String];
            _cppWindow->selectCandidate(candidateStr);
        }
    }
}

- (void)mouseMoved:(NSEvent*)event {
    NSPoint location = [self convertPoint:[event locationInWindow] fromView:nil];
    NSInteger hoveredIndex = (NSInteger)((location.y - _padding) / _itemHeight);
    
    if (hoveredIndex >= 0 && hoveredIndex < [_candidates count] && hoveredIndex != _selectedIndex) {
        _selectedIndex = hoveredIndex;
        [self setNeedsDisplay:YES];
        
        // Notify C++ window about highlight change
        if (_cppWindow) {
            NSString* hoveredCandidate = [_candidates objectAtIndex:hoveredIndex];
            std::string candidateStr = [hoveredCandidate UTF8String];
            _cppWindow->highlightCandidate(candidateStr);
        }
    }
}

- (void)drawRect:(NSRect)dirtyRect {
    // Clear background
    [_backgroundColor setFill];
    NSRectFill(dirtyRect);
    
    // Draw border
    [_borderColor setStroke];
    NSBezierPath* border = [NSBezierPath bezierPathWithRect:[self bounds]];
    [border setLineWidth:1.0];
    [border stroke];
    
    // Draw candidates
    for (NSInteger i = 0; i < [_candidates count]; i++) {
        NSString* candidate = [_candidates objectAtIndex:i];
        NSRect itemRect = NSMakeRect(_padding, 
                                   [self bounds].size.height - (i + 1) * _itemHeight - _padding,
                                   [self bounds].size.width - _padding * 2,
                                   _itemHeight);
        
        // Draw selection background
        if (i == _selectedIndex) {
            [_selectedBackgroundColor setFill];
            NSRectFill(itemRect);
        }
        
        // Draw index number
        NSString* indexStr = [NSString stringWithFormat:@"%ld.", (long)(i + 1)];
        NSDictionary* indexAttributes = @{
            NSFontAttributeName: _font,
            NSForegroundColorAttributeName: (i == _selectedIndex) ? _selectedTextColor : _textColor
        };
        
        NSRect indexRect = NSMakeRect(itemRect.origin.x + 2,
                                    itemRect.origin.y + (itemRect.size.height - [_font pointSize]) / 2,
                                    25,
                                    [_font pointSize]);
        [indexStr drawInRect:indexRect withAttributes:indexAttributes];
        
        // Draw candidate text
        NSDictionary* textAttributes = @{
            NSFontAttributeName: _font,
            NSForegroundColorAttributeName: (i == _selectedIndex) ? _selectedTextColor : _textColor
        };
        
        NSRect textRect = NSMakeRect(itemRect.origin.x + 30,
                                   itemRect.origin.y + (itemRect.size.height - [_font pointSize]) / 2,
                                   itemRect.size.width - 32,
                                   [_font pointSize]);
        [candidate drawInRect:textRect withAttributes:textAttributes];
    }
}

@end

// Objective-C Window Controller
@interface OwCatCandidateWindowController : NSWindowController {
    MacOSCandidateWindow* _cppWindow;
    OwCatCandidateView* _candidateView;
}

@property (nonatomic, assign) MacOSCandidateWindow* cppWindow;
@property (nonatomic, strong) OwCatCandidateView* candidateView;

- (instancetype)initWithCppWindow:(MacOSCandidateWindow*)window;
- (void)showCandidates:(NSArray*)candidates selectedIndex:(NSInteger)index atPosition:(NSPoint)position;
- (void)hideCandidates;
- (void)updateSelection:(NSInteger)index;

@end

@implementation OwCatCandidateWindowController

@synthesize cppWindow = _cppWindow;
@synthesize candidateView = _candidateView;

- (instancetype)initWithCppWindow:(MacOSCandidateWindow*)window {
    // Create the window
    NSRect windowRect = NSMakeRect(0, 0, 200, 100);
    NSWindow* candidateWindow = [[NSWindow alloc] 
                               initWithContentRect:windowRect
                                         styleMask:NSWindowStyleMaskBorderless
                                           backing:NSBackingStoreBuffered
                                             defer:NO];
    
    self = [super initWithWindow:candidateWindow];
    if (self) {
        _cppWindow = window;
        
        // Configure window
        [candidateWindow setLevel:NSFloatingWindowLevel];
        [candidateWindow setOpaque:NO];
        [candidateWindow setBackgroundColor:[NSColor clearColor]];
        [candidateWindow setHasShadow:YES];
        [candidateWindow setAcceptsMouseMovedEvents:YES];
        
        // Create and set up the candidate view
        _candidateView = [[OwCatCandidateView alloc] initWithCppWindow:window];
        [candidateWindow setContentView:_candidateView];
    }
    return self;
}

- (void)showCandidates:(NSArray*)candidates selectedIndex:(NSInteger)index atPosition:(NSPoint)position {
    [_candidateView updateCandidates:candidates selectedIndex:index];
    
    // Position the window
    NSWindow* window = [self window];
    NSRect frame = [window frame];
    frame.origin = position;
    
    // Adjust position to keep window on screen
    NSScreen* screen = [NSScreen mainScreen];
    NSRect screenFrame = [screen visibleFrame];
    
    if (frame.origin.x + frame.size.width > screenFrame.origin.x + screenFrame.size.width) {
        frame.origin.x = screenFrame.origin.x + screenFrame.size.width - frame.size.width;
    }
    if (frame.origin.y - frame.size.height < screenFrame.origin.y) {
        frame.origin.y = screenFrame.origin.y + frame.size.height;
    }
    
    [window setFrame:frame display:YES animate:NO];
    [window orderFront:nil];
}

- (void)hideCandidates {
    [[self window] orderOut:nil];
}

- (void)updateSelection:(NSInteger)index {
    [_candidateView setSelectedIndex:index];
    [_candidateView setNeedsDisplay:YES];
}

@end

#endif // __APPLE__

// C++ Implementation
struct MacOSCandidateWindow::Impl {
#ifdef __APPLE__
    OwCatCandidateWindowController* windowController;
#endif
    
    // Callbacks
    std::function<void(const std::string&)> selectionCallback;
    std::function<void(const std::string&)> highlightCallback;
    
    // State
    std::vector<std::string> candidates;
    int selectedIndex;
    bool isVisible;
    
    Impl() {
#ifdef __APPLE__
        windowController = nil;
#endif
        selectedIndex = 0;
        isVisible = false;
    }
    
    ~Impl() {
#ifdef __APPLE__
        if (windowController) {
            [windowController release];
            windowController = nil;
        }
#endif
    }
};

MacOSCandidateWindow::MacOSCandidateWindow() : pImpl(std::make_unique<Impl>()) {
}

MacOSCandidateWindow::~MacOSCandidateWindow() = default;

bool MacOSCandidateWindow::initialize() {
#ifdef __APPLE__
    pImpl->windowController = [[OwCatCandidateWindowController alloc] initWithCppWindow:this];
    return pImpl->windowController != nil;
#else
    return false;
#endif
}

void MacOSCandidateWindow::shutdown() {
#ifdef __APPLE__
    if (pImpl->windowController) {
        [pImpl->windowController hideCandidates];
        [pImpl->windowController release];
        pImpl->windowController = nil;
    }
#endif
    pImpl->isVisible = false;
}

void MacOSCandidateWindow::show(const std::vector<std::string>& candidates, int selectedIndex, int x, int y) {
#ifdef __APPLE__
    if (!pImpl->windowController) {
        return;
    }
    
    pImpl->candidates = candidates;
    pImpl->selectedIndex = selectedIndex;
    pImpl->isVisible = true;
    
    // Convert candidates to NSArray
    NSMutableArray* nsArray = [[NSMutableArray alloc] init];
    for (const auto& candidate : candidates) {
        NSString* candidateStr = [NSString stringWithUTF8String:candidate.c_str()];
        [nsArray addObject:candidateStr];
    }
    
    // Convert coordinates (flip Y coordinate for macOS)
    NSScreen* screen = [NSScreen mainScreen];
    NSRect screenFrame = [screen frame];
    NSPoint position = NSMakePoint(x, screenFrame.size.height - y);
    
    [pImpl->windowController showCandidates:nsArray selectedIndex:selectedIndex atPosition:position];
    [nsArray release];
#endif
}

void MacOSCandidateWindow::hide() {
#ifdef __APPLE__
    if (pImpl->windowController) {
        [pImpl->windowController hideCandidates];
    }
#endif
    pImpl->isVisible = false;
}

void MacOSCandidateWindow::updateCandidates(const std::vector<std::string>& candidates, int selectedIndex) {
#ifdef __APPLE__
    if (!pImpl->windowController || !pImpl->isVisible) {
        return;
    }
    
    pImpl->candidates = candidates;
    pImpl->selectedIndex = selectedIndex;
    
    // Convert candidates to NSArray
    NSMutableArray* nsArray = [[NSMutableArray alloc] init];
    for (const auto& candidate : candidates) {
        NSString* candidateStr = [NSString stringWithUTF8String:candidate.c_str()];
        [nsArray addObject:candidateStr];
    }
    
    // Get current position
    NSWindow* window = [pImpl->windowController window];
    NSPoint currentPosition = [window frame].origin;
    
    [pImpl->windowController showCandidates:nsArray selectedIndex:selectedIndex atPosition:currentPosition];
    [nsArray release];
#endif
}

void MacOSCandidateWindow::updateSelection(int selectedIndex) {
#ifdef __APPLE__
    if (!pImpl->windowController || !pImpl->isVisible) {
        return;
    }
    
    pImpl->selectedIndex = selectedIndex;
    [pImpl->windowController updateSelection:selectedIndex];
#endif
}

void MacOSCandidateWindow::setSelectionCallback(std::function<void(const std::string&)> callback) {
    pImpl->selectionCallback = callback;
}

void MacOSCandidateWindow::setHighlightCallback(std::function<void(const std::string&)> callback) {
    pImpl->highlightCallback = callback;
}

void MacOSCandidateWindow::selectCandidate(const std::string& candidate) {
    if (pImpl->selectionCallback) {
        pImpl->selectionCallback(candidate);
    }
}

void MacOSCandidateWindow::highlightCandidate(const std::string& candidate) {
    if (pImpl->highlightCallback) {
        pImpl->highlightCallback(candidate);
    }
}

bool MacOSCandidateWindow::isVisible() const {
    return pImpl->isVisible;
}

std::vector<std::string> MacOSCandidateWindow::getCandidates() const {
    return pImpl->candidates;
}

int MacOSCandidateWindow::getSelectedIndex() const {
    return pImpl->selectedIndex;
}

// Bridge functions for Objective-C interoperability
extern "C" {
    MacOSCandidateWindow* createMacOSCandidateWindow() {
        return new MacOSCandidateWindow();
    }
    
    void destroyMacOSCandidateWindow(MacOSCandidateWindow* window) {
        delete window;
    }
}