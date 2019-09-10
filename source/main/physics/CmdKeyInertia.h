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

#include <OgreSimpleSpline.h>
#include <OgreString.h>

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

class CmdKeyInertia
{
public:
    CmdKeyInertia();

    Ogre::Real CalcCmdKeyDelay(Ogre::Real cmdInput, Ogre::Real dt);
    int SetCmdKeyDelay(RoR::CmdKeyInertiaConfig& cfg, Ogre::Real startDelay, Ogre::Real stopDelay, Ogre::String startFunction, Ogre::String stopFunction);
    void ResetCmdKeyDelay();

protected:
    Ogre::Real          m_last_output;
    Ogre::Real          m_start_delay;
    Ogre::Real          m_stop_delay;
    Ogre::Real          m_time;
    Ogre::SimpleSpline* m_start_spline;
    Ogre::SimpleSpline* m_stop_spline;

    Ogre::Real CalculateCmdOutput(Ogre::Real time, Ogre::SimpleSpline* spline);
};

} // namespace RoR
