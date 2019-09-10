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

#include <Ogre.h>

using namespace Ogre;

RoR::CmdKeyInertia::CmdKeyInertia()
    : m_start_spline(nullptr)
    , m_stop_spline(nullptr)
    , m_start_delay(0.f)
    , m_stop_delay(0.f)
    , m_last_output(0.f)
    , m_time(0.f)
{}

Real RoR::CmdKeyInertia::CalcCmdKeyDelay(Real cmd_input, Real dt)
{
    if (!m_start_spline || !m_stop_spline)
    {
        return cmd_input;
    }

    Real calculated_output = m_last_output;
    Real last_output = m_last_output;
    // rel difference to calculate if we have to use start values(accelerating) or stop values
    Real rel_diff = fabs(cmd_input) - fabs(last_output);
    // difference to calculate if were are on the negative side
    Real abs_diff = cmd_input - last_output;
    // if the value is close to our input, reset the timer
    if (fabs(abs_diff) < 0.002)
        m_time = 0;
    // +dt after the timer had been set to zero prevents the motion to stop at 0.002
    m_time += dt;

    Real start_factor = m_start_delay * m_time;
    Real stop_factor = m_stop_delay * m_time;
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

int RoR::CmdKeyInertia::SetCmdKeyDelay(RoR::CmdKeyInertiaConfig& cfg, Real start_delay, Real stop_delay, String start_function, String stop_function)
{
    // Delay values should always be greater than 0
    if (start_delay > 0)
        m_start_delay = start_delay;
    else
        LOG("[RoR|Inertia] Start Delay should be >0");

    if (stop_delay > 0)
        m_stop_delay = stop_delay;
    else
        LOG("[RoR|Inertia] Stop Delay should be >0");

    // if we don't find the spline, we use the "constant" one
    Ogre::SimpleSpline* start_spline = cfg.GetSplineByName(start_function);
    if (start_spline != nullptr)
        m_start_spline = start_spline;
    else
        LOG("[RoR|Inertia] Start Function "+start_function +" not found");

    Ogre::SimpleSpline* stop_spline = cfg.GetSplineByName(stop_function);
    if (stop_spline != nullptr)
        m_stop_spline = stop_spline;
    else
        LOG("[RoR|Inertia] Stop Function "+stop_function +" not found");

    return 0;
}

Real RoR::CmdKeyInertia::CalculateCmdOutput(Real time, SimpleSpline* spline)
{
    time = std::min(time, 1.0f);

    if (spline)
    {
        Vector3 output = spline->interpolate(time);
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

int RoR::CmdKeyInertiaConfig::LoadDefaultInertiaModels()
{
    // check if we have a config file
    String group = "";
    try
    {
        group = ResourceGroupManager::getSingleton().findGroupContainingResource("inertia_models.cfg");
    }
    catch (...)
    {
    }
    // emit a warning if we did not found the file
    if (group.empty())
    {
        LOG("Inertia| inertia_models.cfg not found");
        return 1;
    }

    // open the file for reading
    DataStreamPtr ds = ResourceGroupManager::getSingleton().openResource("inertia_models.cfg", group);
    String line = "";
    String current_model = "";

    while (!ds->eof())
    {
        line = RoR::Utils::SanitizeUtf8String(ds->getLine());
        StringUtil::trim(line);

        if (line.empty() || line[0] == ';')
            continue;

        Ogre::StringVector args = StringUtil::split(line, ",");

        if (args.size() == 1)
        {
            current_model = line;
            continue;
        }

        // process the line if we got a model
        if (!current_model.empty())
            this->ProcessLine(args, current_model);
    }
    return 0;
}

int RoR::CmdKeyInertiaConfig::ProcessLine(Ogre::StringVector args, String model)
{
    // we only accept 2 arguments
    if (args.size() != 2)
        return 1;
    // parse the data
    float point_x = StringConverter::parseReal(args[0]);
    float point_y = StringConverter::parseReal(args[1]);
    Vector3 point = Vector3(point_x, point_y, 0.0f);

    // find the spline to attach the points
    if (m_splines.find(model) == m_splines.end())
    {
        m_splines[model] = SimpleSpline();
    }

    // attach the points to the spline
    m_splines[model].addPoint(point);

    return 0;
}

void RoR::CmdKeyInertia::ResetCmdKeyDelay()
{
    // reset last_output and time, if we reset the truck
    m_last_output = 0.0;
    m_time = 0.0;
}
