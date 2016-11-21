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

#ifdef USE_CAELUM

#pragma once

#include "RoRPrerequisites.h"

#include "CaelumPrerequisites.h"

class SkyManager : public ZeroedMemoryAllocator
{
public:

    SkyManager();
    ~SkyManager();

    void loadScript(Ogre::String script, int fogStart = -1, int fogEnd = -1);

    /// change the time scale
    void setTimeFactor(Ogre::Real f);
    Ogre::Light* getMainLight();
    /// gets the current time scale
    Ogre::Real getTimeFactor();

    /// prints the current time of the simulation in the format of HH:MM:SS
    Ogre::String getPrettyTime();

    bool update(float dt);

    void forceUpdate(float dt);

    void notifyCameraChanged(Ogre::Camera* cam);

    void detectUpdate();

    Caelum::CaelumSystem* getCaelumSys()
    {
        return mCaelumSystem;
    }

protected:
    Caelum::LongReal lc;
    Caelum::CaelumSystem* mCaelumSystem;
    Caelum::CaelumSystem* getCaelumSystem() { return mCaelumSystem; };
};

#endif // USE_CAELUM
