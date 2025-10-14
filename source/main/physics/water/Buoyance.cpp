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

#include "Buoyance.h"

#include "Application.h"
#include "SimData.h"
#include "ActorManager.h"
#include "GameContext.h"
#include "GfxScene.h"
#include "DustPool.h"
#include "Terrain.h"
#include "Water.h"

using namespace Ogre;
using namespace RoR;

Buoyance::Buoyance(DustPool* splash, DustPool* ripple) :
    splashp(splash),
    ripplep(ripple)
{
}

Buoyance::~Buoyance()
{
}

BuoyCachedNodeID_t Buoyance::cacheBuoycabNode(node_t* n)
{
    auto itor = std::find_if(buoy_cached_nodes.begin(), buoy_cached_nodes.end(),
        [n](BuoyCachedNode& bcn)
        {
            return bcn.nodenum == n->pos;
        });

    if (itor == buoy_cached_nodes.end())
    {
        BuoyCachedNodeID_t retval = static_cast<BuoyCachedNodeID_t>(buoy_cached_nodes.size());
        buoy_cached_nodes.emplace_back(n->pos);
        return retval;
    }

    return static_cast<BuoyCachedNodeID_t>(std::distance(buoy_cached_nodes.begin(), itor));
}

//compute tetrahedron volume
inline float Buoyance::computeVolume(Vec3 o, Vec3 a, Vec3 b, Vec3 c)
{
    return ((a - o).dotProduct((b - o).crossProduct(c - o))) / 6.0;
}

//compute pressure and drag force on a submerged triangle
Vec3 Buoyance::computePressureForceSub(Vec3 a, Vec3 b, Vec3 c, Vec3 vel, int type)
{
    //compute normal vector
    Vec3 normal = (b - a).crossProduct(c - a);
    float surf = normal.length();
    if (surf < 0.00001)
        return Vec3();
    normal = normal / surf; //normalize
    surf = surf / 2.0; //surface
    float vol = 0.0;
    if (type != BUOY_DRAGONLY)
    {
        //compute pression prism points
        Vec3 ap = a + (App::GetGameContext()->GetTerrain()->getWater()->CalcWavesHeight(a) - a.y) * 9810 * normal;
        Vec3 bp = b + (App::GetGameContext()->GetTerrain()->getWater()->CalcWavesHeight(b) - b.y) * 9810 * normal;
        Vec3 cp = c + (App::GetGameContext()->GetTerrain()->getWater()->CalcWavesHeight(c) - c.y) * 9810 * normal;
        //find centroid
        Vec3 ctd = (a + b + c + ap + bp + cp) / 6.0;
        //compute volume
        vol += computeVolume(ctd, a, b, c);
        vol += computeVolume(ctd, a, ap, bp);
        vol += computeVolume(ctd, a, bp, b);
        vol += computeVolume(ctd, b, bp, cp);
        vol += computeVolume(ctd, b, cp, c);
        vol += computeVolume(ctd, c, cp, ap);
        vol += computeVolume(ctd, c, ap, a);
        vol += computeVolume(ctd, ap, cp, bp);
    };
    Vec3 drg;
    if (type != BUOY_DRAGLESS)
    {
        //now, the drag
        //take in account the wave speed
        //compute center
        Vec3 tc = (a + b + c) / 3.0;
        vel = vel - App::GetGameContext()->GetTerrain()->getWater()->CalcWavesVelocity(tc);
        float vell = vel.length();
        if (vell > 0.01)
        {
            float cosaoa = fabs(normal.dotProduct(vel / vell));
            //		drg=(-500.0*surf*vell*cosaoa)*vel;
            drg = (-500.0 * surf * vell * vell * cosaoa) * normal;
            if (normal.dotProduct(vel / vell) < 0)
                drg = -drg;
            if (update && splashp)
            {
                float fxl = vell * cosaoa * surf;
                if (fxl > 1.5) //if enough pushing drag
                {
                    Vec3 fxdir = fxl * normal;
                    if (fxdir.y < 0)
                        fxdir.y = -fxdir.y;

                    if (App::GetGameContext()->GetTerrain()->getWater()->CalcWavesHeight(a) - a.y < 0.1)
                        splashp->malloc(a, fxdir);

                    else if (App::GetGameContext()->GetTerrain()->getWater()->CalcWavesHeight(b) - b.y < 0.1)
                        splashp->malloc(b, fxdir);

                    else if (App::GetGameContext()->GetTerrain()->getWater()->CalcWavesHeight(c) - c.y < 0.1)
                        splashp->malloc(c, fxdir);
                }
            }
        }
    }
    //okay
    if (sink)
        return drg;
    if (update && buoy_debug_view)
    {
        buoy_debug_subcabs.emplace_back(a, b, c, normal, drg, vol);
    }
    return vol * normal + drg;
}

