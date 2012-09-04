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
// created on 15th of May 2011 by Thomas Fischer

#ifndef OGRESCRIPTBUILDER_H__
#define OGRESCRIPTBUILDER_H__

#include "RoRPrerequisites.h"

#include "scriptbuilder/scriptbuilder.h"

#include <angelscript.h>

// our own class that wraps the CScriptBuilder and just overwrites the file loading parts
// to use the ogre resource system
class OgreScriptBuilder : public AngelScript::CScriptBuilder, public ZeroedMemoryAllocator
{
public:
	Ogre::String getHash() { return hash; };
protected:
	Ogre::String hash;
	int LoadScriptSection(const char *filename);
};

#endif //OGRESCRIPTBUILDER_H__
