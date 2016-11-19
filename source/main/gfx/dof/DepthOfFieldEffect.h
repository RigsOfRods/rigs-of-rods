/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

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

// "Depth of Field" demo for Ogre
// Copyright (C) 2006  Christian Lindequist Larsen
//
// This code is in the public domain. You may do whatever you want with it.

#pragma once

#include <OgrePrerequisites.h>
#include <OgreCompositorInstance.h>
#include <OgreRenderTargetListener.h>
#include <OgreFrameListener.h>
#include <OgreRenderQueue.h>

#include "RoRPrerequisites.h"

class Lens;

class DepthOfFieldEffect : public Ogre::CompositorInstance::Listener,
    public Ogre::RenderTargetListener,
    public Ogre::RenderQueue::RenderableListener,
    public ZeroedMemoryAllocator
{
public:

    DepthOfFieldEffect();
    ~DepthOfFieldEffect();

    float getNearDepth() const { return mNearDepth; }
    float getFocalDepth() const { return mFocalDepth; }
    float getFarDepth() const { return mFarDepth; }
    void setFocalDepths(float nearDepth, float focalDepth, float farDepth);
    float getFarBlurCutoff() const { return mFarBlurCutoff; }
    void setFarBlurCutoff(float cutoff);
    bool getEnabled() const;
    void setEnabled(bool value);

private:

    // Implementation of Ogre::CompositorInstance::Listener
    virtual void notifyMaterialSetup(Ogre::uint32 passId, Ogre::MaterialPtr& material);

    // Implementation of Ogre::RenderTargetListener
    virtual void preViewportUpdate(const Ogre::RenderTargetViewportEvent& evt);
    virtual void postViewportUpdate(const Ogre::RenderTargetViewportEvent& evt);

    // Implementation of Ogre::RenderQueue::RenderableListener
    virtual bool renderableQueued(Ogre::Renderable* rend, Ogre::uint8 groupID,
        Ogre::ushort priority, Ogre::Technique** ppTech, Ogre::RenderQueue* pQueue);

    int mWidth;
    int mHeight;

    static const int BLUR_DIVISOR;

    enum PassId
    {
        BlurPass,
        OutputPass
    };

    Ogre::Viewport* mDepthViewport;
    Ogre::RenderTexture* mDepthTarget;
    Ogre::TexturePtr mDepthTexture;
    Ogre::MaterialPtr mDepthMaterial;
    Ogre::Technique* mDepthTechnique;
    Ogre::CompositorInstance* mCompositor;

    float mNearDepth;
    float mFocalDepth;
    float mFarDepth;
    float mFarBlurCutoff;

    void createDepthRenderTexture();
    void destroyDepthRenderTexture();
    //	void createCompositor();
    //	void destroyCompositor();
    void addCompositor();
    void removeCompositor();
};

class DOFManager : public Ogre::FrameListener, public ZeroedMemoryAllocator
{
public:

    DOFManager();
    ~DOFManager();

    void setEnabled(bool enabled);
    bool getEnabled();

    // controls
    enum FocusMode
    {
        Auto,
        Manual,
        Pinhole
    };

    void Aperture(float delta);
    void moveFocus(float delta);
    void setAperture(float f);
    void setAutoSpeed(float f);
    void setFocus(float f);
    void setFocusMode(int mode) { mFocusMode = (FocusMode)mode; }
    void setLensFOV(Ogre::Radian fov);
    void setZoom(float f);
    void zoomView(float delta);

protected:

    virtual bool frameStarted(const Ogre::FrameEvent& evt);

    void cleanup();

    DepthOfFieldEffect* mDepthOfFieldEffect;
    FocusMode mFocusMode;
    Lens* mLens;
    Ogre::RaySceneQuery* mRaySceneQuery;
    Ogre::Real targetFocalDistance;
    Ogre::SceneNode* debugNode;
    float mAutoSpeed;
    float mAutoTime;
};
