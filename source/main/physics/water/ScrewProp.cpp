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

#include "ScrewProp.h"

#include "Application.h"
#include "BeamData.h"
#include "BeamFactory.h"
#include "DustPool.h"
#include "GfxScene.h"
#include "RoRFrameListener.h" // SimController
#include "SoundScriptManager.h"
#include "TerrainManager.h"
#include "Water.h"

using namespace Ogre;
using namespace RoR;

Screwprop::Screwprop(node_t* nodes, int noderef, int nodeback, int nodeup, float fullpower, int trucknum) :
    nodes(nodes)
    , noderef(noderef)
    , nodeback(nodeback)
    , nodeup(nodeup)
    , fullpower(fullpower)
    , trucknum(trucknum)
{
    splashp = RoR::App::GetSimController()->GetGfxScene().GetDustPool("splash");
    ripplep = RoR::App::GetSimController()->GetGfxScene().GetDustPool("ripple");
    reset();
}

void Screwprop::updateForces(int update)
{
    if (!App::GetSimTerrain()->getWater())
        return;

    float depth = App::GetSimTerrain()->getWater()->CalcWavesHeight(nodes[noderef].AbsPosition) - nodes[noderef].AbsPosition.y;
    if (depth < 0)
        return; //out of water!
    Vector3 dir = nodes[nodeback].RelPosition - nodes[noderef].RelPosition;
    Vector3 rudaxis = nodes[noderef].RelPosition - nodes[nodeup].RelPosition;
    dir.normalise();
    if (reverse)
        dir = -dir;
    rudaxis.normalise();
    dir = (throtle * fullpower) * (Quaternion(Degree(rudder), rudaxis) * dir);
    nodes[noderef].Forces += dir;

    if (update && splashp && throtle > 0.1)
    {
        if (depth < 0.2)
            splashp->allocSplash(nodes[noderef].AbsPosition, 10.0 * dir / fullpower);
        else
            splashp->allocSplash(nodes[noderef].AbsPosition, 5.0 * dir / fullpower);
        ripplep->allocRipple(nodes[noderef].AbsPosition, 10.0 * dir / fullpower);
    }
}

void Screwprop::setThrottle(float val)
{
    if (val > 1.0)
        val = 1.0;
    if (val < -1.0)
        val = -1.0;
    throtle = fabs(val);
    reverse = (val < 0);
    //pseudo-rpm
    float prpm = (0.5 + fabs(val) / 2.0) * 100.0;
    SOUND_MODULATE(trucknum, SS_MOD_ENGINE, prpm);
}

void Screwprop::setRudder(float val)
{
    if (val > 1.0)
        val = 1.0;
    if (val < -1.0)
        val = -1.0;
    rudder = val * 45.0;
}

float Screwprop::getThrottle()
{
    if (reverse)
        return -throtle;
    else
        return throtle;
}

float Screwprop::getRudder()
{
    return rudder / 45.0;
}

void Screwprop::reset()
{
    setThrottle(0);
    rudder = 0;
    reverse = false;
}

void Screwprop::toggleReverse()
{
    throtle = 0;
    reverse = !reverse;
}
