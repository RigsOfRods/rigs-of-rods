/*
    This source file is part of Rigs of Rods
    Copyright 2013-2016 Petr Ohlidal & contributors

    For more information, see http://www.rigsofrods.com/

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
    @file   RigEditor_CameraHandler.cpp
    @date   07/2014
*/

#include "RigEditor_CameraHandler.h"

#include <OgreCamera.h>
#include <OgreSceneNode.h>
#include <OgreSceneManager.h>

using namespace RoR;
using namespace RoR::RigEditor;

CameraHandler::CameraHandler(Ogre::Camera* cam)
    : mCamera(cam)
    , m_ortho_zoom_ratio(0)
    , m_inverse_projection_matrix_dirty(true)
    , m_inverse_view_matrix_dirty(true)
    , mTarget(0)
    , mZooming(false)
    , mTopSpeed(150)
    , mVelocity(Ogre::Vector3::ZERO)
    , mGoingForward(false)
    , mGoingBack(false)
    , mGoingLeft(false)
    , mGoingRight(false)
    , mGoingUp(false)
    , mGoingDown(false)
    , mFastMove(false)
{
    SetStyle(CS_ORBIT);
}

void CameraHandler::SetOrbitTarget(Ogre::SceneNode* target)
{
    if (target != mTarget)
    {
        mTarget = target;
        if(target)
        {
            SetYawPitchDist(Ogre::Degree(0), Ogre::Degree(15), 150);
            mCamera->setAutoTracking(true, mTarget);
        }
        else
        {
            mCamera->setAutoTracking(false);
        }
        m_inverse_view_matrix_dirty = true;
    }
}

void CameraHandler::ToggleOrtho()
{
    if (mCamera->getProjectionType() == Ogre::PT_PERSPECTIVE)
    {
        mCamera->setProjectionType(Ogre::PT_ORTHOGRAPHIC);
        UpdateOrthoZoom();
    }
    else
    {
        mCamera->setProjectionType(Ogre::PT_PERSPECTIVE);
    }
    m_inverse_projection_matrix_dirty = true;
}

float CameraHandler::GetCameraTargetDistance()
{
    return (mCamera->getViewMatrix() * mTarget->getPosition()).z * -1;
}

void CameraHandler::UpdateOrthoZoom()
{
    mCamera->setOrthoWindowWidth(GetCameraTargetDistance() * m_ortho_zoom_ratio);
}

void CameraHandler::SetYawPitchDist(Ogre::Radian yaw, Ogre::Radian pitch, Ogre::Real dist)
{
    mCamera->setPosition(mTarget->_getDerivedPosition());
    mCamera->setOrientation(mTarget->_getDerivedOrientation());
    mCamera->yaw(yaw);
    mCamera->pitch(-pitch);
    mCamera->moveRelative(Ogre::Vector3(0, 0, dist));
}

void CameraHandler::SetStyle(CameraStyle style)
{
    if (mStyle != CS_ORBIT && style == CS_ORBIT)
    {
        SetOrbitTarget(mTarget ? mTarget : mCamera->getSceneManager()->getRootSceneNode());
        mCamera->setFixedYawAxis(true);
        ManualStop();
        SetYawPitchDist(Ogre::Degree(0), Ogre::Degree(15), 150);
    }
    else if (mStyle != CS_FREELOOK && style == CS_FREELOOK)
    {
        mCamera->setAutoTracking(false);
        mCamera->setFixedYawAxis(true);
    }
    else if (mStyle != CS_MANUAL && style == CS_MANUAL)
    {
        mCamera->setAutoTracking(false);
        ManualStop();
    }
    mStyle = style;

}

void CameraHandler::ManualStop()
{
    if (mStyle == CS_FREELOOK)
    {
        mGoingForward = false;
        mGoingBack = false;
        mGoingLeft = false;
        mGoingRight = false;
        mGoingUp = false;
        mGoingDown = false;
        mVelocity = Ogre::Vector3::ZERO;
    }
}

