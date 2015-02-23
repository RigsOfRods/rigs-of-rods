/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "DustPool.h"

#include "Ogre.h"
#include "RoRPrerequisites.h"
#include "TerrainManager.h"
#include "Water.h"

using namespace Ogre;

#ifndef _WIN32
  // This definitions is needed because the variable is declared but not defined in DustPool
  const int DustPool::MAX_DUSTS;
#endif // !_WIN32

DustPool::DustPool(const char* dname, int dsize) : 
	  allocated(0)
	, size(std::min(dsize, MAX_DUSTS))
{
	for (int i=0; i<size; i++)
	{
		char dename[256];
		sprintf(dename,"Dust %s %i", dname, i);
		sns[i]=gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
		pss[i]=gEnv->sceneManager->createParticleSystem(dename, dname);
		if (pss[i])
		{
			sns[i]->attachObject(pss[i]);
			pss[i]->setCastShadows(false);
			pss[i]->setVisibilityFlags(DEPTHMAP_DISABLED);
			if (pss[i]->getNumEmitters() > 0)
			{
				pss[i]->getEmitter(0)->setEnabled(false);
			}
			//can't do this
			//if (w) ((DeflectorPlaneAffector*)(pss[i]->getAffector(0)))->setPlanePoint(Vector3(0, w->getHeight(), 0));
		}
	}

	// hide after creation
	//setVisible(false);
	
	pthread_mutex_init(&allocation_mutex, NULL);
}

DustPool::~DustPool()
{
	pthread_mutex_destroy(&allocation_mutex);
}

void DustPool::setVisible(bool s)
{
	for (int i=0; i<size; i++)
	{
		//visible[i] = s;
		pss[i]->setVisible(s);
		//pss[i]->setVisible(s ^ (types[i]==DUST_RIPPLE));
		//pss[i]->setEmitting(s);
	}
}

//Dust
void DustPool::malloc(Vector3 pos, Vector3 vel, ColourValue col)
{
	MUTEX_LOCK(&allocation_mutex);
	if (allocated < size)
	{
		positions[allocated]=pos;
		velocities[allocated]=vel;
		colours[allocated]=col;
		types[allocated]=DUST_NORMAL;
		//visible[allocated]=true;
		allocated++;
	}
	MUTEX_UNLOCK(&allocation_mutex);
}

//Clumps
void DustPool::allocClump(Vector3 pos, Vector3 vel, ColourValue col)
{
	MUTEX_LOCK(&allocation_mutex);
	if (allocated < size)
	{
		positions[allocated]=pos;
		velocities[allocated]=vel;
		colours[allocated]=col;
		types[allocated]=DUST_CLUMP;
		//visible[allocated]=true;
		allocated++;
	}
	MUTEX_UNLOCK(&allocation_mutex);
}

//Rubber smoke
void DustPool::allocSmoke(Vector3 pos, Vector3 vel)
{
	MUTEX_LOCK(&allocation_mutex);
	if (allocated < size)
	{
		positions[allocated]=pos;
		velocities[allocated]=vel;
		types[allocated]=DUST_RUBBER;
		//visible[allocated]=true;
		allocated++;
	}
	MUTEX_UNLOCK(&allocation_mutex);
}

//
void DustPool::allocSparks(Vector3 pos, Vector3 vel)
{
	if (vel.length() < 0.1) return; // try to prevent emitting sparks while standing
	MUTEX_LOCK(&allocation_mutex);
	if (allocated < size)
	{
		positions[allocated]=pos;
		velocities[allocated]=vel;
		types[allocated]=DUST_SPARKS;
		//visible[allocated]=true;
		allocated++;
	}
	MUTEX_UNLOCK(&allocation_mutex);
}

//Water vapour
void DustPool::allocVapour(Vector3 pos, Vector3 vel, float time)
{
	MUTEX_LOCK(&allocation_mutex);
	if (allocated < size)
	{
		positions[allocated]=pos;
		velocities[allocated]=vel;
		types[allocated]=DUST_VAPOUR;
		rates[allocated]=5.0-time;
		//visible[allocated]=true;
		allocated++;
	}
	MUTEX_UNLOCK(&allocation_mutex);
}

void DustPool::allocDrip(Vector3 pos, Vector3 vel, float time)
{
	MUTEX_LOCK(&allocation_mutex);
	if (allocated < size)
	{
		positions[allocated]=pos;
		velocities[allocated]=vel;
		types[allocated]=DUST_DRIP;
		rates[allocated]=5.0-time;
		//visible[allocated]=true;
		allocated++;
	}
	MUTEX_UNLOCK(&allocation_mutex);
}

void DustPool::allocSplash(Vector3 pos, Vector3 vel)
{
	MUTEX_LOCK(&allocation_mutex);
	if (allocated < size)
	{
		positions[allocated]=pos;
		velocities[allocated]=vel;
		types[allocated]=DUST_SPLASH;
		//visible[allocated]=true;
		allocated++;
	}
	MUTEX_UNLOCK(&allocation_mutex);
}

void DustPool::allocRipple(Vector3 pos, Vector3 vel)
{
	MUTEX_LOCK(&allocation_mutex);
	if (allocated < size)
	{
		positions[allocated]=pos;
		velocities[allocated]=vel;
		types[allocated]=DUST_RIPPLE;
		//visible[allocated]=true;
		allocated++;
	}
	MUTEX_UNLOCK(&allocation_mutex);
}

