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
#ifndef __I_BehaviorManager_H__
#define __I_BehaviorManager_H__

#include "RoRPrerequisites.h"

#include "IManager.h"

class IBehaviorManager : public IManager
{
public:

	virtual ~IBehaviorManager() {}

	virtual void switchBehavior(int newBehavior, bool reset = true) = 0;
	virtual void switchToNextBehavior(bool force = true) = 0;
	virtual void toggleBehavior(int behavior) = 0;

	virtual bool gameControlsLocked() = 0;
	virtual bool hasActiveBehavior() = 0;
	virtual bool hasActiveCharacterBehavior() = 0;
	virtual bool hasActiveVehicleBehavior() = 0;

	virtual int getCurrentBehavior() = 0;
};

#endif // __I_BehaviorManager_H__
