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
#include "Actor.h"
#include "SimData.h"
#include "ActorManager.h"
#include "DustPool.h"
#include "GfxScene.h"
#include "SoundScriptManager.h"
#include "TerrainManager.h"
#include "Water.h"

using namespace Ogre;
using namespace RoR;

Screwprop::Screwprop(Actor* a, NodeIdx_t noderef, NodeIdx_t nodeback, NodeIdx_t nodeup, float fullpower) :
    m_actor(a)
    , noderef(noderef)
    , nodeback(nodeback)
    , nodeup(nodeup)
    , fullpower(fullpower)
{
    splashp = RoR::App::GetGfxScene()->GetDustPool("splash");
    ripplep = RoR::App::GetGfxScene()->GetDustPool("ripple");
    reset();
}

void Screwprop::updateForces(int update)
{
    if (!App::GetSimTerrain()->getWater())
        return;

    float depth = App::GetSimTerrain()->getWater()->CalcWavesHeight(m_actor->ar_nodes[noderef].AbsPosition) - m_actor->ar_nodes[noderef].AbsPosition.y;
    if (depth < 0)
        return; //out of water!
    Vector3 dir = m_actor->ar_nodes[nodeback].RelPosition - m_actor->ar_nodes[noderef].RelPosition;
    Vector3 rudaxis = m_actor->ar_nodes[noderef].RelPosition - m_actor->ar_nodes[nodeup].RelPosition;
    dir.normalise();
    if (reverse)
        dir = -dir;
    rudaxis.normalise();
    dir = (throtle * fullpower) * (Quaternion(Degree(rudder), rudaxis) * dir);
    m_actor->ar_nodes[noderef].Forces += dir;

    if (update && splashp && throtle > 0.1)
    {
        if (depth < 0.2)
            splashp->allocSplash(m_actor->ar_nodes[noderef].AbsPosition, 10.0 * dir / fullpower);
        else
            splashp->allocSplash(m_actor->ar_nodes[noderef].AbsPosition, 5.0 * dir / fullpower);
        ripplep->allocRipple(m_actor->ar_nodes[noderef].AbsPosition, 10.0 * dir / fullpower);
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
    SOUND_MODULATE(m_actor, SS_MOD_ENGINE, prpm);
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
