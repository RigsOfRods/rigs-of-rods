// SPDX-License-Identifier: GPL-3.0-only
#include "MacApplication.h"
#import <AppKit/AppKit.h>
#include <stdexcept>

namespace {
struct NativeFlags { bool quit = false; bool reset = false; };
NativeFlags* active_owner = nullptr;
void RequireMainThread()
{
    if (![NSThread isMainThread]) throw std::runtime_error("Cocoa lifecycle requires the main thread");
}
}

@interface RoRCocoaDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>
{
@public
    NativeFlags* flags;
}
- (void)lostFocus:(NSNotification*)notification;
@end
@implementation RoRCocoaDelegate
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender
{
    flags->quit = true;
    return NSTerminateCancel;
}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender { return NO; }
- (BOOL)windowShouldClose:(NSWindow*)sender { flags->quit = true; return NO; }
- (void)lostFocus:(NSNotification*)notification { flags->reset = true; }
@end

namespace RoR {
MacAutoreleasePool::MacAutoreleasePool() : m_pool([[NSAutoreleasePool alloc] init]) {}
MacAutoreleasePool::~MacAutoreleasePool() { [static_cast<NSAutoreleasePool*>(m_pool) drain]; }

struct MacApplication::Impl
{
    MacAutoreleasePool bootstrap;
    NativeFlags flags;
    NSApplication* app = nil; // Borrowed singleton; never released or terminated here.
    NSWindow* window = nil; // Borrowed Ogre window; detach before renderer teardown.
    RoRCocoaDelegate* delegate = nil;
    NSMenu* menu = nil;
    bool observing_app = false;
    bool observing_window = false;
    bool launched = false;
    bool pumping = false;
    bool detached = false;

    void DetachWindow() noexcept
    {
        detached = true;
        if (observing_window)
            [[NSNotificationCenter defaultCenter] removeObserver:delegate name:NSWindowDidResignKeyNotification object:window];
        observing_window = false;
        if (window && [window delegate] == delegate) [window setDelegate:nil];
        window = nil;
    }
    ~Impl()
    {
        DetachWindow();
        if (observing_app)
            [[NSNotificationCenter defaultCenter] removeObserver:delegate name:NSApplicationDidResignActiveNotification object:app];
        if (delegate && [app delegate] == delegate) [app setDelegate:nil];
        if (menu && [app mainMenu] == menu) [app setMainMenu:nil];
        [menu release];
        [delegate release];
        if (active_owner == &flags) active_owner = nullptr;
    }
};

MacApplication::MacApplication() noexcept = default;
MacApplication::~MacApplication() = default;
void MacApplication::Activate()
{
    RequireMainThread();
    if (m_impl || active_owner) throw std::runtime_error("Cocoa application already owned");
    std::unique_ptr<Impl> state(new Impl);
    state->app = [NSApplication sharedApplication];
    if ([state->app isRunning] || [state->app delegate])
        throw std::runtime_error("Standalone Cocoa lifecycle conflicts with an existing application owner");
    if (![state->app setActivationPolicy:NSApplicationActivationPolicyRegular])
        throw std::runtime_error("Cocoa regular activation policy was rejected");
    state->delegate = [[RoRCocoaDelegate alloc] init];
    state->delegate->flags = &state->flags;
    active_owner = &state->flags;
    [state->app setDelegate:state->delegate];
    [[NSNotificationCenter defaultCenter] addObserver:state->delegate selector:@selector(lostFocus:)
        name:NSApplicationDidResignActiveNotification object:state->app];
    state->observing_app = true;
    if (![state->app mainMenu])
    {
        state->menu = [[NSMenu alloc] initWithTitle:@""];
        NSMenuItem* item = [[[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""] autorelease];
        NSMenu* submenu = [[[NSMenu alloc] initWithTitle:@""] autorelease];
        [submenu addItemWithTitle:@"Quit Rigs of Rods" action:@selector(terminate:) keyEquivalent:@"q"];
        [item setSubmenu:submenu];
        [state->menu addItem:item];
        [state->app setMainMenu:state->menu];
    }
    // Per-owner single launch; Activate cannot be repeated on this owner.
    [state->app finishLaunching];
    state->launched = true;
    m_impl = std::move(state);
}
void MacApplication::AttachWindow(void* native_window)
{
    RequireMainThread();
    if (!m_impl || !m_impl->launched || m_impl->detached || m_impl->window)
        throw std::runtime_error("Invalid Cocoa window attachment lifetime");
    NSWindow* window = static_cast<NSWindow*>(native_window);
    if (!window || ![window isKindOfClass:[NSWindow class]] || [window delegate])
        throw std::runtime_error("Cocoa window is invalid or already has a delegate");
    m_impl->window = window;
    [window setDelegate:m_impl->delegate];
    [[NSNotificationCenter defaultCenter] addObserver:m_impl->delegate selector:@selector(lostFocus:)
        name:NSWindowDidResignKeyNotification object:window];
    m_impl->observing_window = true;
}
void MacApplication::DetachWindow() noexcept { if (m_impl) m_impl->DetachWindow(); }
void MacApplication::Pump()
{
    RequireMainThread();
    if (!m_impl || !m_impl->launched || m_impl->detached || m_impl->pumping) return;
    m_impl->pumping = true;
    struct Guard { bool& pumping; ~Guard() { pumping = false; } } guard{m_impl->pumping};
    NSEvent* event;
    while ((event = [m_impl->app nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast]
        inMode:NSDefaultRunLoopMode dequeue:YES]))
        [m_impl->app sendEvent:event];
    [m_impl->app updateWindows];
}
bool MacApplication::QuitRequested() const noexcept { return m_impl && m_impl->flags.quit; }
bool MacApplication::ConsumeInputReset() noexcept
{
    if (!m_impl) return false;
    bool reset = m_impl->flags.reset;
    m_impl->flags.reset = false;
    return reset;
}
} // namespace RoR
