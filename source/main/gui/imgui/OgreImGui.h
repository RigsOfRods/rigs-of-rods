
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
#include <OgreCommon.h>
#include <OISMouse.h>
#include <OISKeyboard.h>

#include <OgreRenderQueueListener.h>
#include <OgreTexture.h>
#include "OgrePrerequisites.h"
#include "OgreRenderable.h"
#include <OgreRenderOperation.h>

/// ImGui rendering for OGRE engine
/// Usage:
///  1. Call `Init()` after OGRE was started
///  2. Call `StartRendering()` when you're ready. This attaches OgreImGui as render queue listener. IMPORTANT: see next step.
///  3. Call `NewFrame()` before each render, otherwise IMGUI will crash!! This is because IMGUI rendering is done automatically by `OgreImGui` and expects `NewFrame()` to be called.
///  4. Use any MyGUI functions to create your GUI; You need to use `Inject*()` functions to handle inputs.
///  5. Call `StopRendering()` to detach OgreImGui from render queue.
class OgreImGui : public Ogre::RenderQueueListener
{
public:
    OgreImGui(): mLastRenderedFrame(-1), mSceneMgr(nullptr) {}

    void Init();
    void StartRendering(Ogre::SceneManager* scene_mgr);
    void StopRendering();
    void NewFrame(float deltaTime,const Ogre::Rect & windowRect, bool ctrl, bool alt, bool shift);

    //inherited from RenderQueueListener
    virtual void renderQueueEnded(Ogre::uint8 queueGroupId, const Ogre::String& invocation,bool& repeatThisInvocation) override;

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

        void updateVertexData(ImDrawData* data,unsigned int cmdIndex);
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
    void updateVertexData();

    std::list<ImGUIRenderable*> mRenderables;
    Ogre::Pass*                 mPass;
    int                         mLastRenderedFrame;
    Ogre::TexturePtr            mFontTex;
    bool                        mFrameEnded;
    Ogre::SceneManager*         mSceneMgr;

};

