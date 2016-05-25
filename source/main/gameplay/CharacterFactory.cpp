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

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 7th of August 2009

#include "CharacterFactory.h"

#include "Character.h"
#include "Network.h"

CharacterFactory::CharacterFactory()
{
}

CharacterFactory::~CharacterFactory()
{
}

Character *CharacterFactory::createLocal(int playerColour)
{
	Character *ch = new Character(-1, 0, playerColour, false);
	return ch;
}

Character *CharacterFactory::createRemoteInstance()
{
	return nullptr;
}

void CharacterFactory::update(float dt)
{
	gEnv->player->update(dt);
}

void CharacterFactory::updateLabels()
{
}
