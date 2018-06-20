
// -------------------------------------------
// OGRE-IMGUI bindings
// See file 'README-OgreImGui.txt' for details
// -------------------------------------------

/*
    This source file is part of Rigs of Rods
    Copyright 2016-2017 Petr Ohlidal & contributors

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

#include <imgui.h>
#include <OgreString.h>
#include <OgreCommon.h>
#include <OISMouse.h>
#include <OISKeyboard.h>

#include <OgreRenderQueueListener.h>
#include <OgreTexture.h>
#include "OgrePrerequisites.h"
#include "OgreRenderable.h"
#include <OgreRenderOperation.h>

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

/// DearIMGUI rendering for OGRE engine; Usage:
///  1. Call `Init()` after OGRE was started
///  2. Call `NewFrame()` before each render, otherwise IMGUI will crash.
///  3. Use `Inject*()` functions to handle inputs.
///  4. Use any ImGui*() functions to create your GUI.
///  5. Call `Render()` to render the GUI.
class OgreImGui
{
public:
    OgreImGui(): mSceneMgr(nullptr) {}

    void Init(Ogre::SceneManager* scenemgr);
    void NewFrame(float deltaTime, float vpWidth, float vpHeight, bool ctrl, bool alt, bool shift);
    void Render();

    // Input-injecting functions
    void InjectMouseMoved( const OIS::MouseEvent &arg );
    void InjectMousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id );
    void InjectMouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id );
    void InjectKeyPressed( const OIS::KeyEvent &arg );
    void InjectKeyReleased( const OIS::KeyEvent &arg );

private:

    class ImGUIRenderable : public Ogre::Renderable
    {
    public:
        ImGUIRenderable();
        virtual ~ImGUIRenderable();

        void updateVertexData(const ImDrawVert* vtxBuf, const ImDrawIdx* idxBuf, unsigned int vtxCount, unsigned int idxCount);
        Ogre::Real getSquaredViewDepth(const Ogre::Camera* cam) const   { (void)cam; return 0; }

        void setMaterial( const Ogre::String& matName );
        void setMaterial(const Ogre::MaterialPtr & material);
        virtual const Ogre::MaterialPtr& getMaterial(void) const override;
        virtual void getWorldTransforms( Ogre::Matrix4* xform ) const override;
        virtual void getRenderOperation( Ogre::RenderOperation& op ) override;
        virtual const Ogre::LightList& getLights(void) const override;

        int mVertexBufferSize;
        int mIndexBufferSize;

    private:
        void initImGUIRenderable(void);

        Ogre::MaterialPtr mMaterial;
        Ogre::RenderOperation mRenderOp;
    };

    void createFontTexture();
    void createMaterial();

    Ogre::Pass*                 mPass;
    Ogre::TexturePtr            mFontTex;
    Ogre::SceneManager*         mSceneMgr;

};
