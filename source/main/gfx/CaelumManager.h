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
#ifdef USE_CAELUM

#ifndef __CaelumManager_H_
#define __CaelumManager_H_

#include "RoRPrerequisites.h"

#include "CaelumPrerequisites.h"
#include "IManager.h"

class CaelumManager : public IManager
{
public:

	CaelumManager();
	~CaelumManager();

	void loadScript(Ogre::String script, int fogStart=-1, int fogEnd=-1);
	
	/// change the time scale
	void setTimeFactor(Ogre::Real f);
	Ogre::Light *getMainLight();
	/// gets the current time scale
	Ogre::Real getTimeFactor();
	
	/// prints the current time of the simulation in the format of HH:MM:SS
	Ogre::String getPrettyTime();
	
	bool update( float dt );
	size_t getMemoryUsage();
	void freeResources();

	void forceUpdate(float dt);

	void notifyCameraChanged(Ogre::Camera *cam);

	void detectUpdate();

	Caelum::CaelumSystem* getCaelumSys()
    {
		return mCaelumSystem;
	}
protected:
	Caelum::LongReal lc;
    Caelum::CaelumSystem *mCaelumSystem;
	Caelum::CaelumSystem *getCaelumSystem() { return mCaelumSystem; };
};

#endif // __CaelumManager_H_

#endif // USE_CAELUM
