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

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 23rd of December 2009

#ifndef PLAYERCOLOURS_H__
#define PLAYERCOLOURS_H__

#include "RoRPrerequisites.h"

#include "Singleton.h"


class PlayerColours : public RoRSingleton < PlayerColours >, public ZeroedMemoryAllocator
{
public:
	PlayerColours();
	~PlayerColours();

	void updateMaterial(int colourNum, Ogre::String materialName, int textureUnitStateNum=0);
	Ogre::String getColourMaterial(int colourNum);
	Ogre::ColourValue getColour(int colourNum);
protected:
	void updatePlayerColours();
};



#endif //PLAYERCOLOURS_H__
