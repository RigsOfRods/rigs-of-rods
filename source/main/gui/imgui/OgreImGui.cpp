
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

void OgreImGui::Init()
{
    m_imgui_overlay = std::unique_ptr<Ogre::ImGuiOverlay>(new Ogre::ImGuiOverlay());

    ImGuiIO& io = ImGui::GetIO();

    // Disable 'imgui.ini' - we don't need to persist window positions.
    io.IniFilename = nullptr;

    // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array that we will update during the application lifetime.
    io.KeyMap[ImGuiKey_Tab] = OIS::KC_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = OIS::KC_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = OIS::KC_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = OIS::KC_UP;
    io.KeyMap[ImGuiKey_DownArrow] = OIS::KC_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = OIS::KC_PGUP;
    io.KeyMap[ImGuiKey_PageDown] = OIS::KC_PGDOWN;
    io.KeyMap[ImGuiKey_Home] = OIS::KC_HOME;
    io.KeyMap[ImGuiKey_End] = OIS::KC_END;
    io.KeyMap[ImGuiKey_Delete] = OIS::KC_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = OIS::KC_BACK;
    io.KeyMap[ImGuiKey_Enter] = OIS::KC_RETURN;
    io.KeyMap[ImGuiKey_Escape] = OIS::KC_ESCAPE;
    io.KeyMap[ImGuiKey_A] = OIS::KC_A;
    io.KeyMap[ImGuiKey_C] = OIS::KC_C;
    io.KeyMap[ImGuiKey_V] = OIS::KC_V;
    io.KeyMap[ImGuiKey_X] = OIS::KC_X;
    io.KeyMap[ImGuiKey_Y] = OIS::KC_Y;
    io.KeyMap[ImGuiKey_Z] = OIS::KC_Z;
    io.KeyMap[ImGuiKey_KeyPad0] = OIS::KC_NUMPAD0;
    io.KeyMap[ImGuiKey_KeyPad1] = OIS::KC_NUMPAD1;
    io.KeyMap[ImGuiKey_KeyPad2] = OIS::KC_NUMPAD2;
    io.KeyMap[ImGuiKey_KeyPad3] = OIS::KC_NUMPAD3;
    io.KeyMap[ImGuiKey_KeyPad4] = OIS::KC_NUMPAD4;
    io.KeyMap[ImGuiKey_KeyPad5] = OIS::KC_NUMPAD5;
    io.KeyMap[ImGuiKey_KeyPad6] = OIS::KC_NUMPAD6;
    io.KeyMap[ImGuiKey_KeyPad7] = OIS::KC_NUMPAD7;
    io.KeyMap[ImGuiKey_KeyPad8] = OIS::KC_NUMPAD8;
    io.KeyMap[ImGuiKey_KeyPad9] = OIS::KC_NUMPAD9;
    io.KeyMap[ImGuiKey_Slash] = OIS::KC_SLASH;

    // Load font
    m_imgui_overlay->addFont("rigsofrods/fonts/Roboto-Medium",
        ContentManager::ResourcePack::FONTS.resource_group_name);

    // Start rendering
    m_imgui_overlay->setZOrder(300);
    m_imgui_overlay->initialise(); // Build font texture
    m_imgui_overlay->show();
}

void OgreImGui::Shutdown()
{
    m_imgui_overlay->hide();
    m_imgui_overlay = nullptr;
}

void OgreImGui::InjectMouseMoved( const OIS::MouseEvent &arg )
{

    ImGuiIO& io = ImGui::GetIO();

    io.MousePos.x = arg.state.X.abs;
    io.MousePos.y = arg.state.Y.abs;
    io.MouseWheel = Ogre::Math::Clamp((float)arg.state.Z.rel, -1/3.f, 1/3.f);
}

void OgreImGui::InjectMousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
{
    ImGuiIO& io = ImGui::GetIO();
    if (id<5)
    {
        io.MouseDown[id] = true;
    }
}

void OgreImGui::InjectMouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
{
    ImGuiIO& io = ImGui::GetIO();
    if (id<5)
    {
        io.MouseDown[id] = false;
    }
}

void OgreImGui::InjectKeyPressed( const OIS::KeyEvent &arg )
{
    ImGuiIO& io = ImGui::GetIO();
    io.KeysDown[arg.key] = true;

    if (arg.text>0)
    {
        io.AddInputCharacter((unsigned short)arg.text);
    }
}

void OgreImGui::InjectKeyReleased( const OIS::KeyEvent &arg )
{
    ImGuiIO& io = ImGui::GetIO();
    io.KeysDown[arg.key] = false;
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
