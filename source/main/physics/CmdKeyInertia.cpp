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

CmdKeyInertia::CmdKeyInertia()
{
    loadDefaultInertiaModels();
}

Real CmdKeyInertia::calcCmdKeyDelay(Real cmdInput, int cmdKey, Real dt)
{
    if (!cmdKeyInertia[cmdKey].startSpline || !cmdKeyInertia[cmdKey].stopSpline)
    {
        return cmdInput;
    }

    Real calculatedOutput = cmdKeyInertia[cmdKey].lastOutput;
    Real lastOutput = cmdKeyInertia[cmdKey].lastOutput;
    // rel difference to calculate if we have to use start values(accelerating) or stop values
    Real relDiff = fabs(cmdInput) - fabs(lastOutput);
    // difference to calculate if were are on the negative side
    Real absDiff = cmdInput - lastOutput;
    // if the value is close to our input, reset the timer
    if (fabs(absDiff) < 0.002)
        cmdKeyInertia[cmdKey].time = 0;
    // +dt after the timer had been set to zero prevents the motion to stop at 0.002
    cmdKeyInertia[cmdKey].time += dt;

    Real startFactor = cmdKeyInertia[cmdKey].startDelay * cmdKeyInertia[cmdKey].time;
    Real stopFactor = cmdKeyInertia[cmdKey].stopDelay * cmdKeyInertia[cmdKey].time;
    // positive values between 0 and 1
    if (absDiff > 0)
    { // we have to accelerate our last outout to the new commanded input
        if (relDiff > 0)
            calculatedOutput = lastOutput + calculateCmdOutput(startFactor, cmdKeyInertia[cmdKey].startSpline);
        if (relDiff < 0)
        // we have to deccelerate our last outout to the new commanded input
            calculatedOutput = lastOutput + calculateCmdOutput(stopFactor, cmdKeyInertia[cmdKey].stopSpline);
        if (calculatedOutput > cmdInput)
        // if the calculated value is bigger than input set to input to avoid overshooting
            calculatedOutput = cmdInput;
    }
    // negative values, mainly needed for hydros, between 0 and -1
    if (absDiff < 0)
    {
        if (relDiff > 0)
            calculatedOutput = lastOutput - calculateCmdOutput(startFactor, cmdKeyInertia[cmdKey].startSpline);
        if (relDiff < 0)
            calculatedOutput = lastOutput - calculateCmdOutput(stopFactor, cmdKeyInertia[cmdKey].stopSpline);
        if (calculatedOutput < cmdInput)
            calculatedOutput = cmdInput;
    }
    cmdKeyInertia[cmdKey].lastOutput = calculatedOutput;
    return calculatedOutput;
}

int CmdKeyInertia::setCmdKeyDelay(int cmdKey, Real startDelay, Real stopDelay, String startFunction, String stopFunction)
{
    // Delay values should always be greater than 0
    if (startDelay > 0)
        cmdKeyInertia[cmdKey].startDelay = startDelay;
    else
    LOG("Inertia| Start Delay should be >0");

    if (stopDelay > 0)
        cmdKeyInertia[cmdKey].stopDelay = stopDelay;
    else
    LOG("Inertia| Stop Delay should be >0");

    // if we don't find the spline, we use the "constant" one
    if (splines.find(startFunction) != splines.end())
        cmdKeyInertia[cmdKey].startSpline = &splines.find(startFunction)->second;
    else
    LOG("Inertia| Start Function "+startFunction +" not found");

    if (splines.find(stopFunction) != splines.end())
        cmdKeyInertia[cmdKey].stopSpline = &splines.find(stopFunction)->second;
    else
    LOG("Inertia| Stop Function "+stopFunction +" not found");

    return 0;
}

Real CmdKeyInertia::calculateCmdOutput(Real time, SimpleSpline* spline)
{
    time = std::min(time, 1.0f);

    if (spline)
    {
        Vector3 output = spline->interpolate(time);
        return output.y * 0.001f;
    }

    return 0;
}

int CmdKeyInertia::loadDefaultInertiaModels()
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
            processLine(args, currentModel);
    }
    return 0;
}

int CmdKeyInertia::processLine(Ogre::StringVector args, String model)
{
    // we only accept 2 arguments
    if (args.size() != 2)
        return 1;
    // parse the data
    float pointx = StringConverter::parseReal(args[0]);
    float pointy = StringConverter::parseReal(args[1]);
    Vector3 point = Vector3(pointx, pointy, 0.0f);

    // find the spline to attach the points
    if (splines.find(model) == splines.end())
    {
        splines[model] = SimpleSpline();
    }

    // attach the points to the spline
    splines[model].addPoint(point);

    return 0;
}

void CmdKeyInertia::resetCmdKeyDelay()
{
    // reset lastOutput and time, if we reset the truck
    for (std::map<int, cmdKeyInertia_s>::iterator it = cmdKeyInertia.begin(); it != cmdKeyInertia.end(); ++it)
    {
        it->second.lastOutput = 0.0;
        it->second.time = 0.0;
    }
}
