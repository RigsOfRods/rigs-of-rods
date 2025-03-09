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

#include "CmdKeyInertia.h"

#include "Application.h"
#include "Utils.h"

#include <OgreDataStream.h>
#include <OgreResourceGroupManager.h>
#include <OgreSimpleSpline.h>
#include <OgreVector3.h>

RoR::CmdKeyInertia::CmdKeyInertia()
    : m_start_spline(nullptr)
    , m_stop_spline(nullptr)
    , m_start_delay(0.f)
    , m_stop_delay(0.f)
    , m_last_output(0.f)
    , m_time(0.f)
{}

float RoR::CmdKeyInertia::CalcCmdKeyDelay(float cmd_input, float dt)
{
    if (!m_start_spline || !m_stop_spline)
    {
        return cmd_input;
    }

    float calculated_output = m_last_output;
    float last_output = m_last_output;
    // rel difference to calculate if we have to use start values(accelerating) or stop values
    float rel_diff = fabs(cmd_input) - fabs(last_output);
    // difference to calculate if were are on the negative side
    float abs_diff = cmd_input - last_output;
    // if the value is close to our input, reset the timer
    if (fabs(abs_diff) < 0.002)
        m_time = 0;
    // +dt after the timer had been set to zero prevents the motion to stop at 0.002
    m_time += dt;

    const float start_factor = m_start_delay * m_time;
    const float stop_factor = m_stop_delay * m_time;
    // positive values between 0 and 1
    if (abs_diff > 0)
    { // we have to accelerate our last outout to the new commanded input
        if (rel_diff > 0)
            calculated_output = last_output + this->CalculateCmdOutput(start_factor, m_start_spline);
        if (rel_diff < 0)
        // we have to deccelerate our last outout to the new commanded input
            calculated_output = last_output + this->CalculateCmdOutput(stop_factor, m_stop_spline);
        if (calculated_output > cmd_input)
        // if the calculated value is bigger than input set to input to avoid overshooting
            calculated_output = cmd_input;
    }
    // negative values, mainly needed for hydros, between 0 and -1
    if (abs_diff < 0)
    {
        if (rel_diff > 0)
            calculated_output = last_output - this->CalculateCmdOutput(start_factor, m_start_spline);
        if (rel_diff < 0)
            calculated_output = last_output - this->CalculateCmdOutput(stop_factor, m_stop_spline);
        if (calculated_output < cmd_input)
            calculated_output = cmd_input;
    }
    m_last_output = calculated_output;
    return calculated_output;
}

int RoR::CmdKeyInertia::SetCmdKeyDelay(RoR::CmdKeyInertiaConfig& cfg, float start_delay, float stop_delay, std::string start_function, std::string stop_function)
{
    // Delay values should always be greater than 0
    if (start_delay > 0)
        m_start_delay = start_delay;
    else
        RoR::LogFormat("[RoR|Inertia] Warning: Start Delay '%f', should be >0, using 0", start_delay);

    if (stop_delay > 0)
        m_stop_delay = stop_delay;
    else
        RoR::LogFormat("[RoR|Inertia] Warning: Stop Delay '%f', should be >0, using 0", start_delay);

    // if we don't find the spline, we use the "constant" one
    m_start_function = start_function;
    Ogre::SimpleSpline* start_spline = cfg.GetSplineByName(start_function);
    if (start_spline != nullptr)
        m_start_spline = start_spline;
    else
        RoR::LogFormat("[RoR|Inertia] Start Function '%s' not found", start_function.c_str());

    m_stop_function = stop_function;
    Ogre::SimpleSpline* stop_spline = cfg.GetSplineByName(stop_function);
    if (stop_spline != nullptr)
        m_stop_spline = stop_spline;
    else
        RoR::LogFormat("[RoR|Inertia] Stop Function '%s' not found", stop_function.c_str());

    return 0;
}

float RoR::CmdKeyInertia::CalculateCmdOutput(float time, Ogre::SimpleSpline* spline)
{
    time = std::min(time, 1.0f);

    if (spline)
    {
        Ogre::Vector3 output = spline->interpolate(time);
        return output.y * 0.001f;
    }

    return 0;
}