void DustPool::update(float gspeed)
{
	gspeed=fabs(gspeed);
	for (int i=0; i<allocated; i++)
	{
		/*
		// show particle if requested
		if (pss[i] && visible[i])
		{
			pss[i]->setVisible(true);
			//pss[i]->setEmitting(true);
		}
		*/

		if (types[i]==DUST_NORMAL)
		{
			ParticleEmitter *emit=pss[i]->getEmitter(0);
			Vector3 ndir=velocities[i];
			ndir.y=0;
			ndir=ndir/2.0;
			//if (ndir.y<0) ndir.y=-ndir.y;
			Real vel=ndir.length();
			if (vel == 0)
				vel += 0.0001;
			ndir=ndir/vel;
			emit->setEnabled(true);
			sns[i]->setPosition(positions[i]);
			emit->setDirection(ndir);
			emit->setParticleVelocity(vel);
//			emit->setColour(ColourValue(0.65, 0.55, 0.53,(vel+(gspeed/10.0))*0.05));
			ColourValue col=colours[i];
			col.a=(vel+(gspeed/10.0))*0.05;
			emit->setColour(col);
			emit->setTimeToLive((vel+(gspeed/10.0))*0.05/0.1);
		}
		if (types[i]==DUST_CLUMP)
		{
			ParticleEmitter *emit=pss[i]->getEmitter(0);
			Vector3 ndir=velocities[i];
			//ndir.y=0;
			ndir=ndir/2.0;
			if (ndir.y<0) ndir.y=-ndir.y;
			Real vel=ndir.length();
			ndir.y+=vel/5.5;
			vel=ndir.length();
			if (vel == 0)
				vel += 0.0001;
			ndir=ndir/vel;
			emit->setEnabled(true);
			sns[i]->setPosition(positions[i]);
			emit->setDirection(ndir);
			emit->setParticleVelocity(vel);
			ColourValue col=colours[i];
			col.a=1.0;
			emit->setColour(col);
		} else if (types[i]==DUST_RUBBER)
		{
			ParticleEmitter *emit=pss[i]->getEmitter(0);
			Vector3 ndir=velocities[i];
			ndir.y=0;
			ndir=ndir/4.0;
			//if (ndir.y<0) ndir.y=-ndir.y;
			Real vel=ndir.length();
			if (vel == 0)
				vel += 0.0001;
			ndir=ndir/vel;
			emit->setEnabled(true);
			sns[i]->setPosition(positions[i]);
			emit->setDirection(ndir);
			emit->setParticleVelocity(vel);
			emit->setColour(ColourValue(0.9, 0.9, 0.9,vel*0.05));
			emit->setTimeToLive(vel*0.05/0.1);
		} else if (types[i]==DUST_SPARKS)
		{
			ParticleEmitter *emit=pss[i]->getEmitter(0);
			Vector3 ndir=-velocities[i];
			//ndir.y=-ndir.y;
			Real vel=ndir.length();
			if (vel == 0)
				vel += 0.0001;
			ndir=ndir/vel;
			emit->setEnabled(true);
			sns[i]->setPosition(positions[i]);
			emit->setDirection(ndir);
			emit->setParticleVelocity(vel);
		} else if (types[i]==DUST_VAPOUR)
		{
			ParticleEmitter *emit=pss[i]->getEmitter(0);
			Vector3 ndir=velocities[i];
			Real vel=ndir.length();
			if (vel == 0)
				vel += 0.0001;
			ndir=ndir/vel;
			emit->setEnabled(true);
			sns[i]->setPosition(positions[i]);
			emit->setDirection(ndir);
			emit->setParticleVelocity(vel/2.0);
			emit->setColour(ColourValue(0.9, 0.9, 0.9,rates[i]*0.03));
			emit->setTimeToLive(rates[i]*0.03/0.1);
		} else if (types[i]==DUST_DRIP)
		{
			ParticleEmitter *emit=pss[i]->getEmitter(0);
			Vector3 ndir=velocities[i];
			Real vel=ndir.length();
			if (vel == 0)
				vel += 0.0001;
			ndir=ndir/vel;
			emit->setEnabled(true);
			sns[i]->setPosition(positions[i]);
			emit->setDirection(ndir);
			emit->setParticleVelocity(vel);
			emit->setEmissionRate(rates[i]);
		} else if (types[i]==DUST_SPLASH)
		{
			ParticleEmitter *emit=pss[i]->getEmitter(0);
			Vector3 ndir=velocities[i];
			if (ndir.y<0) ndir.y=-ndir.y/2.0;
			ndir=ndir/2.0;
			Real vel=ndir.length();
			if (vel == 0)
				vel += 0.0001;
			ndir=ndir/vel;
			emit->setEnabled(true);
			sns[i]->setPosition(positions[i]);
			emit->setDirection(ndir);
			emit->setParticleVelocity(vel);
			emit->setColour(ColourValue(0.9, 0.9, 0.9,vel*0.05));
			emit->setTimeToLive(vel*0.05/0.1);
		} else if (types[i]==DUST_RIPPLE)
		{
			ParticleEmitter *emit=pss[i]->getEmitter(0);
			Real vel=velocities[i].length();
			emit->setEnabled(true);
			positions[i].y=gEnv->terrainManager->getWater()->getHeight()-0.02;
			sns[i]->setPosition(positions[i]);
			emit->setColour(ColourValue(0.9, 0.9, 0.9,vel*0.04));
			emit->setTimeToLive(vel*0.04/0.1);
		}
	}
	for (int i=allocated; i<size; i++)
	{
		pss[i]->getEmitter(0)->setEnabled(false);
	}
	allocated=0;
}
