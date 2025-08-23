#include "platform/macos/macos_ime_adapter.h"
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <iostream>

#ifdef __APPLE__
#import <Cocoa/Cocoa.h>
#import <InputMethodKit/InputMethodKit.h>
#import <Carbon/Carbon.h>

// Forward declarations for Objective-C classes
@class OwCatInputController;
@class OwCatCandidateWindow;

// Objective-C Input Controller implementation
@interface OwCatInputController : IMKInputController {
    MacOSInputController* _cppController;
    NSString* _composedBuffer;
    NSRange _markedRange;
    NSRange _selectedRange;
    BOOL _isComposing;
}

@property (nonatomic, assign) MacOSInputController* cppController;
@property (nonatomic, strong) NSString* composedBuffer;
@property (nonatomic, assign) NSRange markedRange;
@property (nonatomic, assign) NSRange selectedRange;
@property (nonatomic, assign) BOOL isComposing;

- (instancetype)initWithServer:(IMKServer*)server delegate:(id)delegate client:(id)inputClient cppController:(MacOSInputController*)controller;
- (BOOL)inputText:(NSString*)string client:(id)sender;
- (void)commitComposition:(id)sender;
- (void)cancelComposition;
- (NSArray*)candidates:(id)sender;
- (void)candidateSelected:(NSAttributedString*)candidateString;
- (void)candidateSelectionChanged:(NSAttributedString*)candidateString;
- (NSMenu*)menu;

@end

@implementation OwCatInputController

@synthesize cppController = _cppController;
@synthesize composedBuffer = _composedBuffer;
@synthesize markedRange = _markedRange;
@synthesize selectedRange = _selectedRange;
@synthesize isComposing = _isComposing;

- (instancetype)initWithServer:(IMKServer*)server delegate:(id)delegate client:(id)inputClient cppController:(MacOSInputController*)controller {
    self = [super initWithServer:server delegate:delegate client:inputClient];
    if (self) {
        _cppController = controller;
        _composedBuffer = @"";
        _markedRange = NSMakeRange(NSNotFound, 0);
        _selectedRange = NSMakeRange(0, 0);
        _isComposing = NO;
    }
    return self;
}

- (BOOL)inputText:(NSString*)string client:(id)sender {
    if (!_cppController) {
        return NO;
    }
    
    // Convert NSString to std::string
    std::string inputStr = [string UTF8String];
    
    // Handle special keys
    if ([string isEqualToString:@"\r"] || [string isEqualToString:@"\n"]) {
        // Enter key - commit current composition
        if (_isComposing && [_composedBuffer length] > 0) {
            [self commitComposition:sender];
            return YES;
        }
        return NO;
    }
    
    if ([string isEqualToString:@"\x1b"]) {
        // Escape key - cancel composition
        if (_isComposing) {
            [self cancelComposition];
            return YES;
        }
        return NO;
    }
    
    if ([string isEqualToString:@"\x7f"]) {
        // Backspace key
        if (_isComposing && [_composedBuffer length] > 0) {
            _composedBuffer = [_composedBuffer substringToIndex:[_composedBuffer length] - 1];
            if ([_composedBuffer length] == 0) {
                [self cancelComposition];
            } else {
                _cppController->handleInput(inputStr);
                [self updateComposition:sender];
            }
            return YES;
        }
        return NO;
    }
    
    // Regular character input
    if (!_isComposing) {
        _isComposing = YES;
        _composedBuffer = @"";
    }
    
    _composedBuffer = [_composedBuffer stringByAppendingString:string];
    
    // Notify C++ controller
    _cppController->handleInput(inputStr);
    
    [self updateComposition:sender];
    
    return YES;
}

- (void)updateComposition:(id)sender {
    if (!_isComposing || [_composedBuffer length] == 0) {
        return;
    }
    
    // Create attributed string for composition
    NSMutableAttributedString* attrString = [[NSMutableAttributedString alloc] initWithString:_composedBuffer];
    
    // Add underline attribute to show it's being composed
    [attrString addAttribute:NSUnderlineStyleAttributeName
                       value:@(NSUnderlineStyleSingle)
                       range:NSMakeRange(0, [_composedBuffer length])];
    
    // Set marked text
    _markedRange = NSMakeRange(0, [_composedBuffer length]);
    [sender setMarkedText:attrString
           selectionRange:NSMakeRange([_composedBuffer length], 0)
         replacementRange:NSMakeRange(NSNotFound, NSNotFound)];
}