Ogre::SimpleSpline* RoR::CmdKeyInertiaConfig::GetSplineByName(Ogre::String model)
{
    auto itor = m_splines.find(model);
    if (itor != m_splines.end())
        return &itor->second;
    else
        return nullptr;
}

void RoR::CmdKeyInertiaConfig::LoadDefaultInertiaModels()
{
    try
    {
        Ogre::DataStreamPtr ds = Ogre::ResourceGroupManager::getSingleton().openResource("inertia_models.cfg", Ogre::RGN_AUTODETECT);
        std::string current_model;
        while (!ds->eof())
        {
            std::string line = SanitizeUtf8String(ds->getLine());
            Ogre::StringUtil::trim(line);

            if (line.empty() || line[0] == ';')
                continue;

            Ogre::StringVector args = Ogre::StringUtil::split(line, ",");
            if (args.size() == 1)
            {
                current_model = line;
            }
            else if (args.size() == 2 && !current_model.empty())
            {
                // find the spline to attach the points
                if (m_splines.find(current_model) == m_splines.end())
                {
                    m_splines[current_model] = Ogre::SimpleSpline();
                }

                // parse the data
                const float point_x = Ogre::StringConverter::parseReal(args[0]);
                const float point_y = Ogre::StringConverter::parseReal(args[1]);

                // attach the points to the spline
                m_splines[current_model].addPoint(Ogre::Vector3(point_x, point_y, 0.0f));
            }
        }
    }
    catch (std::exception& e)
    {
        RoR::LogFormat("[RoR|Inertia] Failed to load 'inertia_models.cfg', message: '%s'", e.what());
    }
}

void RoR::CmdKeyInertia::ResetCmdKeyDelay()
{
    // reset last_output and time, if we reset the truck
    m_last_output = 0.0;
    m_time = 0.0;
}

// -------------------------- Simple inertia --------------------------

void RoR::SimpleInertia::SetSimpleDelay(RoR::CmdKeyInertiaConfig& cfg, float start_delay, float stop_delay, std::string start_function, std::string stop_function)
{
    // Delay values should always be greater than 0
    if (start_delay > 0)
        m_start_delay = start_delay;
    else
        RoR::LogFormat("[RoR|SimpleInertia] Warning: Start Delay '%f', should be >0, using 0", start_delay);

    if (stop_delay > 0)
        m_stop_delay = stop_delay;
    else
        RoR::LogFormat("[RoR|SimpleInertia] Warning: Stop Delay '%f', should be >0, using 0", start_delay);

    // if we don't find the spline, we use the "constant" one
    Ogre::SimpleSpline* start_spline = cfg.GetSplineByName(start_function);
    if (start_spline != nullptr)
        m_start_spline = start_spline;
    else
        RoR::LogFormat("[RoR|SimpleInertia] Start Function '%s' not found", start_function.c_str());

    Ogre::SimpleSpline* stop_spline = cfg.GetSplineByName(stop_function);
    if (stop_spline != nullptr)
        m_stop_spline = stop_spline;
    else
        RoR::LogFormat("[RoR|SimpleInertia] Stop Function '%s' not found", stop_function.c_str());
}

float RoR::SimpleInertia::CalcSimpleDelay(bool input, float dt)
{
    if (input)
    {
        if (m_spline_time < 1.f)
        {
            m_spline_time += (dt / m_start_delay);
            if (m_spline_time > 1.f)
            {
                m_spline_time = 1.f;
            }
        }
    }
    else
    {
        if (m_spline_time > 0.f)
        {
            m_spline_time -= (dt / m_stop_delay);
            if (m_spline_time < 0.f)
            {
                m_spline_time = 0.f;
            }
        }
    }

    if (input)
    {
        return m_start_spline->interpolate(m_spline_time).y;
    }
    else
    {
        return m_stop_spline->interpolate(m_spline_time).y;
    }
}

