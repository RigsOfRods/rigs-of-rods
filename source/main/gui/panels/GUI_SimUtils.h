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
	along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

/** 
	@file   GUI_SimUtils.h
	@author Moncef Ben Slimane
	@date   12/2014
*/

#include "ForwardDeclarations.h"
#include "GUI_SimUtilsLayout.h"
#include "InterThreadStoreVector.h"

namespace RoR
{

struct NotificationMessage
{
	unsigned long time; //!< post time in milliseconds since RoR start
	unsigned long ttl;  //!< in milliseconds
	Ogre::UTFString txt; //!< not POD, beware...
	Ogre::UTFString title; //!< not POD, beware...
};

namespace GUI
{

class SimUtils: public SimUtilsLayout
{

public:
	SimUtils();
	~SimUtils();

	void ShowMain();
	void HideMain();
	
	void ToggleFPSBox();
	void ToggleTruckInfoBox();

	void UpdateStats(float dt, Beam *truck); //different from Framestep!
	void framestep(float dt);

	void PushNotification(Ogre::String Title, Ogre::String text);

	//Not sure about this, might throw some exceptions..
	bool GetMainVisibiltyState()
	{
		return ((MyGUI::Window*)mMainWidget)->getVisible();
	}
	
private:
	bool b_fpsbox;
	bool b_truckinfo;
	bool b_notification;

	//taken from TruckHUD.h
	std::map<int, float> avVelos;
	std::map<int, float> maxNegLatG;
	std::map<int, float> maxNegSagG;
	std::map<int, float> maxNegVerG;
	std::map<int, float> maxPosLatG;
	std::map<int, float> maxPosSagG;
	std::map<int, float> maxPosVerG;
	std::map<int, float> maxVelos;
	std::map<int, float> minVelos;

	//Colors
	Ogre::UTFString MainThemeColor; // colour key shortcut
	Ogre::UTFString WhiteColor; // colour key shortcut
	Ogre::UTFString RedColor; // colour key shortcut
	Ogre::UTFString BlueColor; // colour key shortcut

	Ogre::String truckstats;

	std::vector<NotificationMessage> tmpWaitingNotifications;

	// logic
	float alpha;
	long pushTime;
};

} // namespace GUI

} // namespace RoR
