/*
    This source file is part of Rigs of Rods
    Copyright 2013-2016 Petr Ohlidal & contributors

    For more information, see http://www.rigsofrods.com/

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

/* Based on SdkCameraMan.h from OGRE project */

/*
-----------------------------------------------------------------------------
    This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
    For the latest info, see http://www.ogre3d.org/
 
    Copyright (c) 2000-2014 Torus Knot Software Ltd
 
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
 
    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.
 
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
-----------------------------------------------------------------------------
 */

/** 
    @file
    @date   07/2014
*/

#pragma once

#include "RigDef_Prerequisites.h"
#include "RigEditor_Types.h"
#include "RoRPrerequisites.h"

#include <OgreMatrix4.h>

namespace RoR
{

namespace RigEditor
{

/**
* Handles user view camera (scrolling, zooming etc...)
*/
class CameraHandler
{
    public:

    enum CameraStyle   // enumerator values for different styles of camera movement
    {
        CS_FREELOOK = 0,
        CS_ORBIT,
        CS_MANUAL,
    };
    
    CameraHandler(Ogre::Camera* cam);

    /** Sets the target we will revolve around. Only applies for orbit style. */
    void SetOrbitTarget(Ogre::SceneNode* target);

    Ogre::SceneNode* GetOrbitTarget()
    {
        return mTarget;
    }

    /*-----------------------------------------------------------------------------
    | Sets the spatial offset from the target. Only applies for orbit style.
    -----------------------------------------------------------------------------*/
    void SetYawPitchDist(Ogre::Radian yaw, Ogre::Radian pitch, Ogre::Real dist);

    /*-----------------------------------------------------------------------------
    | Sets the camera's top speed. Only applies for free-look style.
    -----------------------------------------------------------------------------*/
    void SetTopSpeed(Ogre::Real topSpeed)
    {
        mTopSpeed = topSpeed;
    }

    Ogre::Real GetTopSpeed()
    {
        return mTopSpeed;
    }

    void SetOrthoZoomRatio(float ortho_zoom_ratio)
    {
        m_ortho_zoom_ratio = ortho_zoom_ratio;
    }

    void ToggleOrtho();

    void UpdateOrthoZoom();

    /*-----------------------------------------------------------------------------
    | Sets the movement style of our camera man.
    -----------------------------------------------------------------------------*/
    void SetStyle(CameraStyle style);

    CameraStyle GetStyle()
    {
        return mStyle;
    }

    /*-----------------------------------------------------------------------------
    | Manually stops the camera when in free-look mode.
    -----------------------------------------------------------------------------*/
    virtual void ManualStop();

    void Update(float delta_time_seconds);

    /**
    * @return True if camera moved/zoomed.
    */
    bool InjectMouseMove(bool do_orbit, int x_rel, int y_rel, int wheel_rel);

    void LookInDirection(Ogre::Vector3 const & direction);

    void TopView(bool inverted);

#if 0 // Doesn't match RigEditor's event scheme. Left here for reference

    /*-----------------------------------------------------------------------------
    | Processes key presses for free-look style movement.
    -----------------------------------------------------------------------------*/
    virtual void injectKeyDown(const OIS::KeyEvent& evt)
    {
        if (mStyle == CS_FREELOOK)
        {
            if (evt.key == OIS::KC_W || evt.key == OIS::KC_UP) mGoingForward = true;
            else if (evt.key == OIS::KC_S || evt.key == OIS::KC_DOWN) mGoingBack = true;
            else if (evt.key == OIS::KC_A || evt.key == OIS::KC_LEFT) mGoingLeft = true;
            else if (evt.key == OIS::KC_D || evt.key == OIS::KC_RIGHT) mGoingRight = true;
            else if (evt.key == OIS::KC_PGUP) mGoingUp = true;
            else if (evt.key == OIS::KC_PGDOWN) mGoingDown = true;
            else if (evt.key == OIS::KC_LSHIFT) mFastMove = true;
        }
    }

    /*-----------------------------------------------------------------------------
    | Processes key releases for free-look style movement.
    -----------------------------------------------------------------------------*/
    virtual void injectKeyUp(const OIS::KeyEvent& evt)
    {
        if (mStyle == CS_FREELOOK)
        {
            if (evt.key == OIS::KC_W || evt.key == OIS::KC_UP) mGoingForward = false;
            else if (evt.key == OIS::KC_S || evt.key == OIS::KC_DOWN) mGoingBack = false;
            else if (evt.key == OIS::KC_A || evt.key == OIS::KC_LEFT) mGoingLeft = false;
            else if (evt.key == OIS::KC_D || evt.key == OIS::KC_RIGHT) mGoingRight = false;
            else if (evt.key == OIS::KC_PGUP) mGoingUp = false;
            else if (evt.key == OIS::KC_PGDOWN) mGoingDown = false;
            else if (evt.key == OIS::KC_LSHIFT) mFastMove = false;
        }
    }


    /*-----------------------------------------------------------------------------
    | Processes mouse presses. Only applies for orbit style.
    | Left button is for orbiting, and right button is for zooming.
    -----------------------------------------------------------------------------*/
#if (OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS) || (OGRE_PLATFORM == OGRE_PLATFORM_ANDROID)
    virtual void injectMouseDown(const OIS::MultiTouchEvent& evt)
    {
        if (mStyle == CS_ORBIT)
        {
            mOrbiting = true;
        }
    }
#else
    virtual void injectMouseDown(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
    {
        if (mStyle == CS_ORBIT)
        {
            if (id == OIS::MB_Left) mOrbiting = true;
            else if (id == OIS::MB_Right) mZooming = true;
        }
    }
#endif

    /*-----------------------------------------------------------------------------
    | Processes mouse releases. Only applies for orbit style.
    | Left button is for orbiting, and right button is for zooming.
    -----------------------------------------------------------------------------*/
#if (OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS) || (OGRE_PLATFORM == OGRE_PLATFORM_ANDROID)
    virtual void injectMouseUp(const OIS::MultiTouchEvent& evt)
    {
        if (mStyle == CS_ORBIT)
        {
            mOrbiting = false;
        }
    }
#else
    virtual void injectMouseUp(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
    {
        if (mStyle == CS_ORBIT)
        {
            if (id == OIS::MB_Left) mOrbiting = false;
            else if (id == OIS::MB_Right) mZooming = false;
        }
    }
#endif

#endif // End of <#if 0> section

    bool ConvertWorldToScreenPosition(
        Ogre::Vector3 const & world_position,
        Vector2int & out_screen_position
    );

    float GetCameraTargetDistance();

    Ogre::Vector3 ConvertScreenToWorldPosition(Vector2int const & screen_pos, Ogre::Vector3 const & pivot);

protected:

    Ogre::Camera* mCamera;
    CameraStyle mStyle;
    Ogre::SceneNode* mTarget;
    bool mZooming;
    Ogre::Real mTopSpeed;
    Ogre::Vector3 mVelocity;
    bool mGoingForward;
    bool mGoingBack;
    bool mGoingLeft;
    bool mGoingRight;
    bool mGoingUp;
    bool mGoingDown;
    bool mFastMove;
    float m_ortho_zoom_ratio;
    Ogre::Matrix4     m_inverse_projection_matrix;
    bool              m_inverse_projection_matrix_dirty;
    Ogre::Matrix4     m_inverse_view_matrix;
    bool              m_inverse_view_matrix_dirty;
};

} // namespace RigEditor

} // namespace RoR
