/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2026 Petr Ohlidal & contributors

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/// @file   MacOS.mm
/// @brief  Cocoa glue for the macOS port - see MacOS.h for the rationale.
///
/// NOTE: This file is compiled WITHOUT ARC (-fno-objc-arc is set in CMake), so memory is managed
/// manually. Keep it that way or the retain/release calls below become compile errors.

#import <Cocoa/Cocoa.h>

#include "MacOS.h"

namespace {

// These flags are only ever touched from the main thread (Cocoa delegate callbacks run there, and so
// does the game loop), so plain bools are sufficient.
bool g_quit_requested = false;
bool g_resize_pending = false;
bool g_focus_changed  = false;
int  g_terminate_calls = 0;

} // anonymous namespace

// ---------------------------------------------------------------------------------------------
// Delegate: receives Cmd+Q / close button / resize / focus notifications and turns them into flags
// which the game loop polls. We never let Cocoa tear the process down on its own - the game must
// shut down gracefully (save config, stop threads, release OpenGL).
// ---------------------------------------------------------------------------------------------

@interface RoRCocoaDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>
@end

@implementation RoRCocoaDelegate

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender
{
    (void)sender;
    g_quit_requested = true;
    // If the game loop is wedged (or the OS is logging out) and we are asked again, stop stalling.
    g_terminate_calls++;
    return (g_terminate_calls > 1) ? NSTerminateNow : NSTerminateCancel;
}

- (BOOL)windowShouldClose:(NSWindow*)sender
{
    (void)sender;
    g_quit_requested = true;
    return NO; // the game loop closes the window itself during shutdown
}

- (void)windowDidResize:(NSNotification*)notification
{
    (void)notification;
    g_resize_pending = true;
}

- (void)windowDidChangeBackingProperties:(NSNotification*)notification
{
    (void)notification; // moved to a display with a different scale factor
    g_resize_pending = true;
}

- (void)windowDidBecomeKey:(NSNotification*)notification
{
    (void)notification;
    g_focus_changed = true;
}

- (void)windowDidResignKey:(NSNotification*)notification
{
    (void)notification;
    g_focus_changed = true;
}

@end

static RoRCocoaDelegate* s_delegate = nil;

// ---------------------------------------------------------------------------------------------

static void BuildMainMenu()
{
    NSString* app_name = @"Rigs of Rods";

    NSMenu* main_menu = [[[NSMenu alloc] init] autorelease];

    // --- Application menu (About / Hide / Quit) ---
    NSMenuItem* app_item = [[[NSMenuItem alloc] init] autorelease];
    [main_menu addItem:app_item];

    NSMenu* app_menu = [[[NSMenu alloc] initWithTitle:app_name] autorelease];
    [app_menu addItemWithTitle:[@"About " stringByAppendingString:app_name]
                        action:@selector(orderFrontStandardAboutPanel:)
                 keyEquivalent:@""];
    [app_menu addItem:[NSMenuItem separatorItem]];
    [app_menu addItemWithTitle:[@"Hide " stringByAppendingString:app_name]
                        action:@selector(hide:)
                 keyEquivalent:@"h"];
    NSMenuItem* hide_others = [app_menu addItemWithTitle:@"Hide Others"
                                                  action:@selector(hideOtherApplications:)
                                           keyEquivalent:@"h"];
    [hide_others setKeyEquivalentModifierMask:(NSEventModifierFlagOption | NSEventModifierFlagCommand)];
    [app_menu addItemWithTitle:@"Show All"
                        action:@selector(unhideAllApplications:)
                 keyEquivalent:@""];
    [app_menu addItem:[NSMenuItem separatorItem]];
    [app_menu addItemWithTitle:[@"Quit " stringByAppendingString:app_name]
                        action:@selector(terminate:)
                 keyEquivalent:@"q"];
    [app_item setSubmenu:app_menu];

    // --- Window menu ---
    NSMenuItem* window_item = [[[NSMenuItem alloc] init] autorelease];
    [main_menu addItem:window_item];

    NSMenu* window_menu = [[[NSMenu alloc] initWithTitle:@"Window"] autorelease];
    [window_menu addItemWithTitle:@"Minimize"
                           action:@selector(performMiniaturize:)
                    keyEquivalent:@"m"];
    [window_menu addItemWithTitle:@"Zoom"
                           action:@selector(performZoom:)
                    keyEquivalent:@""];
    [window_item setSubmenu:window_menu];

    [NSApp setMainMenu:main_menu];
    [NSApp setWindowsMenu:window_menu];
}

