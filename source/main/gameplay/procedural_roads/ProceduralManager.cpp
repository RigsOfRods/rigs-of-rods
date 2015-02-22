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

#include "ProceduralManager.h"
#include "Road2.h"

using namespace Ogre;

ProceduralManager::ProceduralManager()
{
	// set values
	objectcounter = 0;
}


ProceduralManager::~ProceduralManager()
{
}

int ProceduralManager::deleteObject(ProceduralObject &po)
{
	if (po.loadingState == 1 && po.road)
	{
		// loaded already, destroy old object
		po.loadingState = 0;
		delete po.road;
		po.road = 0;
	}
	return 0;
}

std::vector<ProceduralObject> &ProceduralManager::getObjects()
{
	return pObjects;
}

int ProceduralManager::updateObject(ProceduralObject &po)
{
	if (po.loadingState == 1 && po.road)
		deleteObject(po);
	// create new road2 object
	po.road = new Road2(objectcounter++);;

	std::vector<ProceduralPoint>::iterator it;
	for (it=po.points.begin(); it!=po.points.end(); it++)
	{
		ProceduralPoint pp = *it;
		po.road->addBlock(pp.position, pp.rotation, pp.type, pp.width, pp.bwidth, pp.bheight, pp.pillartype);
	}
	po.road->finish();

	po.loadingState = 1;
	return 0;
}

int ProceduralManager::updateAllObjects()
{
	LOG(" *** ProceduralManager::updateAllObjects");
	std::vector<ProceduralObject>::iterator it;
	for (it=pObjects.begin();it!=pObjects.end();it++)
	{
		updateObject(*it);
	}
	return 0;
}

int ProceduralManager::deleteAllObjects()
{
	LOG(" *** ProceduralManager::deleteAllObjects");
	std::vector<ProceduralObject>::iterator it;
	for (it=pObjects.begin();it!=pObjects.end();it++)
	{
		deleteObject(*it);
	}
	return 0;
}

int ProceduralManager::addObject(ProceduralObject &po)
{
	updateObject(po);
	pObjects.push_back(po);
	return 0;
}