- (void)commitComposition:(id)sender {
    if (!_isComposing) {
        return;
    }
    
    // Get final text from C++ controller
    std::string finalText = _cppController->getFinalText();
    NSString* commitText = [NSString stringWithUTF8String:finalText.c_str()];
    
    if ([commitText length] == 0) {
        commitText = _composedBuffer;
    }
    
    // Insert the final text
    [sender insertText:commitText replacementRange:NSMakeRange(NSNotFound, NSNotFound)];
    
    // Reset state
    _isComposing = NO;
    _composedBuffer = @"";
    _markedRange = NSMakeRange(NSNotFound, 0);
    _selectedRange = NSMakeRange(0, 0);
    
    // Notify C++ controller
    _cppController->commitText(finalText);
}

- (void)cancelComposition {
    if (!_isComposing) {
        return;
    }
    
    // Reset state
    _isComposing = NO;
    _composedBuffer = @"";
    _markedRange = NSMakeRange(NSNotFound, 0);
    _selectedRange = NSMakeRange(0, 0);
    
    // Notify C++ controller
    _cppController->cancelComposition();
}

- (NSArray*)candidates:(id)sender {
    if (!_cppController) {
        return @[];
    }
    
    // Get candidates from C++ controller
    std::vector<std::string> candidates = _cppController->getCandidates();
    
    NSMutableArray* nsArray = [[NSMutableArray alloc] init];
    for (const auto& candidate : candidates) {
        NSString* candidateStr = [NSString stringWithUTF8String:candidate.c_str()];
        [nsArray addObject:candidateStr];
    }
    
    return nsArray;
}

- (void)candidateSelected:(NSAttributedString*)candidateString {
    if (!_cppController) {
        return;
    }
    
    std::string selectedText = [[candidateString string] UTF8String];
    _cppController->selectCandidate(selectedText);
    
    // Commit the selected candidate
    [self commitComposition:[self client]];
}

- (void)candidateSelectionChanged:(NSAttributedString*)candidateString {
    if (!_cppController) {
        return;
    }
    
    std::string selectedText = [[candidateString string] UTF8String];
    _cppController->highlightCandidate(selectedText);
}

- (NSMenu*)menu {
    // Create context menu for input method
    NSMenu* menu = [[NSMenu alloc] init];
    
    NSMenuItem* aboutItem = [[NSMenuItem alloc] initWithTitle:@"About OwCat IME"
                                                       action:@selector(showAbout:)
                                                keyEquivalent:@""];
    [aboutItem setTarget:self];
    [menu addItem:aboutItem];
    
    [menu addItem:[NSMenuItem separatorItem]];
    
    NSMenuItem* prefsItem = [[NSMenuItem alloc] initWithTitle:@"Preferences..."
                                                       action:@selector(showPreferences:)
                                                keyEquivalent:@""];
    [prefsItem setTarget:self];
    [menu addItem:prefsItem];
    
    return menu;
}

- (void)showAbout:(id)sender {
    NSAlert* alert = [[NSAlert alloc] init];
    [alert setMessageText:@"OwCat IME"];
    [alert setInformativeText:@"A modern Chinese input method with AI-powered predictions."];
    [alert addButtonWithTitle:@"OK"];
    [alert runModal];
}

- (void)showPreferences:(id)sender {
    // TODO: Implement preferences window
    NSAlert* alert = [[NSAlert alloc] init];
    [alert setMessageText:@"Preferences"];
    [alert setInformativeText:@"Preferences window is not implemented yet."];
    [alert addButtonWithTitle:@"OK"];
    [alert runModal];
}

@end

#endif // __APPLE__

// C++ Implementation
struct MacOSInputController::Impl {
#ifdef __APPLE__
    OwCatInputController* objcController;
    IMKServer* server;
#endif
    
    // Callbacks
    std::function<void(const std::string&)> inputCallback;
    std::function<void(const std::string&)> commitCallback;
    std::function<void()> cancelCallback;
    std::function<std::vector<std::string>()> candidatesCallback;
    std::function<void(const std::string&)> selectCallback;
    std::function<void(const std::string&)> highlightCallback;
    
