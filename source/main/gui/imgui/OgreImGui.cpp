
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

#include "OgreImGui.h"

#include "Actor.h"
#include "AppContext.h"
#include "ContentManager.h"
#include "OgreImGuiOverlay.h"

#include <imgui.h>
#include <Ogre.h>

using namespace RoR;

// Based on OGRE 13.5.3, file Components\Bites\src\OgreImGuiInputListener.cpp
// returning int instead of ImGuiKey because our outdated DearIMGUI doesn't have `ImGuiKey_None`
static int SDL2KeycodeToImGuiKey(int keycode)
{
    switch (keycode)
    {
    case '\t': return ImGuiKey_Tab;
    case SDLK_LEFT: return ImGuiKey_LeftArrow;
    case SDLK_RIGHT: return ImGuiKey_RightArrow;
    case SDLK_UP: return ImGuiKey_UpArrow;
    case SDLK_DOWN: return ImGuiKey_DownArrow;
    case SDLK_PAGEUP: return ImGuiKey_PageUp;
    case SDLK_PAGEDOWN: return ImGuiKey_PageDown;
    case SDLK_HOME: return ImGuiKey_Home;
    case SDLK_END: return ImGuiKey_End;
    case SDLK_INSERT: return ImGuiKey_Insert;
    case SDLK_DELETE: return ImGuiKey_Delete;
    case '\b': return ImGuiKey_Backspace;
    case SDLK_SPACE: return ImGuiKey_Space;
    case SDLK_RETURN: return ImGuiKey_Enter;
    case SDLK_ESCAPE: return ImGuiKey_Escape;

    case SDLK_KP_0: return ImGuiKey_KeyPad0;
    case SDLK_KP_1: return ImGuiKey_KeyPad1;
    case SDLK_KP_2: return ImGuiKey_KeyPad2;
    case SDLK_KP_3: return ImGuiKey_KeyPad3;
    case SDLK_KP_4: return ImGuiKey_KeyPad4;
    case SDLK_KP_5: return ImGuiKey_KeyPad5;
    case SDLK_KP_6: return ImGuiKey_KeyPad6;
    case SDLK_KP_7: return ImGuiKey_KeyPad7;
    case SDLK_KP_8: return ImGuiKey_KeyPad8;
    case SDLK_KP_9: return ImGuiKey_KeyPad9;

    case 'a': return ImGuiKey_A;
    case 'c': return ImGuiKey_C;
    case 'v': return ImGuiKey_V;
    case 'x': return ImGuiKey_X;
    case 'y': return ImGuiKey_Y;
    case 'z': return ImGuiKey_Z;

    case '/': return ImGuiKey_Slash; // Heh, apparently newer DearIMGUI added the same key we hacked-in :)

    }
    return -1;
}

void OgreImGui::Init()
{
    m_imgui_overlay = std::unique_ptr<Ogre::ImGuiOverlay>(new Ogre::ImGuiOverlay());

    ImGuiIO& io = ImGui::GetIO();

    // Disable 'imgui.ini' - we don't need to persist window positions.
    io.IniFilename = nullptr;

    // NOTE: Setting up `io.KeyMap` isn't possible with SDL2 because the values must be 0-512 and SDL's Keycodes are bitmasks which are much larger.

    // Load font
    m_imgui_overlay->addFont("rigsofrods/fonts/Roboto-Medium",
        ContentManager::ResourcePack::FONTS.resource_group_name);

    // Start rendering
    m_imgui_overlay->setZOrder(300);
    m_imgui_overlay->initialise(); // Build font texture
    m_imgui_overlay->show();
}

void OgreImGui::InjectMouseMoved(const OgreBites::MouseMotionEvent& arg)
{
    ImGuiIO& io = ImGui::GetIO();

    io.MousePos.x = arg.x;
    io.MousePos.y = arg.y;
}

void OgreImGui::InjectMouseWheelRolled(const OgreBites::MouseWheelEvent& arg)
{
    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheel = Ogre::Math::Clamp((float)arg.y, -1 / 3.f, 1 / 3.f);
}


// map sdl2 mouse buttons to imgui (copypasted from OgreImGuiInputListener.cpp)
static int sdl2imgui(int b)
{
    switch (b)
    {
    case 2:
        return 2;
    case 3:
        return 1;
    default:
        return b - 1;
    }
}

void OgreImGui::InjectMousePressed(const OgreBites::MouseButtonEvent& arg)
{
    ImGuiIO& io = ImGui::GetIO();
    int id = sdl2imgui(arg.button);
    if (id<5)
    {
        io.MouseDown[id] = true;
    }
}

void OgreImGui::InjectMouseReleased(const OgreBites::MouseButtonEvent& arg)
{
    ImGuiIO& io = ImGui::GetIO();
    int id = sdl2imgui(arg.button);
    if (id<5)
    {
        io.MouseDown[id] = false;
    }
}

void OgreImGui::InjectKeyPressed(const OgreBites::KeyboardEvent& arg)
{
    ImGuiIO& io = ImGui::GetIO();
    int key = SDL2KeycodeToImGuiKey(arg.keysym.sym);
    if (key != -1)
    {
        io.KeysDown[key] = true;
    }
}

void OgreImGui::InjectKeyReleased(const OgreBites::KeyboardEvent& arg)
{
    ImGuiIO& io = ImGui::GetIO();
    int key = SDL2KeycodeToImGuiKey(arg.keysym.sym);
    if (key != -1)
    {
        io.KeysDown[key] = false;
    }
}

void OgreImGui::InjectTextInput(const OgreBites::TextInputEvent& arg)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddInputCharactersUTF8(arg.chars);
}

void OgreImGui::renderQueueStarted(Ogre::uint8 queueGroupId,
        const Ogre::String& invocation, bool& skipThisInvocation)
{
    // Shamelessly copy-pasted from `Ogre::OverlaySystem::renderQueueStarted()`
    if(queueGroupId == Ogre::RENDER_QUEUE_OVERLAY)
    {
        Ogre::Viewport* vp = Ogre::Root::getSingletonPtr()->getRenderSystem()->_getViewport();
        if(vp != NULL)
        {
            Ogre::SceneManager* sceneMgr = vp->getCamera()->getSceneManager();
            if (vp->getOverlaysEnabled() && sceneMgr->_getCurrentRenderStage() != Ogre::SceneManager::IRS_RENDER_TO_TEXTURE)
            {
                // Checking `sceneMgr->_getCurrentRenderStage() == Ogre::SceneManager::IRS_RENDER_TO_TEXTURE`)
                // doesn't do the trick if the RTT is updated by calling `Ogre::RenderTarget::update()` by hand,
                // which we do frequently.
                // To compensate, we also check if the active viewport matches our screen viewport.
                Ogre::Viewport* vp_target = App::GetAppContext()->GetViewport();
                if (vp == vp_target)
                {
                    m_imgui_overlay->_findVisibleObjects(vp->getCamera(), sceneMgr->getRenderQueue(), vp);
                }
            }
        }
    }
}
