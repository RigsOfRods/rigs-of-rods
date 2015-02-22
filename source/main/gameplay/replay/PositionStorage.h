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
// created on the 26th of July 2009 by Thomas Fischer (thomas{AT}thomasfischer{DOT}biz)
#ifndef __PositionStorage_H__
#define __PositionStorage_H__

#include "RoRPrerequisites.h"
#include "OgrePrerequisites.h"


class PositionStorage : public ZeroedMemoryAllocator
{
protected:
	Ogre::Vector3 *nodes;
	bool *usage;
	int numNodes;
	int numStorage;

public:
	PositionStorage(int numNodes, int numStorage);
	~PositionStorage();

	Ogre::Vector3 *getStorage(int indexNum);
	
	void setUsage(int indexNum, bool use);
	bool getUsage(int indexNum);
};
#endif //__PositionStorage_H__
