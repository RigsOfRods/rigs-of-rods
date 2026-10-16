# Standalone macOS lifecycle and ImGui 1.73 input

Desktop macOS builds use a RoR-owned AppKit launch and pump. `MacApplication`
starts inert, activates after command-line early returns and before rendering,
and attaches only OGRE's `getCustomAttribute("WINDOW", ...)` window. An existing
application owner/delegate or window delegate is rejected. Host-managed embedding
is unsupported by this standalone entry point. Existing main menus are preserved;
a minimal Quit menu is installed only when there is no main menu.

The bridge requests regular activation policy and finishes launch once per owner.
It adds no forced activation. The pinned OGRE Cocoa window still performs its own
`activateIgnoringOtherApps:` call; this contribution does not remove that behavior.
Close and application termination callbacks latch quit and cancel native immediate
destruction. Focus notifications only latch input reset. The main loop consumes
these flags through the existing shutdown message and effective-input reset.

The frame pool covers dispatch, update and render. Physics synchronizes before
native dispatch because Cocoa OIS mouse callbacks can run during `sendEvent:`.
The pump drains available default-mode events without blocking or filtering event
types, then calls `updateWindows`. There is exactly one native pump; non-macOS
builds retain Bites. There is no resize/render forwarding in native callbacks.
Native observers, delegates and the owned menu detach before the C++ owner dies;
the owner never releases NSApp or destroys the OGRE window/view/context.
A future RTSS scoped owner must be declared before `MacApplication`, so native
callbacks detach before RTSS listeners/generator cleanup. Future complete renderer
teardown must explicitly detach the native bridge before destroying its window.

## Buffered UI input

ImGui 1.73 exposes polled key/button arrays. A full native drain can put both press
and release in the same iteration, so macOS uses `ImGuiInputQueue` to preserve these
edges for UI frames. The existing AppContext callbacks still deliver game input
immediately, with the existing keyboard capture/release filtering.

The adapter copies key/text, button positions, motion and wheel values. Delivery
runs immediately before `ImGui::NewFrame`, stopping before an opposite transition
for any control already changed that frame. Independent controls can share a frame;
repeated keydown text is delivered once per callback. Later position changes also
wait after a newly delivered mouse edge, preserving the sampled click position.
Only adjacent position-only motion events coalesce; wheel increments accumulate.
Modifiers come from delivered left/right key states, never later OIS polling.
The queue preserves callback and per-control order. It cannot reconstruct original
native cross-device chronology: legacy OIS buffers keyboard callbacks but emits
mouse buttons during native dispatch.

The cap is 1,024 pending events. Overflow logs input loss, discards the backlog and
clears exposed UI state; this is recovery, not lossless delivery. Focus loss,
input destruction/recreation, GUI destruction and committed quit clear queued and
delivered state. Existing effective game input is reset when input exists. No
synthetic OIS release is added here. The separate OIS Cocoa focus fix remains
necessary to clear stale backend queues/held state. In particular, the previously
completed dependency prefix does not acquire that fix from this contribution.

OIS capture runs once per macOS UI iteration after synchronization/dispatch and
message handling, including zero simulation dt. Game action updates retain their
existing timestep condition. Committed SHUTDOWN exits before capture/UI/render.
The non-Apple input path keeps its immediate adapter and capture timing.

## Headless check and limits

Compile and run the dependency-free production-queue fixture, using the project's
C++11 standard:

```sh
c++ -std=c++11 -Wall -Wextra -Werror -Isource/main/gui/imgui \
    tests/imgui_input_queue.cpp -o /tmp/ror-imgui-input-queue
/tmp/ror-imgui-input-queue
```

It covers short presses/releases, repeated clicks/text, independent controls,
delivered modifier state, motion around deferred clicks, wheel accumulation,
reset/recreation, bounded overflow recovery and delivery without a dt condition.
On toolchains requiring an explicit libc++ SDK include path, supply the same
compiler configuration as the application. Objective-C++ must receive that
configuration too (`CMAKE_OBJCXX_FLAGS` where needed); C++ flags do not configure
another language automatically. The target sets C++11 Objective-C++, inherits the
macOS architecture/deployment settings, links AppKit, uses manual ownership and
skips the C++ PCH for the `.mm` file.

These deterministic fixtures and compilation do not establish live native event
or window behavior. No application/NSApplication/NSWindow/GPU/live-input test was
run for this contribution. Close, Command-Q, focus/first click, modal/menu/resize
tracking and visible responsiveness remain unverified. This default-mode pump does
not promise simulation progress inside AppKit nested tracking/modal loops. Retina
coordinate behavior and OIS backend focus repair are separate changes.
