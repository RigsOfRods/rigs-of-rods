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

/// @file   MacOS.h
/// @brief  Cocoa glue for the macOS port.
///
/// Why this exists: OGRE 1.11's `OgreBites::WindowEventUtilities::messagePump()` has no macOS
/// implementation, and OGRE's Cocoa render window installs no delegate. Without this layer the
/// game would never receive keyboard/mouse events (OIS's Cocoa backend is fed by NSApp's event
/// dispatch), the close button / Cmd+Q would do nothing, and window resizes would go unnoticed.
///
/// This header is deliberately free of Objective-C and OGRE types so it can be included anywhere.

#pragma once

#include <string>

namespace RoR {
namespace MacOS {

/// Creates the NSApplication, the menu bar (About / Hide / Quit, Window), and activates the app.
/// Call once, before the render window is created.
void InitApplication();

/// Hooks our delegate (close / resize / focus) onto the NSWindow that OGRE created.
/// @param ns_window  The value OGRE returns for `renderWindow->getCustomAttribute("WINDOW", &ptr)`.
void AttachToWindow(void* ns_window);

/// Drains the Cocoa event queue without blocking. Call once per frame on the main thread.
void PumpEvents();

/// True (once) after the user clicked the close button or chose Quit / pressed Cmd+Q.
bool ConsumeQuitRequest();

/// True (once) after the window was resized or moved to a display with different scaling.
bool ConsumeResizeEvent();

/// True (once) after the window gained or lost keyboard focus.
bool ConsumeFocusChange();

/// Path of `<App>.app/Contents/Resources`, or an empty string if not running from an .app bundle
/// (e.g. when launched straight from a build directory).
std::string GetBundleResourcesDir();

/// Shows a native modal alert. Safe to call before InitApplication(). Ignored off the main thread.
void ShowAlert(const std::string& title, const std::string& text, bool is_info);

} // namespace MacOS
} // namespace RoR
