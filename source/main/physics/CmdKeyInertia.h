/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2020-2023 Petr Ohlidal

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

#include <OgrePrerequisites.h>
#include <string>
#include <unordered_map>

namespace RoR {

/// @addtogroup Gameplay
/// @{

/// Loads and manages 'inertia_models.cfg'
class CmdKeyInertiaConfig
{
public:
    void LoadDefaultInertiaModels();
    Ogre::SimpleSpline* GetSplineByName(Ogre::String model);

private:
    std::map<Ogre::String, Ogre::SimpleSpline> m_splines;
};

/// Designed to be run in physics loop (2khz)
class CmdKeyInertia
{
public:
    CmdKeyInertia();

    float CalcCmdKeyDelay(float cmd_input, float dt);
    int SetCmdKeyDelay(RoR::CmdKeyInertiaConfig& cfg, float start_delay, float stop_delay, std::string start_function, std::string stop_function);
    void ResetCmdKeyDelay();
    float GetStartDelay() const { return m_start_delay; }
    float GetStopDelay() const { return m_stop_delay; }
    const std::string& GetStartFunction() const { return m_start_function; }
    const std::string& GetStopFunction() const { return m_stop_function; }

protected:
    float               m_last_output;
    float               m_start_delay;
    float               m_stop_delay;
    float               m_time;
    std::string         m_start_function;
    Ogre::SimpleSpline* m_start_spline;
    std::string         m_stop_function;
    Ogre::SimpleSpline* m_stop_spline;

    float CalculateCmdOutput(float time, Ogre::SimpleSpline* spline);
};

/// Designed to be run on main/rendering loop (FPS)
class SimpleInertia
{
public:
    void SetSimpleDelay(RoR::CmdKeyInertiaConfig& cfg, float start_delay, float stop_delay, std::string start_function, std::string stop_function);

    /// Expected to be invoked in main/rendering loop, once per frame. The `dt` is in seconds.
    float CalcSimpleDelay(bool input, float dt);

private:
    bool               m_last_input = false;
    float               m_start_delay = 0;
    float               m_stop_delay = 0;
    float               m_spline_time = 0;
    Ogre::SimpleSpline* m_start_spline = nullptr;
    Ogre::SimpleSpline* m_stop_spline = nullptr;
};

/// @} // addtogroup Gameplay

} // namespace RoR