//void CameraHandler::Update(float delta_time_seconds)
//{
//    if (mStyle == CS_FREELOOK)
//    {
//        // build our acceleration vector based on keyboard input composite
//        Ogre::Vector3 accel = Ogre::Vector3::ZERO;
//        if (mGoingForward) accel += mCamera->getDirection();
//        if (mGoingBack) accel -= mCamera->getDirection();
//        if (mGoingRight) accel += mCamera->getRight();
//        if (mGoingLeft) accel -= mCamera->getRight();
//        if (mGoingUp) accel += mCamera->getUp();
//        if (mGoingDown) accel -= mCamera->getUp();
//
//        // if accelerating, try to reach top speed in a certain time
//        Ogre::Real topSpeed = mFastMove ? mTopSpeed * 20 : mTopSpeed;
//        if (accel.squaredLength() != 0)
//        {
//            accel.normalise();
//            mVelocity += accel * topSpeed * delta_time_seconds * 10;
//        }
//        // if not accelerating, try to stop in a certain time
//        else mVelocity -= mVelocity * delta_time_seconds * 10;
//
//        Ogre::Real tooSmall = std::numeric_limits<Ogre::Real>::epsilon();
//
//        // keep camera velocity below top speed and above epsilon
//        if (mVelocity.squaredLength() > topSpeed * topSpeed)
//        {
//            mVelocity.normalise();
//            mVelocity *= topSpeed;
//        }
//        else if (mVelocity.squaredLength() < tooSmall * tooSmall)
//            mVelocity = Ogre::Vector3::ZERO;
//
//        if (mVelocity != Ogre::Vector3::ZERO) mCamera->move(mVelocity * delta_time_seconds);
//    }
//}

bool CameraHandler::InjectMouseMove(bool do_orbit, int x_rel, int y_rel, int wheel_rel)
{
    if (mStyle == CS_ORBIT)
    {
        Ogre::Real dist = GetCameraTargetDistance();

        if (do_orbit)   // yaw around the target, and pitch locally
        {
            mCamera->setPosition(mTarget->_getDerivedPosition());

            mCamera->yaw(Ogre::Degree(-x_rel * 0.25f));
            mCamera->pitch(Ogre::Degree(-y_rel * 0.25f));

            mCamera->moveRelative(Ogre::Vector3(0, 0, dist));
            m_inverse_view_matrix_dirty = true;
            return true;

            // don't let the camera go over the top or around the bottom of the target
        }
        else if (mZooming)  // (mouse-move zooming) move the camera toward or away from the target
        {
            // the further the camera is, the faster it moves
            mCamera->moveRelative(Ogre::Vector3(0, 0, y_rel * 0.004f * dist));
            m_inverse_view_matrix_dirty = true;
            return true;
        }
        else if (wheel_rel != 0)  // move the camera toward or away from the target
        {
            // the further the camera is, the faster it moves
            mCamera->moveRelative(Ogre::Vector3(0, 0, -wheel_rel * 0.0008f * dist));

            if (mCamera->getProjectionType() == Ogre::PT_ORTHOGRAPHIC)
            {
                UpdateOrthoZoom();
            }
            m_inverse_view_matrix_dirty = true;
            return true;
        }
    }
    else if (mStyle == CS_FREELOOK)
    {
        mCamera->yaw(Ogre::Degree(-x_rel * 0.15f));
        mCamera->pitch(Ogre::Degree(-y_rel * 0.15f));
    }

    return false;
}

void CameraHandler::LookInDirection(Ogre::Vector3 const & direction)
{
    Ogre::Vector3 const & target_pos = mTarget->_getDerivedPosition();
    Ogre::Real dist = GetCameraTargetDistance();
    mCamera->setPosition(target_pos);
    mCamera->setOrientation(Ogre::Quaternion::IDENTITY);
    mCamera->lookAt(target_pos + direction);
    mCamera->moveRelative(Ogre::Vector3(0, 0, dist));
    m_inverse_view_matrix_dirty = true;
}

