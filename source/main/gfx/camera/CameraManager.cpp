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
#include "CameraManager.h"

#include "BeamFactory.h"
#include "DepthOfFieldEffect.h"
#include "InputEngine.h"
#include "RoRFrameListener.h"
#include "Settings.h"

#include "CameraBehaviorOrbit.h"
#include "CameraBehaviorCharacter.h"
#include "CameraBehaviorFixed.h"
#include "CameraBehaviorFree.h"
#include "CameraBehaviorStatic.h"
#include "CameraBehaviorVehicle.h"
#include "CameraBehaviorVehicleCineCam.h"
#include "CameraBehaviorVehicleSpline.h"
#include "CameraBehaviorIsometric.h"

#include <stack>

using namespace Ogre;

CameraManager::CameraManager(OverlayWrapper *ow, DOFManager *dof) : 
	  currentBehavior(0)
	, currentBehaviorID(-1)
	, mTransScale(1.0f)
	, mTransSpeed(50.0f)
	, mRotScale(0.1f)
	, mRotateSpeed(100.0f)
{
	gEnv->cameraManager = this;

	createGlobalBehaviors();

	ctx.mCurrTruck = 0;
	ctx.mDof = dof;
	ctx.mOverlayWrapper = ow;
	ctx.mDebug = BSETTING("Camera Debug", false);

	if ( ctx.mDof )
	{
		ctx.mDof->setFocusMode(DOFManager::Auto);
	}
}

CameraManager::~CameraManager()
{
	for (std::map <int , IBehavior<CameraContext> *>::iterator it = globalBehaviors.begin(); it != globalBehaviors.end(); ++it)
	{
		delete it->second;
	}

	globalBehaviors.clear();
}

bool CameraManager::update(float dt)
{
	static std::stack<int> precedingBehaviors;

	if ( dt == 0 ) return false;

	mTransScale = mTransSpeed  * dt;
	mRotScale   = mRotateSpeed * dt;

	ctx.mCurrTruck  = BeamFactory::getSingleton().getCurrentTruck();
	ctx.mDt         = dt;
	ctx.mRotScale   = Degree(mRotScale);
	ctx.mTransScale = mTransScale;
	ctx.fovInternal = Degree(FSETTING("FOV Internal", 75.0f));
	ctx.fovExternal = Degree(FSETTING("FOV External", 60.0f));

	if ( currentBehaviorID < CAMERA_BEHAVIOR_END && INPUTENGINE.getEventBoolValueBounce(EV_CAMERA_CHANGE) )
	{
		switchToNextBehavior(false);
	}

	if ( INPUTENGINE.getEventBoolValueBounce(EV_CAMERA_FREE_MODE_FIX) )
	{
		toggleBehavior(CAMERA_BEHAVIOR_FIXED);
	}

	if ( INPUTENGINE.getEventBoolValueBounce(EV_CAMERA_FREE_MODE) )
	{
		toggleBehavior(CAMERA_BEHAVIOR_FREE);
	}

	if ( !ctx.mCurrTruck && hasActiveVehicleBehavior() )
	{
		precedingBehaviors.push(currentBehaviorID);
		switchBehavior(CAMERA_BEHAVIOR_CHARACTER);
	} else if ( ctx.mCurrTruck && hasActiveCharacterBehavior() )
	{
		if ( !precedingBehaviors.empty() )
		{
			switchBehavior(precedingBehaviors.top(), false);
			precedingBehaviors.pop();
		} else
		{
			switchBehavior(CAMERA_BEHAVIOR_VEHICLE);
		}
	}

	if ( currentBehavior )
	{
		currentBehavior->update(ctx);
	} else
	{
		switchBehavior(CAMERA_BEHAVIOR_CHARACTER);
	}

	return true;
}

void CameraManager::switchToNextBehavior(bool force /* = true */)
{
	if ( !currentBehavior || force || currentBehavior->switchBehavior(ctx) )
	{
		int i = (currentBehaviorID + 1) % CAMERA_BEHAVIOR_END;
		switchBehavior(i);
	}
}

