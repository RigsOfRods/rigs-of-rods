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

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 17th of August 2009

#ifndef __CharacterFactory_H_
#define __CharacterFactory_H_

#include "RoRPrerequisites.h"

#include "Character.h"
#include "StreamableFactory.h"

class CharacterFactory : public StreamableFactory < CharacterFactory, Character >, public ZeroedMemoryAllocator
{
	friend class Network;

public:

	CharacterFactory();
	~CharacterFactory();

	Character *createLocal(int playerColour);
	Character *createRemoteInstance(stream_reg_t *reg);

	void updateCharacters(float dt);
	void updateLabels();

protected:

	// functions used by friends
	void netUserAttributesChanged(int source, int streamid);
	void localUserAttributesChanged(int newid);
};

#endif // __CharacterFactory_H_
