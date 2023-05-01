
// -------------------------------------------
// OGRE-IMGUI bindings
// See file 'README-OgreImGui.txt' for details
// -------------------------------------------

/*
    This source file is part of Rigs of Rods
    Copyright 2016-2020 Petr Ohlidal

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "OgreImGuiOverlay.h" // RoR's ported version

#include <imgui.h>
#include <Ogre.h>
#include <OISMouse.h>
#include <OISKeyboard.h>
#include <memory>

// DearIMGUI math functions, copypasted from <imgui_internal.h>
static inline ImVec2 operator*(const ImVec2& lhs, const float rhs)              { return ImVec2(lhs.x*rhs, lhs.y*rhs); }
static inline ImVec2 operator/(const ImVec2& lhs, const float rhs)              { return ImVec2(lhs.x/rhs, lhs.y/rhs); }
static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs)            { return ImVec2(lhs.x+rhs.x, lhs.y+rhs.y); }
static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs)            { return ImVec2(lhs.x-rhs.x, lhs.y-rhs.y); }
static inline ImVec2 operator*(const ImVec2& lhs, const ImVec2& rhs)            { return ImVec2(lhs.x*rhs.x, lhs.y*rhs.y); }
static inline ImVec2 operator/(const ImVec2& lhs, const ImVec2& rhs)            { return ImVec2(lhs.x/rhs.x, lhs.y/rhs.y); }
static inline ImVec2& operator+=(ImVec2& lhs, const ImVec2& rhs)                { lhs.x += rhs.x; lhs.y += rhs.y; return lhs; }
static inline ImVec2& operator-=(ImVec2& lhs, const ImVec2& rhs)                { lhs.x -= rhs.x; lhs.y -= rhs.y; return lhs; }
static inline ImVec2& operator*=(ImVec2& lhs, const float rhs)                  { lhs.x *= rhs; lhs.y *= rhs; return lhs; }
static inline ImVec2& operator/=(ImVec2& lhs, const float rhs)                  { lhs.x /= rhs; lhs.y /= rhs; return lhs; }
//static inline ImVec2 ImLerp(const ImVec2& a, const ImVec2& b, float t)          { return ImVec2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t); }
//static inline ImVec4 ImLerp(const ImVec4& a, const ImVec4& b, float t)          { return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t); }


// Extra math functions
static inline ImVec4 operator*(const ImVec4& lhs, const float rhs)             { return ImVec4(lhs.x*rhs, lhs.y*rhs, lhs.z*rhs, lhs.w*rhs); }


/// DearIMGUI integration.
/// Input handling is done by injecting OIS events to ImGUI
/// Rendering is done via port of Ogre::ImGuiOverlay; this is temporary until we migrate to OGRE 1.12.x
///   however, since our OGRE version doesn't have `OverlayManager::addOverlay()`,
///   we queue it for rendering ourselves via RenderQueueListener
///   (code is shamelessly copy-pasted from OGRE)
class OgreImGui: public Ogre::RenderQueueListener
{
public:
    void Init();

    // Input-injecting functions
    void InjectMouseMoved( const OIS::MouseEvent &arg );
    void InjectMousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id );
    void InjectMouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id );
    void InjectKeyPressed( const OIS::KeyEvent &arg );
    void InjectKeyReleased( const OIS::KeyEvent &arg );

    // Ogre::RenderQueueListener
    virtual void renderQueueStarted(Ogre::uint8 queueGroupId,
        const Ogre::String& invocation, bool& skipThisInvocation) override;

private:
    std::unique_ptr<Ogre::ImGuiOverlay> m_imgui_overlay;
};
