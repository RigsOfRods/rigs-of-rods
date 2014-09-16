/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2014 Petr Ohlidal

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
}

float CameraHandler::GetCameraTargetDistance()
{
	return mCamera->getPosition().distance(mTarget->getPosition());
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

void CameraHandler::Update(float delta_time_seconds)
{
    if (mStyle == CS_FREELOOK)
    {
        // build our acceleration vector based on keyboard input composite
        Ogre::Vector3 accel = Ogre::Vector3::ZERO;
        if (mGoingForward) accel += mCamera->getDirection();
        if (mGoingBack) accel -= mCamera->getDirection();
        if (mGoingRight) accel += mCamera->getRight();
        if (mGoingLeft) accel -= mCamera->getRight();
        if (mGoingUp) accel += mCamera->getUp();
        if (mGoingDown) accel -= mCamera->getUp();

        // if accelerating, try to reach top speed in a certain time
        Ogre::Real topSpeed = mFastMove ? mTopSpeed * 20 : mTopSpeed;
        if (accel.squaredLength() != 0)
        {
            accel.normalise();
            mVelocity += accel * topSpeed * delta_time_seconds * 10;
        }
        // if not accelerating, try to stop in a certain time
        else mVelocity -= mVelocity * delta_time_seconds * 10;

        Ogre::Real tooSmall = std::numeric_limits<Ogre::Real>::epsilon();

        // keep camera velocity below top speed and above epsilon
        if (mVelocity.squaredLength() > topSpeed * topSpeed)
        {
            mVelocity.normalise();
            mVelocity *= topSpeed;
        }
        else if (mVelocity.squaredLength() < tooSmall * tooSmall)
            mVelocity = Ogre::Vector3::ZERO;

        if (mVelocity != Ogre::Vector3::ZERO) mCamera->move(mVelocity * delta_time_seconds);
    }
}

bool CameraHandler::InjectMouseMove(bool do_orbit, int x_rel, int y_rel, int wheel_rel)
{
    if (mStyle == CS_ORBIT)
    {
        Ogre::Real dist = (mCamera->getPosition() - mTarget->_getDerivedPosition()).length();

        if (do_orbit)   // yaw around the target, and pitch locally
        {
            mCamera->setPosition(mTarget->_getDerivedPosition());

            mCamera->yaw(Ogre::Degree(-x_rel * 0.25f));
            mCamera->pitch(Ogre::Degree(-y_rel * 0.25f));

            mCamera->moveRelative(Ogre::Vector3(0, 0, dist));
			return true;

            // don't let the camera go over the top or around the bottom of the target
        }
        else if (mZooming)  // (mouse-move zooming) move the camera toward or away from the target
        {
            // the further the camera is, the faster it moves
            mCamera->moveRelative(Ogre::Vector3(0, 0, y_rel * 0.004f * dist));
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

Ogre::Vector3 CameraHandler::ConvertWorldToViewPosition(
	Ogre::Vector3 const & _world_position
)
{
	return mCamera->getProjectionMatrix() * (mCamera->getViewMatrix(true) * _world_position);
}

/**
* @author http://www.ogre3d.org/forums/viewtopic.php?p=463232#p463232
* @author http://www.ogre3d.org/tikiwiki/tiki-index.php?page=GetScreenspaceCoords&structure=Cookbook
*/
bool CameraHandler::ConvertWorldToScreenPosition(
	Ogre::Vector3 const & _world_position,
	Vector2int & out_screen_position,
	float & _out_camera_distance
)
{
	// Transform position
	Ogre::Vector3 eye_space_pos = mCamera->getViewMatrix(true) * _world_position;

	// Check if the position is in front of the camera
	if (eye_space_pos.z < 0.f)
	{
		// Get distance
		_out_camera_distance = -eye_space_pos.z;

		Ogre::Vector3 view_space_pos = mCamera->getProjectionMatrix() * eye_space_pos;

		// Transform from coordinate space [-1, 1] to [0, 1]
		float screen_space_pos_x = (view_space_pos.x / 2.f) + 0.5f;
		float screen_space_pos_y = 1 - ((view_space_pos.y / 2.f) + 0.5f);

		// Debug
		//std::stringstream s;
		//s.setf(std::ios::fixed);
		//s.precision(3);
		//s<<"view: {"<<view_space_pos.x<<"; "<<view_space_pos.y<<"; "<<view_space_pos.z
		//	<<"}, screenspace: {"<<screen_space_pos.x<<", "<<screen_space_pos.y<<", "<<screen_space_pos.z<<"}";
		//LOG(s.str())
		// ------

		// Transform to absolute pixel coordinates
		Ogre::Viewport* viewport = mCamera->getViewport();
		out_screen_position.x = static_cast<int>(screen_space_pos_x * viewport->getActualWidth());
		out_screen_position.y = static_cast<int>(screen_space_pos_y * viewport->getActualHeight());

		return true;
	}
	else
	{
		_out_camera_distance = 0.f;
		return false;
	}
}
//-------------------------------------------------------------------------------------------------
