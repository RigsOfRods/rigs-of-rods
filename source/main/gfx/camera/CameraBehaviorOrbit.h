/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef __CAMERA_BEHAVIOR_ORBIT_H_
#define __CAMERA_BEHAVIOR_ORBIT_H_

#include "RoRPrerequisites.h"

#include "CameraManager.h"
#include "IBehavior.h"

class CameraBehaviorOrbit : public IBehavior<CameraManager::CameraContext>
{
public:

	CameraBehaviorOrbit();

	void update(const CameraManager::CameraContext &ctx);

	void activate(const CameraManager::CameraContext &ctx, bool reset = true) {};
	void deactivate(const CameraManager::CameraContext &ctx) {};
	void reset(const CameraManager::CameraContext &ctx);

	bool mouseMoved(const CameraManager::CameraContext &ctx, const OIS::MouseEvent& _arg);
	bool mousePressed(const CameraManager::CameraContext &ctx, const OIS::MouseEvent& _arg, OIS::MouseButtonID _id) { return false; };
	bool mouseReleased(const CameraManager::CameraContext &ctx, const OIS::MouseEvent& _arg, OIS::MouseButtonID _id) { return false; };

protected:

	Ogre::Radian camRotX, camRotY;
	Ogre::Radian camRotXSwivel, camRotYSwivel;
	Ogre::Radian targetDirection, targetPitch;
	Ogre::Real camDist, camDistMin, camDistMax, camRatio;
	Ogre::Vector3 camLookAt;

	bool limitMinCamDist;
};

#endif // __CAMERA_BEHAVIOR_ORBIT_H_
