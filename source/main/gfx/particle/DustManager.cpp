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
// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 12th of October 2009

#include "DustManager.h"

#include "DustPool.h"
#include "Settings.h"

using namespace Ogre;

DustManager::DustManager() : mEnabled(false)
{
	setSingleton(this);
	mEnabled = BSETTING("Particles", true);

	if (mEnabled)
	{
		dustpools["dust"]   = new DustPool("tracks/Dust", 20);
		dustpools["clump"]  = new DustPool("tracks/Clump", 20);
		dustpools["sparks"] = new DustPool("tracks/Sparks", 10);
		dustpools["drip"]   = new DustPool("tracks/Drip", 50);
		dustpools["splash"] = new DustPool("tracks/Splash", 20);
		dustpools["ripple"] = new DustPool("tracks/Ripple", 20);
	}
}

DustManager::~DustManager()
{
	// delete all created dustpools and remove them
	std::map < Ogre::String , DustPool * >::iterator it;
	for (it=dustpools.begin(); it!=dustpools.end();it++)
	{
		// delete the DustPool instance
		delete(it->second);
		it->second = 0;
	}
	// then clear the vector
	dustpools.clear();
}

DustPool *DustManager::getGroundModelDustPool(ground_model_t *g)
{
	return 0;

	/*
	// disabled for now...
	if (!mEnabled) return 0;

	// if we have a non particle type, then return 0
	if (g->fx_type != FX_PARTICLE) return 0;

	String pname = String(g->particle_name);
	if (!dustpools.size() || dustpools.find(pname) == dustpools.end())
	{
		addNewDustPool(g);
	}
	// return the entry we have
	return dustpools[pname];
	*/
}

/*
void DustManager::addNewDustPool(ground_model_t *g)
{
	if (!mEnabled) return;

	// simple and clean addition of a new dustpool class instance
	String pname = String(g->particle_name);
	DustPool *dp = new DustPool(pname,
								g->fx_particle_amount,
								g->fx_particle_min_velo,
								g->fx_particle_max_velo,
								g->fx_particle_fade,
								g->fx_particle_timedelta,
								g->fx_particle_velo_factor,
								g->fx_particle_ttl,
								gEnv->ogreSceneManager->getRootSceneNode(),
								gEnv->ogreSceneManager);
	dustpools[pname] = dp;
}
*/

void DustManager::update(float wspeed)
{
	if (!mEnabled) return;
	std::map < Ogre::String , DustPool * >::iterator it;
	for (it=dustpools.begin(); it!=dustpools.end();it++)
	{
		it->second->update(wspeed);
	}
}

void DustManager::setVisible(bool visible)
{
	if (!mEnabled) return;
	std::map < Ogre::String , DustPool * >::iterator it;
	for (it=dustpools.begin(); it!=dustpools.end();it++)
	{
		it->second->setVisible(visible);
	}
}

DustPool *DustManager::getDustPool(Ogre::String name)
{
	if (dustpools.find(name) == dustpools.end())
		return 0;
	return dustpools[name];
}
