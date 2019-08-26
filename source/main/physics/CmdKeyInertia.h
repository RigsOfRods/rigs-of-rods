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

#pragma once

#include "RoRPrerequisites.h"

namespace RoR {

/// Loads and manages 'inertia_models.cfg'
class CmdKeyInertiaConfig
{
public:
    int LoadDefaultInertiaModels();
    Ogre::SimpleSpline* GetSplineByName(Ogre::String model);

private:
    int ProcessLine(Ogre::StringVector args, Ogre::String model);

    std::map<Ogre::String, Ogre::SimpleSpline> m_splines;
};

} // namespace RoR

class CmdKeyInertia
{
public:

    Ogre::Real calcCmdKeyDelay(Ogre::Real cmdInput, int cmdKey, Ogre::Real dt);
    int setCmdKeyDelay(RoR::CmdKeyInertiaConfig& cfg, int number, Ogre::Real startDelay, Ogre::Real stopDelay, Ogre::String startFunction, Ogre::String stopFunction);
    void resetCmdKeyDelay();

protected:

    struct cmdKeyInertia_s
    {
        Ogre::Real lastOutput;
        Ogre::Real startDelay;
        Ogre::Real stopDelay;
        Ogre::Real time;
        Ogre::SimpleSpline* startSpline;
        Ogre::SimpleSpline* stopSpline;
    };

    Ogre::Real calculateCmdOutput(Ogre::Real time, Ogre::SimpleSpline* spline);

    std::map<int, cmdKeyInertia_s> cmdKeyInertia;
};