namespace RoR {
namespace MacOS {

void InitApplication()
{
    @autoreleasepool
    {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

        if (s_delegate == nil)
        {
            s_delegate = [[RoRCocoaDelegate alloc] init]; // intentionally lives for the whole process
        }
        [NSApp setDelegate:s_delegate];

        BuildMainMenu();

        // We run our own loop (see PumpEvents) instead of [NSApp run], so finish launching by hand.
        [NSApp finishLaunching];
        [NSApp activateIgnoringOtherApps:YES];
    }
}

void AttachToWindow(void* ns_window)
{
    if (ns_window == nullptr)
    {
        return;
    }

    @autoreleasepool
    {
        NSWindow* window = (NSWindow*)ns_window;
        [window setDelegate:s_delegate];
        [window setAcceptsMouseMovedEvents:YES]; // OIS's Cocoa mouse needs move events
        [window setReleasedWhenClosed:NO];
        [window setStyleMask:([window styleMask]
                              | NSWindowStyleMaskClosable
                              | NSWindowStyleMaskMiniaturizable)];
        [window makeKeyAndOrderFront:nil];
    }
}

void PumpEvents()
{
    @autoreleasepool
    {
        for (;;)
        {
            NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                                untilDate:[NSDate distantPast]
                                                   inMode:NSDefaultRunLoopMode
                                                  dequeue:YES];
            if (event == nil)
            {
                break;
            }
            [NSApp sendEvent:event];
        }
        [NSApp updateWindows];
    }
}

bool ConsumeQuitRequest()
{
    const bool result = g_quit_requested;
    g_quit_requested = false;
    return result;
}

bool ConsumeResizeEvent()
{
    const bool result = g_resize_pending;
    g_resize_pending = false;
    return result;
}

bool ConsumeFocusChange()
{
    const bool result = g_focus_changed;
    g_focus_changed = false;
    return result;
}

std::string GetBundleResourcesDir()
{
    @autoreleasepool
    {
        NSBundle* bundle = [NSBundle mainBundle];
        NSString* bundle_path = [bundle bundlePath];
        if (bundle_path == nil || ![bundle_path hasSuffix:@".app"])
        {
            return std::string(); // not inside an .app (e.g. run from the build directory)
        }
        NSString* resources = [bundle resourcePath];
        if (resources == nil)
        {
            return std::string();
        }
        return std::string([resources UTF8String]);
    }
}

void ShowAlert(const std::string& title, const std::string& text, bool is_info)
{
    if (![NSThread isMainThread])
    {
        return; // runModal must run on the main thread; the message was already printed to stdout/log
    }

    @autoreleasepool
    {
        [NSApplication sharedApplication];

        NSString* ns_title = [NSString stringWithUTF8String:title.c_str()];
        NSString* ns_text  = [NSString stringWithUTF8String:text.c_str()];

        NSAlert* alert = [[[NSAlert alloc] init] autorelease];
        [alert setMessageText:(ns_title != nil ? ns_title : @"Rigs of Rods")];
        [alert setInformativeText:(ns_text != nil ? ns_text : @"")];
        [alert setAlertStyle:(is_info ? NSAlertStyleInformational : NSAlertStyleCritical)];
        [alert addButtonWithTitle:@"OK"];

        [NSApp activateIgnoringOtherApps:YES];
        [alert runModal];
    }
}

} // namespace MacOS
} // namespace RoR
