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

#include "DustPool.h"

#include <Ogre.h>

#include "Application.h"
#include "GameContext.h"
#include "GfxScene.h"
#include "Terrain.h"
#include "Water.h"

using namespace Ogre;
using namespace RoR;

#ifndef _WIN32
// This definitions is needed because the variable is declared but not defined in DustPool
  const int DustPool::MAX_DUSTS;
#endif // !_WIN32

DustPool::DustPool(Ogre::SceneManager* sm, const char* dname, int dsize):
	allocated(0),
	size(std::min(dsize, static_cast<int>(MAX_DUSTS))),
	m_is_discarded(false)
{
    parent_snode = sm->getRootSceneNode()->createChildSceneNode(fmt::format("DustPools/{}", dname));

    for (int i = 0; i < size; i++)
    {
        char dename[256];
        sprintf(dename, "Dust %s %i", dname, i);
        sns[i] = parent_snode->createChildSceneNode();
        pss[i] = sm->createParticleSystem(dename, dname);
        if (pss[i])
        {
            sns[i]->attachObject(pss[i]);
            pss[i]->setCastShadows(false);
            pss[i]->setVisibilityFlags(RoR::DEPTHMAP_DISABLED);
            if (pss[i]->getNumEmitters() > 0)
            {
                pss[i]->getEmitter(0)->setEnabled(false);
            }
        }
    }
}

DustPool::~DustPool()
{
	ROR_ASSERT(m_is_discarded);
}

void DustPool::Discard(Ogre::SceneManager* sm)
{
	for (int i = 0; i < size; i++)
	{
		sns[i]->removeAndDestroyAllChildren();
		sm->destroySceneNode(sns[i]);
		sns[i] = nullptr;

		if (pss[i])
		{
			sm->destroyParticleSystem(pss[i]);
			pss[i] = nullptr;
		}
	}
    sm->destroySceneNode(parent_snode);
	m_is_discarded = true;
}

void DustPool::setVisible(bool s)
{
    for (int i = 0; i < size; i++)
    {
        pss[i]->setVisible(s);
    }
}

//Dust
void DustPool::malloc(Vector3 pos, Vector3 vel, ColourValue col)
{
    if (allocated < size)
    {
        positions[allocated] = pos;
        velocities[allocated] = vel;
        colours[allocated] = col;
        types[allocated] = DUST_NORMAL;
        allocated++;
    }
}

//Clumps
void DustPool::allocClump(Vector3 pos, Vector3 vel, ColourValue col)
{
    if (allocated < size)
    {
        positions[allocated] = pos;
        velocities[allocated] = vel;
        colours[allocated] = col;
        types[allocated] = DUST_CLUMP;
        allocated++;
    }
}

//Rubber smoke
void DustPool::allocSmoke(Vector3 pos, Vector3 vel)
{
    if (allocated < size)
    {
        positions[allocated] = pos;
        velocities[allocated] = vel;
        types[allocated] = DUST_RUBBER;
        allocated++;
    }
}

//
void DustPool::allocSparks(Vector3 pos, Vector3 vel)
{
    if (vel.length() < 0.1)
        return; // try to prevent emitting sparks while standing
    if (allocated < size)
    {
        positions[allocated] = pos;
        velocities[allocated] = vel;
        types[allocated] = DUST_SPARKS;
        allocated++;
    }
}

//Water vapour
void DustPool::allocVapour(Vector3 pos, Vector3 vel, float time)
{
    if (allocated < size)
    {
        positions[allocated] = pos;
        velocities[allocated] = vel;
        types[allocated] = DUST_VAPOUR;
        rates[allocated] = 5.0 - time;
        allocated++;
    }
}

void DustPool::allocDrip(Vector3 pos, Vector3 vel, float time)
{
    if (allocated < size)
    {
        positions[allocated] = pos;
        velocities[allocated] = vel;
        types[allocated] = DUST_DRIP;
        rates[allocated] = 5.0 - time;
        allocated++;
    }
}

void DustPool::allocSplash(Vector3 pos, Vector3 vel)
{
    if (allocated < size)
    {
        positions[allocated] = pos;
        velocities[allocated] = vel;
        types[allocated] = DUST_SPLASH;
        allocated++;
    }
}

void DustPool::allocRipple(Vector3 pos, Vector3 vel)
{
    if (allocated < size)
    {
        positions[allocated] = pos;
        velocities[allocated] = vel;
        types[allocated] = DUST_RIPPLE;
        allocated++;
    }
}

void DustPool::update()
{
    for (int i = 0; i < allocated; i++)
    {
        App::GetGfxScene()->AdjustParticleSystemTimeFactor(pss[i]);
        ParticleEmitter* emit = pss[i]->getEmitter(0);
        Vector3 ndir = velocities[i];
        Real vel = ndir.length();
        ColourValue col = colours[i];

        if (vel == 0)
            vel += 0.0001;
        ndir = ndir / vel;

        emit->setEnabled(true);

        if (types[i] != DUST_RIPPLE)
        {
            emit->setDirection(ndir);
            emit->setParticleVelocity(vel);
            sns[i]->setPosition(positions[i]);
        }

        if (types[i] == DUST_NORMAL)
        {
            ndir.y = 0;
            ndir = ndir / 2.0;

            col.a = vel * 0.05;
            emit->setTimeToLive(vel * 0.05 / 0.1);
        }
        else if (types[i] == DUST_CLUMP)
        {
            ndir = ndir / 2.0;
            if (ndir.y < 0)
                ndir.y = -ndir.y;

            col.a = 1.0;
        }
        else if (types[i] == DUST_RUBBER)
        {
            ndir.y = 0;
            ndir = ndir / 4.0;

            col.a = sqrt(vel) * 0.1;
            col.b = 0.9;
            col.g = 0.9;
            col.r = 0.9;

            emit->setTimeToLive(vel * 0.025 / 0.1);
        }
        else if (types[i] == DUST_SPARKS)
        {
            //ugh
        }
        else if (types[i] == DUST_VAPOUR)
        {
            emit->setParticleVelocity(vel / 2.0);

            col.a = rates[i] * 0.03;
            col.b = 0.9;
            col.g = 0.9;
            col.r = 0.9;

            emit->setTimeToLive(rates[i] * 0.03 / 0.1);
        }
        else if (types[i] == DUST_DRIP)
        {
            emit->setEmissionRate(rates[i]);
        }
        else if (types[i] == DUST_SPLASH)
        {
            if (ndir.y < 0)
                ndir.y = -ndir.y / 2.0;

            emit->setDirection(ndir);

            col.a = sqrt(vel) * 0.04;
            col.b = 0.9;
            col.g = 0.9;
            col.r = 0.9;

            emit->setTimeToLive(vel * 0.025 / 0.1);
        }
        else if (types[i] == DUST_RIPPLE)
        {
            positions[i].y = RoR::App::GetGameContext()->GetTerrain()->getWater()->GetStaticWaterHeight() - 0.02;
            sns[i]->setPosition(positions[i]);

            col.a = vel * 0.04;
            col.b = 0.9;
            col.g = 0.9;
            col.r = 0.9;

            emit->setTimeToLive(vel * 0.04 / 0.1);
        }

        emit->setColour(col);
    }
    for (int i = allocated; i < size; i++)
    {
        pss[i]->getEmitter(0)->setEnabled(false);
    }
    allocated = 0;
}