void CameraManager::switchBehavior(int newBehaviorID, bool reset)
{
	if (newBehaviorID == currentBehaviorID)
	{
		return;
	}

	if ( globalBehaviors.find(newBehaviorID) == globalBehaviors.end() )
	{
		return;
	}

	// deactivate old
	if ( currentBehavior )
	{
		currentBehavior->deactivate(ctx);
	}

	// set new
	currentBehavior = globalBehaviors[newBehaviorID];
	currentBehaviorID = newBehaviorID;

	// activate new
	currentBehavior->activate(ctx, reset);
}

void CameraManager::toggleBehavior(int behavior)
{
	static std::stack<int> precedingBehaviors;

	if ( behavior != currentBehaviorID && (precedingBehaviors.empty() || precedingBehaviors.top() != behavior))
	{
		if ( currentBehaviorID >= 0 )
		{
			precedingBehaviors.push(currentBehaviorID);
		}
		switchBehavior(behavior);
	} else if ( !precedingBehaviors.empty() )
	{
		switchBehavior(precedingBehaviors.top(), false);
		precedingBehaviors.pop();
	} else
	{
		switchToNextBehavior();
	}
}

bool CameraManager::hasActiveBehavior()
{
	return currentBehavior != 0;
}

bool CameraManager::hasActiveCharacterBehavior()
{
	return dynamic_cast<CameraBehaviorCharacter*>(currentBehavior) != 0;
}

bool CameraManager::hasActiveVehicleBehavior()
{
	return dynamic_cast<CameraBehaviorVehicle*>(currentBehavior) != 0;
}

int CameraManager::getCurrentBehavior()
{
	return currentBehaviorID;
}

void CameraManager::createGlobalBehaviors()
{
	globalBehaviors.insert(std::pair<int, IBehavior<CameraContext>*>(CAMERA_BEHAVIOR_CHARACTER, new CameraBehaviorCharacter()));
	globalBehaviors.insert(std::pair<int, IBehavior<CameraContext>*>(CAMERA_BEHAVIOR_STATIC, new CameraBehaviorStatic()));
	globalBehaviors.insert(std::pair<int, IBehavior<CameraContext>*>(CAMERA_BEHAVIOR_VEHICLE, new CameraBehaviorVehicle()));
	globalBehaviors.insert(std::pair<int, IBehavior<CameraContext>*>(CAMERA_BEHAVIOR_VEHICLE_SPLINE, new CameraBehaviorVehicleSpline()));
	globalBehaviors.insert(std::pair<int, IBehavior<CameraContext>*>(CAMERA_BEHAVIOR_VEHICLE_CINECAM, new CameraBehaviorVehicleCineCam()));
	globalBehaviors.insert(std::pair<int, IBehavior<CameraContext>*>(CAMERA_BEHAVIOR_FREE, new CameraBehaviorFree()));
	globalBehaviors.insert(std::pair<int, IBehavior<CameraContext>*>(CAMERA_BEHAVIOR_FIXED, new CameraBehaviorFixed()));
	globalBehaviors.insert(std::pair<int, IBehavior<CameraContext>*>(CAMERA_BEHAVIOR_ISOMETRIC, new CameraBehaviorIsometric()));
}

bool CameraManager::mouseMoved(const OIS::MouseEvent& _arg)
{
	if ( !currentBehavior ) return false;
	return currentBehavior->mouseMoved(ctx, _arg);
}

bool CameraManager::mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
	if ( !currentBehavior ) return false;
	return currentBehavior->mousePressed(ctx, _arg, _id);
}

bool CameraManager::mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
	if ( !currentBehavior ) return false;
	return currentBehavior->mouseReleased(ctx, _arg, _id);
}

bool CameraManager::gameControlsLocked()
{
	// game controls are only disabled in free camera mode for now
	return (currentBehaviorID == CAMERA_BEHAVIOR_FREE);
}

size_t CameraManager::getMemoryUsage()
{
	// TODO
	return 0;
}
