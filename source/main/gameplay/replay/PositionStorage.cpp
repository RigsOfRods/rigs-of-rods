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
#include "PositionStorage.h"
#include "Ogre.h"


using namespace Ogre;

PositionStorage::PositionStorage(int _numNodes, int _numStorage) : numNodes(_numNodes), numStorage(_numStorage)
{
	nodes = (Vector3*)calloc(numNodes * numStorage, sizeof(Vector3));
	usage = (bool*)calloc(numStorage, sizeof(bool));
}

PositionStorage::~PositionStorage()
{
	if (nodes) free(nodes);
	if (usage) free(usage);
}

Vector3 *PositionStorage::getStorage(int indexNum)
{
	if (indexNum < 0 || indexNum > numStorage) return 0;
	return nodes + (indexNum * numNodes);
}

void PositionStorage::setUsage(int indexNum, bool use)
{
	*(usage + (indexNum)) = use;
}

bool PositionStorage::getUsage(int indexNum)
{
	return *(usage + (indexNum));
}