void CameraHandler::TopView(bool inverted)
{
    // For some reason, Camera::LookAt() doesn't do the trick here.

    Ogre::Vector3 const & target_pos = mTarget->_getDerivedPosition();
    Ogre::Real dist = GetCameraTargetDistance();
    mCamera->setPosition(target_pos);
    mCamera->setOrientation(Ogre::Quaternion::IDENTITY);
    
    mCamera->yaw(Ogre::Degree(180));
    mCamera->pitch(Ogre::Degree(inverted ? -90 : 90)); // Look up/down (truck logical space)

    mCamera->moveRelative(Ogre::Vector3(0, 0, dist));
    m_inverse_view_matrix_dirty = true;
}

/**
* @author http://www.ogre3d.org/forums/viewtopic.php?p=463232#p463232
* @author http://www.ogre3d.org/tikiwiki/tiki-index.php?page=GetScreenspaceCoords&structure=Cookbook
*/
bool CameraHandler::ConvertWorldToScreenPosition(
    Ogre::Vector3 const & _world_position,
    Vector2int & out_screen_position
)
{
    // Transform position: world space -> view space
    Ogre::Vector3 view_space_pos = mCamera->getViewMatrix(true) * _world_position;

    // Check if the position is in front of the camera
    if (view_space_pos.z < 0.f)
    {
        // Transform: view space -> clip space [-1, 1]
        Ogre::Vector3 clip_space_pos = mCamera->getProjectionMatrix() * view_space_pos;

        // Transform: clip space [-1, 1] -> to [0, 1]
        float screen_space_pos_x = (clip_space_pos.x / 2.f) + 0.5f;
        float screen_space_pos_y = 1 - ((clip_space_pos.y / 2.f) + 0.5f);

        // Transform: clip space -> absolute pixel coordinates
        Ogre::Viewport* viewport = mCamera->getViewport();
        out_screen_position.x = static_cast<int>(screen_space_pos_x * viewport->getActualWidth());
        out_screen_position.y = static_cast<int>(screen_space_pos_y * viewport->getActualHeight());

        return true;
    }
    else
    {
        return false;
    }
}

Ogre::Vector3 CameraHandler::ConvertScreenToWorldPosition(Vector2int const & screen_pos, Ogre::Vector3 const & pivot)
{
    if (m_inverse_projection_matrix_dirty)
    {
        m_inverse_projection_matrix = mCamera->getProjectionMatrix().inverse();
        m_inverse_projection_matrix_dirty = false;
    }

    if (m_inverse_view_matrix_dirty)
    {
        m_inverse_view_matrix = mCamera->getViewMatrix().inverse();
        m_inverse_view_matrix_dirty = false;
    }

    // NOTE:
    // Screen pixel coordinates depend on your resolution:
    //     * start at top left corner of the screen (X:0, Y:0)
    //     * end in bottom right (for example X:1024, Y:768)
    // Clip space coordinates are always in range [-1, 1]:
    //     * start in the middle of the screen (X:0, Y:0, Z:0)
    //     * Bottom left corner of screen is (X:-1, Y:-1, Z:0)
    //     * Top right corner of screen is (X:1, Y:1, Z:0)

    // Transform: pixel coordinates -> clip space
    Ogre::Viewport* viewport = mCamera->getViewport();
    Ogre::Vector3 output(
            (((static_cast<float>(screen_pos.x) / static_cast<float>(viewport->getActualWidth()) ) *  2.f) - 1.f),
            (((static_cast<float>(screen_pos.y) / static_cast<float>(viewport->getActualHeight())) * -2.f) + 1.f),
            0.f
        );

    // Set point's depth, defined by pivot
    Ogre::Vector3 clip_space_pivot = mCamera->getProjectionMatrix() * mCamera->getViewMatrix() * pivot; // Transform pivot: world space -> clip space
    output.z = clip_space_pivot.z;

    // Transform clip space -> view space
    output = m_inverse_projection_matrix * output;

    // Transform view space -> world space
    return m_inverse_view_matrix * output;
}
