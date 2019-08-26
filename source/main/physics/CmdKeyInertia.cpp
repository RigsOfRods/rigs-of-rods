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
    : m_startSpline(nullptr)
    , m_stopSpline(nullptr)
    , m_startDelay(0.f)
    , m_stopDelay(0.f)
    , m_lastOutput(0.f)
    , m_time(0.f)
{}

Real RoR::CmdKeyInertia::CalcCmdKeyDelay(Real cmdInput, Real dt)
{
    if (!m_startSpline || !m_stopSpline)
    {
        return cmdInput;
    }

    Real calculatedOutput = m_lastOutput;
    Real lastOutput = m_lastOutput;
    // rel difference to calculate if we have to use start values(accelerating) or stop values
    Real relDiff = fabs(cmdInput) - fabs(lastOutput);
    // difference to calculate if were are on the negative side
    Real absDiff = cmdInput - lastOutput;
    // if the value is close to our input, reset the timer
    if (fabs(absDiff) < 0.002)
        m_time = 0;
    // +dt after the timer had been set to zero prevents the motion to stop at 0.002
    m_time += dt;

    Real startFactor = m_startDelay * m_time;
    Real stopFactor = m_stopDelay * m_time;
    // positive values between 0 and 1
    if (absDiff > 0)
    { // we have to accelerate our last outout to the new commanded input
        if (relDiff > 0)
            calculatedOutput = lastOutput + this->CalculateCmdOutput(startFactor, m_startSpline);
        if (relDiff < 0)
        // we have to deccelerate our last outout to the new commanded input
            calculatedOutput = lastOutput + this->CalculateCmdOutput(stopFactor, m_stopSpline);
        if (calculatedOutput > cmdInput)
        // if the calculated value is bigger than input set to input to avoid overshooting
            calculatedOutput = cmdInput;
    }
    // negative values, mainly needed for hydros, between 0 and -1
    if (absDiff < 0)
    {
        if (relDiff > 0)
            calculatedOutput = lastOutput - this->CalculateCmdOutput(startFactor, m_startSpline);
        if (relDiff < 0)
            calculatedOutput = lastOutput - this->CalculateCmdOutput(stopFactor, m_stopSpline);
        if (calculatedOutput < cmdInput)
            calculatedOutput = cmdInput;
    }
    m_lastOutput = calculatedOutput;
    return calculatedOutput;
}

int RoR::CmdKeyInertia::SetCmdKeyDelay(RoR::CmdKeyInertiaConfig& cfg, Real startDelay, Real stopDelay, String startFunction, String stopFunction)
{
    // Delay values should always be greater than 0
    if (startDelay > 0)
        m_startDelay = startDelay;
    else
        LOG("[RoR|Inertia] Start Delay should be >0");

    if (stopDelay > 0)
        m_stopDelay = stopDelay;
    else
        LOG("[RoR|Inertia] Stop Delay should be >0");

    // if we don't find the spline, we use the "constant" one
    Ogre::SimpleSpline* startSpline = cfg.GetSplineByName(startFunction);
    if (startSpline != nullptr)
        m_startSpline = startSpline;
    else
        LOG("[RoR|Inertia] Start Function "+startFunction +" not found");

    Ogre::SimpleSpline* stopSpline = cfg.GetSplineByName(stopFunction);
    if (stopSpline != nullptr)
        m_stopSpline = stopSpline;
    else
        LOG("[RoR|Inertia] Stop Function "+stopFunction +" not found");

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
    String currentModel = "";

    while (!ds->eof())
    {
        line = RoR::Utils::SanitizeUtf8String(ds->getLine());
        StringUtil::trim(line);

        if (line.empty() || line[0] == ';')
            continue;

        Ogre::StringVector args = StringUtil::split(line, ",");

        if (args.size() == 1)
        {
            currentModel = line;
            continue;
        }

        // process the line if we got a model
        if (!currentModel.empty())
            this->ProcessLine(args, currentModel);
    }
    return 0;
}

int RoR::CmdKeyInertiaConfig::ProcessLine(Ogre::StringVector args, String model)
{
    // we only accept 2 arguments
    if (args.size() != 2)
        return 1;
    // parse the data
    float pointx = StringConverter::parseReal(args[0]);
    float pointy = StringConverter::parseReal(args[1]);
    Vector3 point = Vector3(pointx, pointy, 0.0f);

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
    // reset lastOutput and time, if we reset the truck
    m_lastOutput = 0.0;
    m_time = 0.0;
}
