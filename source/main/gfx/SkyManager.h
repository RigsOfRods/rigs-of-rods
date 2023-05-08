/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2017-2018 Petr Ohlidal

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

#include "Application.h"

#include "CaelumPrerequisites.h"

#include <Caelum.h>

namespace RoR {

/// @addtogroup Gfx
/// @{

class SkyManager
{
public:

    SkyManager();
    ~SkyManager();

    void           LoadCaelumScript(Ogre::String script, int fogStart = -1, int fogEnd = -1);
    void           SetSkyTimeFactor(Ogre::Real f);  //!< change the time scale
    Ogre::Light*   GetSkyMainLight();
    float          GetSkyTimeFactor();              //!< gets the current time scale
    std::string    GetPrettyTime();                 //!< prints the current time of the simulation in the format of HH:MM:SS
    double         GetTime()                    { return m_caelum_system->getJulianDay(); };
    void           SetTime(double time)         {  m_caelum_system->setJulianDay(time); };
    bool           UpdateSky(float dt);
    void           NotifySkyCameraChanged(Ogre::Camera* cam);
    void           DetectSkyUpdate();
    Caelum::CaelumSystem* GetCaelumSys()        { return m_caelum_system; }

private:
    Caelum::LongReal      m_last_clock;
    Caelum::CaelumSystem* m_caelum_system;
};

/// @} // addtogroup Gfx

} // namespace RoR

#endif // USE_CAELUM