//compute pressure and drag forces on a random triangle
Vec3 Buoyance::computePressureForce(Vec3 a, Vec3 b, Vec3 c, Vec3 vel, int type)
{
    float wha = App::GetGameContext()->GetTerrain()->getWater()->CalcWavesHeight((a + b + c) / 3.0);
    //check if fully emerged
    if (a.y > wha && b.y > wha && c.y > wha)
        return Vec3();
    //check if semi emerged
    if (a.y > wha || b.y > wha || c.y > wha)
    {
        //okay, several cases
        //one dip
        if (a.y < wha && b.y > wha && c.y > wha)
        {
            return computePressureForceSub(a, a + (wha - a.y) / (b.y - a.y) * (b - a), a + (wha - a.y) / (c.y - a.y) * (c - a), vel, type);
        }
        if (b.y < wha && c.y > wha && a.y > wha)
        {
            return computePressureForceSub(b, b + (wha - b.y) / (c.y - b.y) * (c - b), b + (wha - b.y) / (a.y - b.y) * (a - b), vel, type);
        }
        if (c.y < wha && a.y > wha && b.y > wha)
        {
            return computePressureForceSub(c, c + (wha - c.y) / (a.y - c.y) * (a - c), c + (wha - c.y) / (b.y - c.y) * (b - c), vel, type);
        }
        //two dips
        if (a.y > wha && b.y < wha && c.y < wha)
        {
            Vec3 tb = a + (wha - a.y) / (b.y - a.y) * (b - a);
            Vec3 tc = a + (wha - a.y) / (c.y - a.y) * (c - a);
            Vec3 f = computePressureForceSub(tb, b, tc, vel, type);
            return f + computePressureForceSub(tc, b, c, vel, type);
        }
        if (b.y > wha && c.y < wha && a.y < wha)
        {
            Vec3 tc = b + (wha - b.y) / (c.y - b.y) * (c - b);
            Vec3 ta = b + (wha - b.y) / (a.y - b.y) * (a - b);
            Vec3 f = computePressureForceSub(tc, c, ta, vel, type);
            return f + computePressureForceSub(ta, c, a, vel, type);
        }
        if (c.y > wha && a.y < wha && b.y < wha)
        {
            Vec3 ta = c + (wha - c.y) / (a.y - c.y) * (a - c);
            Vec3 tb = c + (wha - c.y) / (b.y - c.y) * (b - c);
            Vec3 f = computePressureForceSub(ta, a, tb, vel, type);
            return f + computePressureForceSub(tb, a, b, vel, type);
        }
        return Vec3();
    }
    else
    {
        //fully submerged case
        return computePressureForceSub(a, b, c, vel, type);
    }
}

void Buoyance::computeNodeForce(BuoyCachedNode* a, BuoyCachedNode* b, BuoyCachedNode* c, int type, float timeshift)
{
    if (a->AbsPosition.y > App::GetGameContext()->GetTerrain()->getWater()->CalcWavesHeight(a->AbsPosition) &&
        b->AbsPosition.y > App::GetGameContext()->GetTerrain()->getWater()->CalcWavesHeight(b->AbsPosition) &&
        c->AbsPosition.y > App::GetGameContext()->GetTerrain()->getWater()->CalcWavesHeight(c->AbsPosition))
        return;

    //compute center
    Vec3 m = (a->AbsPosition + b->AbsPosition + c->AbsPosition) / 3.0;

    //suboptimal
    Vec3 mab = (a->AbsPosition + b->AbsPosition) / 2.0;
    Vec3 mbc = (b->AbsPosition + c->AbsPosition) / 2.0;
    Vec3 mca = (c->AbsPosition + a->AbsPosition) / 2.0;
    Vec3 vel = (a->Velocity + b->Velocity + c->Velocity) / 3.0;

    //apply forces
    a->Forces += computePressureForce(a->AbsPosition, mab, m, vel, type) + computePressureForce(a->AbsPosition, m, mca, vel, type);
    b->Forces += computePressureForce(b->AbsPosition, mbc, m, vel, type) + computePressureForce(b->AbsPosition, m, mab, vel, type);
    c->Forces += computePressureForce(c->AbsPosition, mca, m, vel, type) + computePressureForce(c->AbsPosition, m, mbc, vel, type);
}
