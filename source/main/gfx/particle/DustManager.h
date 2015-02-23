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
#ifndef __DustManager_H_
#define __DustManager_H_

#include "RoRPrerequisites.h"

#include "Singleton.h"

class DustManager : public RoRSingletonNoCreation < DustManager >, public ZeroedMemoryAllocator
{
public:

	DustManager();
	~DustManager();

	DustPool *getGroundModelDustPool(ground_model_t *g);
	
	void update(float wspeed);

	void setVisible(bool visible);

	DustPool *getDustPool(Ogre::String name);
	
protected:

	bool mEnabled;
	std::map < Ogre::String , DustPool * > dustpools;
};

#endif // __DustManager_H_