    // State
    std::string currentInput;
    std::vector<std::string> currentCandidates;
    std::string finalText;
    
    Impl() {
#ifdef __APPLE__
        objcController = nil;
        server = nil;
#endif
    }
    
    ~Impl() {
#ifdef __APPLE__
        if (objcController) {
            [objcController release];
            objcController = nil;
        }
#endif
    }
};

MacOSInputController::MacOSInputController() : pImpl(std::make_unique<Impl>()) {
}

MacOSInputController::~MacOSInputController() = default;

bool MacOSInputController::initialize() {
#ifdef __APPLE__
    // The Objective-C controller will be created by IMKServer
    return true;
#else
    return false;
#endif
}

void MacOSInputController::shutdown() {
#ifdef __APPLE__
    if (pImpl->objcController) {
        [pImpl->objcController release];
        pImpl->objcController = nil;
    }
#endif
}

void MacOSInputController::setInputCallback(std::function<void(const std::string&)> callback) {
    pImpl->inputCallback = callback;
}

void MacOSInputController::setCommitCallback(std::function<void(const std::string&)> callback) {
    pImpl->commitCallback = callback;
}

void MacOSInputController::setCancelCallback(std::function<void()> callback) {
    pImpl->cancelCallback = callback;
}

void MacOSInputController::setCandidatesCallback(std::function<std::vector<std::string>()> callback) {
    pImpl->candidatesCallback = callback;
}

void MacOSInputController::setSelectCallback(std::function<void(const std::string&)> callback) {
    pImpl->selectCallback = callback;
}

void MacOSInputController::setHighlightCallback(std::function<void(const std::string&)> callback) {
    pImpl->highlightCallback = callback;
}

void MacOSInputController::handleInput(const std::string& input) {
    pImpl->currentInput += input;
    
    if (pImpl->inputCallback) {
        pImpl->inputCallback(input);
    }
}

void MacOSInputController::commitText(const std::string& text) {
    pImpl->currentInput.clear();
    pImpl->currentCandidates.clear();
    pImpl->finalText.clear();
    
    if (pImpl->commitCallback) {
        pImpl->commitCallback(text);
    }
}

void MacOSInputController::cancelComposition() {
    pImpl->currentInput.clear();
    pImpl->currentCandidates.clear();
    pImpl->finalText.clear();
    
    if (pImpl->cancelCallback) {
        pImpl->cancelCallback();
    }
}

std::vector<std::string> MacOSInputController::getCandidates() {
    if (pImpl->candidatesCallback) {
        pImpl->currentCandidates = pImpl->candidatesCallback();
        return pImpl->currentCandidates;
    }
    return {};
}

void MacOSInputController::selectCandidate(const std::string& candidate) {
    pImpl->finalText = candidate;
    
    if (pImpl->selectCallback) {
        pImpl->selectCallback(candidate);
    }
}

void MacOSInputController::highlightCandidate(const std::string& candidate) {
    if (pImpl->highlightCallback) {
        pImpl->highlightCallback(candidate);
    }
}

std::string MacOSInputController::getCurrentInput() const {
    return pImpl->currentInput;
}

std::string MacOSInputController::getFinalText() const {
    return pImpl->finalText;
}

void MacOSInputController::setFinalText(const std::string& text) {
    pImpl->finalText = text;
}

#ifdef __APPLE__
void* MacOSInputController::getObjCController() {
    return pImpl->objcController;
}

void MacOSInputController::setObjCController(void* controller) {
    pImpl->objcController = static_cast<OwCatInputController*>(controller);
}
#endif

// Bridge functions for Objective-C interoperability
extern "C" {
    MacOSInputController* createMacOSInputController() {
        return new MacOSInputController();
    }
    
    void destroyMacOSInputController(MacOSInputController* controller) {
        delete controller;
    }
    
#ifdef __APPLE__
    void* createObjCInputController(IMKServer* server, id delegate, id client, MacOSInputController* cppController) {
        OwCatInputController* controller = [[OwCatInputController alloc] 
                                          initWithServer:server 
                                                delegate:delegate 
                                                  client:client 
                                           cppController:cppController];
        cppController->setObjCController(controller);
        return controller;
    }
#endif
}