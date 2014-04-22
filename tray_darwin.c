#import <Cocoa/Cocoa.h>
#include <strings.h>
#include "_cgo_export.h"

typedef void(*CallbackFunc)(void *, GoInt);
typedef struct {
    void *manager;      // The Go object that will handle the click
    unsigned int index; // Representative index of which go-side callback to invoke
    CallbackFunc callback;
} MenuCallbackInfo;

@interface AppDelegate: NSObject <NSApplicationDelegate>
{
    NSMutableDictionary *icons;
}
- (void)applicationDidFinishLaunching:(NSNotification *)aNotification;
- (void)showIcon:(NSString*)path hint:(NSString *)hint;
- (void)addMenuItem:(NSString*)item manager:(void*)manager index:(int)index callback:(CallbackFunc)callback;

- (IBAction)clicked:(id)sender;
- (IBAction)quit:(id)sender;
- (IBAction)menuItem:(id)sender;


@property (strong) NSStatusItem *statusItem;
@property (assign) IBOutlet NSWindow *window;

@end

@implementation AppDelegate
NSMenu *m_menu;
void *m_manager;
const char *m_initialIconPath;
const char *m_initialHint;

@synthesize window = _window;

- (id)init:(void *)manager iconPath:(const char *)iconPath hint:(const char*)hint{
    if ((self = [super init])) {
        m_manager = manager;
        m_initialIconPath = iconPath;
        m_initialHint = hint;
    }
    return self;
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    NSLog(@"In applicationDidFinishLaunching");

    // Create a menubar item
    self.statusItem = [[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength];
    NSLog(@"Created status item");

    // Useful for debugging if icon loading is broken (e.g., icons don't exist)
    //[self.statusItem setTitle:@"SystrayTest"];

    // Set up a general click handler - this will happen in addition to any menu
    [self.statusItem setAction:@selector(clicked:)];

    // Create our menu and add some items
    NSMenu *statusMenu = [[NSMenu allocWithZone:[NSMenu menuZone]] initWithTitle:@"Custom"];
    
    // Add a default Quit item
    NSMenuItem *quitItem = [[NSMenuItem allocWithZone:[NSMenu menuZone]] initWithTitle:@"Quit" action:@selector(quit:) keyEquivalent:@""];
    [quitItem setEnabled:YES];
    [statusMenu addItem:quitItem];

    [self.statusItem setMenu:statusMenu];

    // Set up an icon cache for loaded image data
    icons = [[NSMutableDictionary alloc] init];

    // Set our initial icon and hint/tooltip
    NSString *nsIcon = [NSString stringWithCString:m_initialIconPath encoding:NSASCIIStringEncoding];
    NSString *nsHint = [NSString stringWithCString:m_initialHint encoding:NSASCIIStringEncoding];
    [self showIcon:nsIcon hint:nsHint];

    // Tell our caller that the menubar item has been created
    NSLog(@"AppDelegate setup finished, calling menuCreatedCallback");
    menuCreatedCallback(m_manager);
}

// Add a new menu item, with callback and metadata
- (void)addMenuItem:(NSString*)item manager:(void*)manager index:(int)index callback:(CallbackFunc)callback {
    MenuCallbackInfo callbackInfo;
    callbackInfo.manager = manager;
    callbackInfo.index = index;
    callbackInfo.callback = callback;
    NSMenuItem *newItem = [[NSMenuItem allocWithZone:[NSMenu menuZone]] initWithTitle:item action:@selector(menuItem:) keyEquivalent:@""];
    [newItem setEnabled:YES];
    [newItem setRepresentedObject:[NSValue value:&callbackInfo withObjCType:@encode(MenuCallbackInfo)]];
    
    // Implicitly leave the last item ("Quit") at the end
    int insertionIndex = [[self.statusItem menu] numberOfItems] - 1;
    if (insertionIndex < 0) {
        // No items added yet
        insertionIndex = 0;
    }
    [[self.statusItem menu] insertItem:newItem atIndex:insertionIndex];
}

// Process a previously added menu item, extracting the callback info and invoking
// the callback
- (IBAction)menuItem:(id)sender {
    NSLog(@"Menu item!");
    NSValue *nsCallbackInfo = [sender representedObject];
    MenuCallbackInfo callbackInfo;
    [nsCallbackInfo getValue:&callbackInfo];
    if (callbackInfo.manager != nil && callbackInfo.callback != nil) {
        NSLog(@"Issuing callback");
        callbackInfo.callback(callbackInfo.manager, callbackInfo.index);
    }
}

// Provide any necessary response to a click in addition to the menu
- (IBAction)clicked:(id)sender {
    // This could include a generic callback over the fence to Go
    // if we want to do something like reset an idle timer.
    //NSLog(@"clicked");
}

// Exit the application (or signal an exit elsewhere, or just return and thereby
// imply an exit...)
- (IBAction)quit:(id)sender {
    // TODO: wire this to a callback, instead of just exiting
    NSLog(@"quit!!!");
    [NSApp terminate:self];
}


// Set the menubar icon and hint, if any. Either value may
// be nil, in which case no action is taken.
// TBD: decide if nil should mean "remove" instead
- (void)showIcon:(NSString*)path hint:(NSString *)hint {
    if (path) {
        NSImage *icon = [icons objectForKey:path];
        if (!icon) {
            NSLog(@"Creating new icon from file");
            icon = [[NSImage alloc] initWithContentsOfFile:path];
            if (icon) {
                [icons setValue:icon forKey:path];
            }
        }
        if (icon) {
            NSLog(@"Setting icon image");
            [self.statusItem setImage:icon];
        }
    }
    if (hint) {
        NSLog(@"Setting icon hint");
        [self.statusItem setToolTip:hint];
    }
}
@end

// External API - these are the C functions that are directly callable
// from Go.

// Run the cocoa application's main event loop. This must be run on the main
// thread and will block, so architect accordingly, especially cross-platform.

// TODO: Enforce the main thread restriction earlier, rather than letting some
// NS code assert
void runApplication(const char *title,
                    const char *initialIcon,
                    const char *initialHint,
                    void *manager) {
    [NSAutoreleasePool new];
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];

    AppDelegate *delegate = [[[AppDelegate alloc] init:manager iconPath:initialIcon hint:initialHint] autorelease];
    [NSApp setDelegate:delegate];

    //[NSApp activateIgnoringOtherApps:YES];
    NSLog(@"Running main application");
    [NSApp run];
}


// Set the currently displayed icon
// TODO: figure out how we want to pass unicode
void setIcon(const char *path) {
    NSString *nsPath = [NSString stringWithCString:path encoding:NSASCIIStringEncoding];
    [[NSApp delegate] showIcon:nsPath hint:nil];
}

// Set the currently displayed hint
void setHint(const char *hint) {
    NSString *nsHint = [NSString stringWithCString:hint encoding:NSASCIIStringEncoding];
    [[NSApp delegate] showIcon:nil hint:nsHint];
}

// Add a new item to the menu, with some (opaque) info on how to process it back on
// the other side
void addSystrayMenuItem(const char *item, void *object, unsigned int index) {
    [[NSApp delegate] addMenuItem:[NSString stringWithCString:item encoding:NSASCIIStringEncoding] manager:object index:index callback:&menuClickCallback];
}

